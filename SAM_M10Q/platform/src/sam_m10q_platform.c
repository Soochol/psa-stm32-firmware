/*
 * sam_m10q_platform.c
 *
 * SAM-M10Q GPS Module - STM32 Platform Implementation
 * I2C3 integration and application interface
 *
 * Created: 2025-01-11
 */

#include "main.h"
#include "sam_m10q_platform.h"
#include "i2c.h"
#include "tim.h"
#include "mode.h"
#include "uart.h"
#include "lib_log.h"
#include <string.h>

// I2C address for GPS is defined in i2c.h as ADDR_GPS

// Driver instance
static _x_SAM_M10Q_DRV_t x_gps_drv;
static _x_SAM_M10Q_DRV_t* px_gps = &x_gps_drv;

// Communication state
static volatile e_COMM_STAT_t e_gps_comm;
static uint32_t u32_toutRef;

/*
 * Initialization state machine
 *
 * v_GPS_Init() is now a non-blocking kicker — it queues a state machine
 * driven by v_GPS_Handler() (called every main loop iteration). Each state
 * issues at most one i_GPS_Read/i_GPS_Write (interrupt mode) and waits for
 * the existing v_GPS_*_DoneHandler callbacks to set e_gps_comm = DONE.
 *
 * Total wall-clock for init: ~2-3 seconds (mostly the post-12V boot wait
 * and CFG-VALSET ACK). Main-loop occupancy per iteration: ~10us. The main
 * loop is never blocked.
 *
 * State transitions:
 *   IDLE → WAIT_BOOT (kicked by v_GPS_Init)
 *   WAIT_BOOT → DRAIN_AVAIL_REQ (after 2000ms)
 *   DRAIN_AVAIL_REQ → DRAIN_AVAIL_WAIT → (avail==0 ? CFG_WRITE_REQ : DRAIN_DATA_REQ)
 *   DRAIN_DATA_REQ → DRAIN_DATA_WAIT → DRAIN_AVAIL_REQ (loop, max 3 attempts)
 *   CFG_WRITE_REQ → CFG_WRITE_WAIT → ACK_AVAIL_REQ
 *   ACK_AVAIL_REQ → ACK_AVAIL_WAIT → (avail>0 ? ACK_DATA_REQ : ACK_TIMEOUT_CHECK)
 *   ACK_DATA_REQ → ACK_DATA_WAIT → (parse: ACK→RESTART_DELAY, NAK→CFG_WRITE_REQ retry,
 *                                          neither→ACK_TIMEOUT_CHECK)
 *   ACK_TIMEOUT_CHECK → (within 500ms ? ACK_AVAIL_REQ : retry CFG_WRITE_REQ or DONE)
 *   RESTART_DELAY → DONE (after 500ms)
 *   FAIL is a terminal state for graceful skip (GPS not connected, etc.)
 */
typedef enum {
    GPS_INIT_IDLE = 0,
    GPS_INIT_WAIT_BOOT,
    GPS_INIT_DRAIN_AVAIL_REQ,
    GPS_INIT_DRAIN_AVAIL_WAIT,
    GPS_INIT_DRAIN_DATA_REQ,
    GPS_INIT_DRAIN_DATA_WAIT,
    GPS_INIT_CFG_WRITE_REQ,
    GPS_INIT_CFG_WRITE_WAIT,
    GPS_INIT_ACK_AVAIL_REQ,
    GPS_INIT_ACK_AVAIL_WAIT,
    GPS_INIT_ACK_DATA_REQ,
    GPS_INIT_ACK_DATA_WAIT,
    GPS_INIT_ACK_TIMEOUT_CHECK,
    GPS_INIT_RESTART_DELAY,
    GPS_INIT_DONE,
    GPS_INIT_FAIL,
} e_gps_init_state_t;

static volatile e_gps_init_state_t e_gps_init_state = GPS_INIT_IDLE;
static uint32_t u32_gps_init_t_start;       // WAIT_BOOT, RESTART_DELAY anchor
static uint32_t u32_gps_ack_t_start;        // ACK overall timeout (500ms) anchor
static uint8_t  u8_gps_drain_attempts;      // drain retry counter (max 3)
static uint8_t  u8_gps_cfg_attempts;        // CFG-VALSET retry counter (max 3)
static uint8_t  u8_gps_cfg_buf[128];        // CFG-VALSET payload (built once in kicker)
                                            // Sized for 11 config keys (~69 bytes) + headroom.
                                            // The legacy 64-byte buffer was a latent overflow bug
                                            // that the new size-check (line ~150) finally surfaces.
static uint16_t u16_gps_cfg_len;
static uint32_t u32_gps_init_kick_time;     // anchor for elapsed-time logging (RTT verification)

// Forward declarations of transport functions
static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);
static int i_GPS_Bus(void);
static uint32_t u32_GPS_GetTime(void);
static void v_GPS_Init_StateMachine(void);

/*
 * brief: Initialize GPS module
 * note: Non-blocking kicker — sets up driver state and arms the init state
 *       machine. The actual drain → CFG-VALSET → ACK → restart sequence runs
 *       across many main-loop iterations via v_GPS_Init_StateMachine() which
 *       is dispatched from v_GPS_Handler(). Total wall-clock ~2-3s, main-loop
 *       occupancy per iteration ~10us.
 */
void v_GPS_Init(void) {
    u32_gps_init_kick_time = u32_Tim_1msGet();
    LOG_INFO("GPS", "*** GPS INIT START (async state machine) ***");

    // Ensure I2C3 interrupts are enabled (interrupt-mode I2C is required by
    // i_GPS_Read/i_GPS_Write — both queue transactions and rely on the
    // v_GPS_*_DoneHandler callbacks fired from the I2C3 ISR).
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
    LOG_DEBUG("GPS", "I2C3 interrupts enabled");

    // Reset comm state — both the GPS protocol layer and the underlying I2C3
    // driver. v_GPS_Read_DoneHandler / v_GPS_Write_DoneHandler will set
    // e_gps_comm = COMM_STAT_DONE when transactions complete.
    e_gps_comm = COMM_STAT_READY;
    v_I2C3_Set_Comm_Ready();

    // Setup transport layer (function pointers consumed by SAM_M10Q core driver)
    _x_SAM_M10Q_TRANS_t trans;
    trans.i_write = i_GPS_Write;
    trans.i_read = i_GPS_Read;
    trans.i_bus = i_GPS_Bus;
    trans.u32_getTime = u32_GPS_GetTime;

    int ret = i_SAM_M10Q_Init(px_gps, &trans, SAM_M10Q_I2C_ADDR_DEFAULT);
    if(ret != SAM_M10Q_RET_OK) {
        LOG_ERROR("GPS", "Driver init failed (ret=%d) - skipping GPS for this session", ret);
        e_gps_init_state = GPS_INIT_FAIL;
        return;
    }
    LOG_INFO("GPS", "Driver initialized (I2C3, addr=0x%02X)", SAM_M10Q_I2C_ADDR_DEFAULT);

    // Pre-build the CFG-VALSET message ONCE (cheap, no I2C). The state
    // machine will issue this same buffer up to 3 times if the GPS NACKs
    // or fails to ACK within the timeout window.
    extern int i_UBX_CreateCfgI2C(uint8_t* pu8_buf, uint16_t u16_bufSize,
                                   uint8_t u8_layer, uint8_t u8_i2cAddr, bool b_enableNMEA);
    int cfg_len = i_UBX_CreateCfgI2C(u8_gps_cfg_buf, sizeof(u8_gps_cfg_buf),
                                      0x01,  // RAM layer only (SparkFun method)
                                      SAM_M10Q_I2C_ADDR_DEFAULT,
                                      false); // Disable NMEA
    if(cfg_len < 0 || cfg_len > (int)sizeof(u8_gps_cfg_buf)) {
        LOG_ERROR("GPS", "Failed to build CFG-VALSET (cfg_len=%d) - skipping GPS", cfg_len);
        e_gps_init_state = GPS_INIT_FAIL;
        return;
    }
    u16_gps_cfg_len = (uint16_t)cfg_len;
    LOG_INFO("GPS", "CFG-VALSET prebuilt (%d bytes)", cfg_len);

    // Arm the state machine. v_GPS_Handler() (called every main-loop iter)
    // will dispatch into v_GPS_Init_StateMachine() and advance one step at
    // a time, never blocking.
    u32_gps_init_t_start = u32_Tim_1msGet();
    u8_gps_drain_attempts = 0;
    u8_gps_cfg_attempts = 0;
    e_gps_init_state = GPS_INIT_WAIT_BOOT;

    LOG_INFO("GPS", "Init kicker armed (state=WAIT_BOOT) — handler will advance");
}

/*
 * brief: GPS init state machine — advances one state per main-loop iteration.
 *
 * Called only from v_GPS_Handler() while e_gps_init_state is non-terminal.
 * Each state issues at most one i_GPS_Read/i_GPS_Write (interrupt mode) and
 * waits for the existing v_GPS_*_DoneHandler callbacks to set e_gps_comm to
 * COMM_STAT_DONE. The handler returns immediately (~10us per call).
 */
static void v_GPS_Init_StateMachine(void) {
    uint32_t now = u32_Tim_1msGet();

    switch(e_gps_init_state) {

    case GPS_INIT_IDLE:
    case GPS_INIT_DONE:
    case GPS_INIT_FAIL:
        return;  // Terminal / not-yet-armed states

    case GPS_INIT_WAIT_BOOT:
        // SAM-M10Q needs ~2s after 12V enable to be I2C-responsive
        if((now - u32_gps_init_t_start) >= 2000) {
            LOG_INFO("GPS", "Boot wait done — starting drain");
            u8_gps_drain_attempts = 0;
            e_gps_comm = COMM_STAT_READY;
            e_gps_init_state = GPS_INIT_DRAIN_AVAIL_REQ;
        }
        return;

    case GPS_INIT_DRAIN_AVAIL_REQ:
        // Issue read of available-bytes registers (0xFD/0xFE).
        // v_GPS_Read_DoneHandler stores the result into px_gps->u16_availBytes
        // because u16_len == 2.
        if(i_GPS_Read(ADDR_GPS, SAM_M10Q_REG_AVAIL_MSB, 2) == 0) {
            e_gps_init_state = GPS_INIT_DRAIN_AVAIL_WAIT;
        }
        // else: I2C bus busy — retry next iteration. v_GPS_Tout_Handler
        // (1.2s) is the safety net for stuck transactions.
        return;

    case GPS_INIT_DRAIN_AVAIL_WAIT:
        if(e_gps_comm == COMM_STAT_DONE) {
            uint16_t avail = px_gps->u16_availBytes;
            e_gps_comm = COMM_STAT_READY;

            if(avail == 0 || avail == 0xFFFF) {
                // Drain complete — proceed to CFG-VALSET write
                LOG_INFO("GPS", "Drain OK (avail=0x%04X)", avail);
                e_gps_init_state = GPS_INIT_CFG_WRITE_REQ;
            } else if(u8_gps_drain_attempts >= 3) {
                LOG_WARN("GPS", "Drain max attempts — proceeding anyway");
                e_gps_init_state = GPS_INIT_CFG_WRITE_REQ;
            } else {
                u8_gps_drain_attempts++;
                LOG_INFO("GPS", "Drain - %d bytes avail (att=%d)", avail, u8_gps_drain_attempts);
                e_gps_init_state = GPS_INIT_DRAIN_DATA_REQ;
            }
        }
        return;

    case GPS_INIT_DRAIN_DATA_REQ:
        {
            // u8_rxBuf is 128 bytes — clamp the read to that size.
            uint16_t avail = px_gps->u16_availBytes;
            uint16_t to_read = (avail > sizeof(px_gps->u8_rxBuf))
                                ? sizeof(px_gps->u8_rxBuf) : avail;
            if(i_GPS_Read(ADDR_GPS, SAM_M10Q_REG_STREAM, to_read) == 0) {
                e_gps_init_state = GPS_INIT_DRAIN_DATA_WAIT;
            }
        }
        return;

    case GPS_INIT_DRAIN_DATA_WAIT:
        if(e_gps_comm == COMM_STAT_DONE) {
            e_gps_comm = COMM_STAT_READY;
            // Loop back to check available bytes again
            e_gps_init_state = GPS_INIT_DRAIN_AVAIL_REQ;
        }
        return;

    case GPS_INIT_CFG_WRITE_REQ:
        if(i_GPS_Write(ADDR_GPS, SAM_M10Q_REG_STREAM, u8_gps_cfg_buf, u16_gps_cfg_len) == 0) {
            u8_gps_cfg_attempts++;
            LOG_INFO("GPS", "CFG-VALSET attempt %d (%d bytes)", u8_gps_cfg_attempts, u16_gps_cfg_len);
            e_gps_init_state = GPS_INIT_CFG_WRITE_WAIT;
        }
        return;

    case GPS_INIT_CFG_WRITE_WAIT:
        if(e_gps_comm == COMM_STAT_DONE) {
            e_gps_comm = COMM_STAT_READY;
            // Start the 500ms ACK polling window
            u32_gps_ack_t_start = now;
            e_gps_init_state = GPS_INIT_ACK_AVAIL_REQ;
        }
        return;

    case GPS_INIT_ACK_AVAIL_REQ:
        if(i_GPS_Read(ADDR_GPS, SAM_M10Q_REG_AVAIL_MSB, 2) == 0) {
            e_gps_init_state = GPS_INIT_ACK_AVAIL_WAIT;
        }
        return;

    case GPS_INIT_ACK_AVAIL_WAIT:
        if(e_gps_comm == COMM_STAT_DONE) {
            uint16_t avail = px_gps->u16_availBytes;
            e_gps_comm = COMM_STAT_READY;
            if(avail > 0 && avail != 0xFFFF) {
                e_gps_init_state = GPS_INIT_ACK_DATA_REQ;
            } else {
                e_gps_init_state = GPS_INIT_ACK_TIMEOUT_CHECK;
            }
        }
        return;

    case GPS_INIT_ACK_DATA_REQ:
        {
            uint16_t avail = px_gps->u16_availBytes;
            uint16_t to_read = (avail > sizeof(px_gps->u8_rxBuf))
                                ? sizeof(px_gps->u8_rxBuf) : avail;
            if(i_GPS_Read(ADDR_GPS, SAM_M10Q_REG_STREAM, to_read) == 0) {
                e_gps_init_state = GPS_INIT_ACK_DATA_WAIT;
            }
        }
        return;

    case GPS_INIT_ACK_DATA_WAIT:
        if(e_gps_comm == COMM_STAT_DONE) {
            uint16_t to_read = px_gps->u16_rxLen;
            e_gps_comm = COMM_STAT_READY;

            // Scan rxBuf for a UBX ACK message addressed to CFG-VALSET (0x06/0x8A)
            int ack_status = -1;  // -1=not found, 0=NAK, 1=ACK
            if(to_read >= 10) {
                for(uint16_t i = 0; i < to_read - 9; i++) {
                    if(px_gps->u8_rxBuf[i]   == UBX_SYNC_CHAR_1 &&
                       px_gps->u8_rxBuf[i+1] == UBX_SYNC_CHAR_2 &&
                       px_gps->u8_rxBuf[i+2] == UBX_CLASS_ACK) {
                        uint8_t ackType   = px_gps->u8_rxBuf[i+3];
                        uint16_t length   = px_gps->u8_rxBuf[i+4] | (px_gps->u8_rxBuf[i+5] << 8);
                        if(length != 2) continue;
                        uint8_t ack_class = px_gps->u8_rxBuf[i+6];
                        uint8_t ack_id    = px_gps->u8_rxBuf[i+7];

                        // Verify checksum over class/id/length/payload (6 bytes)
                        uint8_t ckA, ckB;
                        extern void v_UBX_CalcChecksum(uint8_t* pu8_data, uint16_t u16_len,
                                                       uint8_t* pu8_ckA, uint8_t* pu8_ckB);
                        v_UBX_CalcChecksum(&px_gps->u8_rxBuf[i+2], 6, &ckA, &ckB);
                        if(px_gps->u8_rxBuf[i+8] != ckA || px_gps->u8_rxBuf[i+9] != ckB) continue;

                        // ACK is for CFG-VALSET (class 0x06, id 0x8A)?
                        if(ack_class == UBX_CLASS_CFG && ack_id == UBX_CFG_VALSET) {
                            if(ackType == UBX_ACK_ACK) { ack_status = 1; break; }
                            if(ackType == UBX_ACK_NAK) { ack_status = 0; break; }
                        }
                    }
                }
            }

            if(ack_status == 1) {
                LOG_INFO("GPS", "CFG-VALSET accepted! NMEA disabled, UBX only");
                u32_gps_init_t_start = now;  // anchor for the 500ms restart delay
                e_gps_init_state = GPS_INIT_RESTART_DELAY;
            } else if(ack_status == 0) {
                LOG_WARN("GPS", "CFG-VALSET rejected (NAK) - retrying");
                if(u8_gps_cfg_attempts < 3) {
                    e_gps_init_state = GPS_INIT_CFG_WRITE_REQ;
                } else {
                    LOG_ERROR("GPS", "CFG-VALSET NAK after 3 attempts — proceeding without NMEA disable");
                    e_gps_init_state = GPS_INIT_DONE;
                }
            } else {
                // Neither ACK nor NAK in this batch — keep polling within timeout
                e_gps_init_state = GPS_INIT_ACK_TIMEOUT_CHECK;
            }
        }
        return;

    case GPS_INIT_ACK_TIMEOUT_CHECK:
        if((now - u32_gps_ack_t_start) >= 500) {
            // 500ms ACK window expired
            if(u8_gps_cfg_attempts < 3) {
                LOG_WARN("GPS", "ACK timeout, retry CFG-VALSET (attempt %d)", u8_gps_cfg_attempts);
                e_gps_init_state = GPS_INIT_CFG_WRITE_REQ;
            } else {
                LOG_ERROR("GPS", "CFG-VALSET ACK timeout after 3 attempts — proceeding without NMEA disable");
                // Still mark DONE — GPS will work, just less efficient (NMEA enabled)
                e_gps_init_state = GPS_INIT_DONE;
            }
        } else {
            // Still within window — poll again
            e_gps_init_state = GPS_INIT_ACK_AVAIL_REQ;
        }
        return;

    case GPS_INIT_RESTART_DELAY:
        // u-blox: wait 500ms after CFG-VALSET ACK for the GNSS subsystem to restart
        if((now - u32_gps_init_t_start) >= 500) {
            LOG_INFO("GPS", "GNSS subsystem ready — init complete (total t=%lums)",
                     (unsigned long)(u32_Tim_1msGet() - u32_gps_init_kick_time));
            e_gps_init_state = GPS_INIT_DONE;
        }
        return;
    }
}

/*
 * brief: Deinitialize GPS module
 */
void v_GPS_Deinit(void) {
    e_gps_init_state = GPS_INIT_IDLE;
    e_gps_comm = COMM_STAT_READY;
}

/*
 * brief: Reset GPS communication state
 * note: Use after diagnostic tests that may corrupt I2C state
 */
void v_GPS_Reset_Comm(void) {
    e_gps_comm = COMM_STAT_READY;
}

/*
 * brief: I2C write done callback
 */
void v_GPS_Write_DoneHandler(uint8_t u8_addr) {
    if(u8_addr == ADDR_GPS) {
        e_gps_comm = COMM_STAT_DONE;
    }
}

/*
 * brief: I2C read done callback
 */
void v_GPS_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len) {
    if(u8_addr == ADDR_GPS) {
        // Check if this was reading available bytes or actual data
        if(u16_len == 2) {
            // Available bytes count (registers 0xFD, 0xFE) - big-endian
            px_gps->u16_availBytes = (pu8_arr[0] << 8) | pu8_arr[1];
            LOG_DEBUG("GPS", "Available bytes=%d", px_gps->u16_availBytes);
        } else {
            // FIX: Add buffer overflow protection
            if(u16_len <= sizeof(px_gps->u8_rxBuf)) {
                memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);
                px_gps->u16_rxLen = u16_len;
                LOG_DEBUG("GPS", "Received %d bytes", u16_len);
            } else {
                // Buffer overflow error
                LOG_ERROR("GPS", "Buffer overflow! len=%d > bufSize=%d",
                              u16_len, sizeof(px_gps->u8_rxBuf));
                px_gps->u16_rxLen = 0;  // Invalidate data
            }
        }
        e_gps_comm = COMM_STAT_DONE;
    }
}

/*
 * brief: Initialization status query (called from v_Mode_Booting)
 *
 * Returns COMM_STAT_DONE for both successful init AND graceful skip (e.g.,
 * GPS not connected). The mode handler treats GPS as non-critical, mirroring
 * the IMU/Temp graceful-skip pattern. COMM_STAT_BUSY means the state machine
 * is still running.
 */
e_COMM_STAT_t e_GPS_Ready(void) {
    if(e_gps_init_state == GPS_INIT_DONE)  return COMM_STAT_DONE;
    if(e_gps_init_state == GPS_INIT_FAIL)  return COMM_STAT_DONE;  // Graceful skip
    if(e_gps_init_state == GPS_INIT_IDLE)  return COMM_STAT_READY; // Not yet kicked
    return COMM_STAT_BUSY;  // State machine in progress
}

/*
 * brief: Main processing handler
 *
 * Called every main-loop iteration. While the init state machine is active
 * (state != DONE && state != FAIL) we run only the init dispatcher and skip
 * the normal SAM_M10Q handler — this guarantees a single outstanding I2C
 * transaction at a time on the GPS bus during init. Once init reaches DONE,
 * normal polling takes over.
 */
void v_GPS_Handler(void) {
#if IWDG_USED
    // CRITICAL: Refresh watchdog during GPS processing
    // GPS I2C communication can take significant time (UBX protocol)
    extern IWDG_HandleTypeDef hiwdg1;
    HAL_IWDG_Refresh(&hiwdg1);
#endif

    if(e_gps_init_state != GPS_INIT_DONE && e_gps_init_state != GPS_INIT_FAIL) {
        v_GPS_Init_StateMachine();
        return;
    }

    i_SAM_M10Q_Handler(px_gps);
}

/*
 * brief: Timeout handler (1.2 second timeout for 1Hz Auto PVT)
 */
void v_GPS_Tout_Handler(void) {
    if((e_gps_comm == COMM_STAT_BUSY) &&
       _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 1200)) {
        // Abort I2C transaction
        extern I2C_HandleTypeDef hi2c3;
        HAL_I2C_Master_Abort_IT(&hi2c3, ADDR_GPS);
        e_gps_comm = COMM_STAT_READY;

        // GPS timeout - log warning but don't enter ERROR mode
        // This allows system to continue operating without GPS
        LOG_WARN("GPS", "I2C timeout (no response)");
    }
}

// ========== Transport Layer Functions ==========

static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len) {
    // STEP 2: Add BUSY timeout recovery
    static uint32_t busy_start = 0;

    // Check if GPS state is stuck in BUSY
    if(e_gps_comm == COMM_STAT_BUSY) {
        uint32_t now = u32_Tim_1msGet();
        if(busy_start == 0) {
            busy_start = now;  // Start timeout timer
        } else if((now - busy_start) > 1200) {  // 1.2 second timeout (1.2x nav rate)
            LOG_WARN("GPS", "Write BUSY timeout - force READY recovery");
            e_gps_comm = COMM_STAT_READY;
            busy_start = 0;
            // Allow this write to proceed after recovery
        }
    } else {
        busy_start = 0;  // Reset timeout timer when not BUSY
    }

    u32_toutRef = u32_Tim_1msGet();
    e_gps_comm = COMM_STAT_BUSY;

    int ret = i_I2C3_Write(ADDR_GPS, u16_reg, pu8_arr, u16_len);

    // STEP 3: Improve error code handling
    // COMM_STAT_FIFO_FULL (1) means transaction was successfully queued
    // Only COMM_STAT_OK (1) and COMM_STAT_READY (0) are success codes
    if(ret == COMM_STAT_OK || ret == COMM_STAT_FIFO_FULL) {
        return 0;  // Success - transaction queued or started
    }

    // Actual errors (ret >= 2)
    LOG_ERROR("GPS", "Write failed (reg=0x%02X, len=%d, ret=%d)", u16_reg, u16_len, ret);
    return ret;
}

static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len) {
    // STEP 2: Add BUSY timeout recovery
    static uint32_t busy_start = 0;

    // Check if GPS state is stuck in BUSY
    if(e_gps_comm == COMM_STAT_BUSY) {
        uint32_t now = u32_Tim_1msGet();
        if(busy_start == 0) {
            busy_start = now;  // Start timeout timer
        } else if((now - busy_start) > 1200) {  // 1.2 second timeout (1.2x nav rate)
            LOG_WARN("GPS", "Read BUSY timeout - force READY recovery");
            e_gps_comm = COMM_STAT_READY;
            busy_start = 0;
            // Allow this read to proceed after recovery
        }
    } else {
        busy_start = 0;  // Reset timeout timer when not BUSY
    }

    u32_toutRef = u32_Tim_1msGet();
    e_gps_comm = COMM_STAT_BUSY;

    int ret = i_I2C3_Read(ADDR_GPS, u16_reg, u16_len);

    // STEP 3: Improve error code handling
    // COMM_STAT_FIFO_FULL (1) means transaction was successfully queued
    // COMM_STAT_OK (1) and COMM_STAT_READY (0) are success codes
    if(ret == COMM_STAT_OK || ret == COMM_STAT_FIFO_FULL || ret == COMM_STAT_READY) {
        return 0;  // Success - transaction queued or started
    }

    // Actual errors (ret >= 2)
    LOG_ERROR("GPS", "Read failed (reg=0x%02X, len=%d, ret=%d)", u16_reg, u16_len, ret);
    return ret;
}

static int i_GPS_Bus(void) {
    // FIX: Simplified bus check logic with debugging
    // e_gps_comm is the GPS protocol state (managed by callbacks)
    // e_comm_i2c3 is the I2C3 hardware state (managed by i2c.c)
    // These two states are independent - GPS driver only needs to check e_gps_comm

    static uint32_t last_debug_log = 0;
    uint32_t now = u32_Tim_1msGet();
    bool should_log = (now - last_debug_log) > 10000;  // Log every 10s

    // Auto-reset DONE state to READY to allow next transaction
    if(e_gps_comm == COMM_STAT_DONE) {
        if(should_log) {
            LOG_DEBUG("GPS", "Bus: DONE → READY (returning 0)");
            last_debug_log = now;
        }
        e_gps_comm = COMM_STAT_READY;
        return 0;  // Transaction completed, ready for next operation
    }

    // Check if GPS communication channel is ready
    if(e_gps_comm == COMM_STAT_READY) {
        if(should_log) {
            LOG_DEBUG("GPS", "Bus: READY (returning 0)");
            last_debug_log = now;
        }
        return 0;  // Bus is ready for next operation
    }

    // Bus is busy (COMM_STAT_BUSY or COMM_STAT_OK - waiting for callback)
    if(should_log) {
        LOG_DEBUG("GPS", "Bus: BUSY (e_gps_comm=%d, returning -1)", e_gps_comm);
        last_debug_log = now;
    }
    return -1;
}

static uint32_t u32_GPS_GetTime(void) {
    return u32_Tim_1msGet();
}

// ========== Application Data Access Functions ==========

bool b_GPS_HasFix(void) {
    return (px_gps->b_pvtValid && px_gps->x_pvt.fixType >= GPS_FIX_2D);
}

uint8_t u8_GPS_GetFixType(void) {
    if(!px_gps) return 0;
    return px_gps->x_pvt.fixType;
}

uint8_t u8_GPS_GetNumSatellites(void) {
    if(!px_gps) return 0;
    return px_gps->x_pvt.numSV;
}

float f_GPS_GetLatitude(void) {
    // CRITICAL FIX: Return 0.0 if GPS data is invalid
    // Prevents crash from accessing uninitialized data
    if(!px_gps || !px_gps->b_pvtValid) {
        return 0.0f;
    }
    // Convert from 1e-7 degrees to degrees
    return (float)px_gps->x_pvt.lat / 10000000.0f;
}

float f_GPS_GetLongitude(void) {
    // CRITICAL FIX: Return 0.0 if GPS data is invalid
    // Prevents crash from accessing uninitialized data
    if(!px_gps || !px_gps->b_pvtValid) {
        return 0.0f;
    }
    // Convert from 1e-7 degrees to degrees
    return (float)px_gps->x_pvt.lon / 10000000.0f;
}

float f_GPS_GetAltitude(void) {
    if(!px_gps || !px_gps->b_pvtValid) {
        return 0.0f;
    }
    // Convert from mm to meters
    return (float)px_gps->x_pvt.hMSL / 1000.0f;
}

float f_GPS_GetSpeed(void) {
    if(!px_gps || !px_gps->b_pvtValid) {
        return 0.0f;
    }
    // Convert from mm/s to m/s
    return (float)px_gps->x_pvt.gSpeed / 1000.0f;
}

float f_GPS_GetHeading(void) {
    if(!px_gps || !px_gps->b_pvtValid) {
        return 0.0f;
    }
    // Convert from 1e-5 degrees to degrees
    return (float)px_gps->x_pvt.headMot / 100000.0f;
}

float f_GPS_GetHorizontalAccuracy(void) {
    // Convert from mm to meters
    return (float)px_gps->x_pvt.hAcc / 1000.0f;
}

float f_GPS_GetVerticalAccuracy(void) {
    // Convert from mm to meters
    return (float)px_gps->x_pvt.vAcc / 1000.0f;
}

void v_GPS_GetDateTime(uint16_t* pu16_year, uint8_t* pu8_month, uint8_t* pu8_day,
                       uint8_t* pu8_hour, uint8_t* pu8_min, uint8_t* pu8_sec) {
    if(pu16_year) *pu16_year = px_gps->x_pvt.year;
    if(pu8_month) *pu8_month = px_gps->x_pvt.month;
    if(pu8_day)   *pu8_day = px_gps->x_pvt.day;
    if(pu8_hour)  *pu8_hour = px_gps->x_pvt.hour;
    if(pu8_min)   *pu8_min = px_gps->x_pvt.min;
    if(pu8_sec)   *pu8_sec = px_gps->x_pvt.sec;
}

bool b_GPS_IsTimeValid(void) {
    return (px_gps->x_pvt.valid & GPS_VALID_TIME) != 0;
}

_x_GPS_PVT_t* px_GPS_GetPVT(void) {
    return &px_gps->x_pvt;
}

/*
 * brief: Test function - print GPS data periodically
 */
void v_GPS_Test(void) {
    static uint32_t timRef = 0;
    if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 2000)) {
        timRef = u32_Tim_1msGet();

        if(b_GPS_HasFix()) {
            LOG_INFO("GPS", "Lat=%.6f Lon=%.6f Alt=%.1fm Sats=%d Fix=%d",
                          f_GPS_GetLatitude(),
                          f_GPS_GetLongitude(),
                          f_GPS_GetAltitude(),
                          u8_GPS_GetNumSatellites(),
                          u8_GPS_GetFixType());
        } else {
            LOG_INFO("GPS", "No fix (Sats=%d)", u8_GPS_GetNumSatellites());
        }
    }
}
