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

// GPS DEBUG 로그 비활성화 (로그 과다 출력 방지)
#define GPS_DEBUG_LOG_ENABLED	0

#if GPS_DEBUG_LOG_ENABLED
    #define GPS_LOG_DEBUG(...)  LOG_DEBUG("GPS", __VA_ARGS__)
#else
    #define GPS_LOG_DEBUG(...)  ((void)0)
#endif

// I2C address for GPS is defined in i2c.h as ADDR_GPS

// Driver instance
static _x_SAM_M10Q_DRV_t x_gps_drv;
static _x_SAM_M10Q_DRV_t* px_gps = &x_gps_drv;

// Communication state
static volatile e_COMM_STAT_t e_gps_comm;
static uint32_t u32_toutRef;

// Initialization state
static e_COMM_STAT_t e_gps_init;

// Forward declarations of transport functions
static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);
static int i_GPS_Bus(void);
static uint32_t u32_GPS_GetTime(void);

/*
 * brief: Initialize GPS module
 * note: Main branch approach - GPS modules ship with I2C enabled by default
 *       No explicit I2C configuration required, handler will detect module
 */
/*
 * Helper: Wait for ACK-ACK or ACK-NAK response from GPS
 * Returns: 1 if ACK-ACK received, 0 if ACK-NAK received, -1 if timeout/error
 */
static int i_GPS_WaitForACK(I2C_HandleTypeDef* hi2c, uint8_t addr,
                            uint8_t msgClass, uint8_t msgID, uint32_t timeout_ms) {
    uint8_t avail_buf[2];
    uint8_t read_buf[256];  // Buffer for reading ACK + possible NMEA data
    uint32_t start_time = HAL_GetTick();

#if IWDG_USED
    extern IWDG_HandleTypeDef hiwdg1;
#endif

    GPS_LOG_DEBUG("Waiting for ACK (Class=0x%02X, ID=0x%02X)...", msgClass, msgID);

    // Start reading immediately - GPS sends ACK very quickly
    while((HAL_GetTick() - start_time) < timeout_ms) {
#if IWDG_USED
        HAL_IWDG_Refresh(&hiwdg1);
#endif

        // Check if data available
        HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, addr, SAM_M10Q_REG_AVAIL_MSB,
                                                  I2C_MEMADD_SIZE_8BIT, avail_buf, 2, 200);

        if(ret != HAL_OK) {
            LOG_ERROR("GPS", "ACK wait - I2C error (0x%02lX)", hi2c->ErrorCode);
            return -1;
        }

        uint16_t avail_bytes = (avail_buf[0] << 8) | avail_buf[1];

        if(avail_bytes > 0 && avail_bytes != 0xFFFF) {
            // Read available data
            uint16_t to_read = (avail_bytes > sizeof(read_buf)) ? sizeof(read_buf) : avail_bytes;
            ret = HAL_I2C_Mem_Read(hi2c, addr, SAM_M10Q_REG_STREAM,
                                   I2C_MEMADD_SIZE_8BIT, read_buf, to_read, 200);

            if(ret != HAL_OK) {
                LOG_ERROR("GPS", "ACK read fail");
                return -1;
            }

            // Debug: Show first bytes of what we read (verbose level hex dump)
            GPS_LOG_DEBUG("Read %d bytes (first 16)", to_read);

            // Search for UBX ACK message in received data
            for(uint16_t i = 0; i < to_read - 9; i++) {  // ACK message is 10 bytes
                // Look for UBX sync bytes
                if(read_buf[i] == UBX_SYNC_CHAR_1 && read_buf[i+1] == UBX_SYNC_CHAR_2) {
                    // Check if it's ACK class
                    if(read_buf[i+2] == UBX_CLASS_ACK) {
                        uint8_t ackType = read_buf[i+3];  // 0x01=ACK, 0x00=NAK
                        uint16_t length = read_buf[i+4] | (read_buf[i+5] << 8);

                        // ACK payload is 2 bytes: [clsID] [msgID]
                        if(length == 2) {
                            uint8_t ack_class = read_buf[i+6];
                            uint8_t ack_id = read_buf[i+7];

                            // Verify checksum
                            uint8_t ckA, ckB;
                            extern void v_UBX_CalcChecksum(uint8_t* pu8_data, uint16_t u16_len,
                                                           uint8_t* pu8_ckA, uint8_t* pu8_ckB);
                            v_UBX_CalcChecksum(&read_buf[i+2], 6, &ckA, &ckB);

                            if(read_buf[i+8] == ckA && read_buf[i+9] == ckB) {
                                // Valid checksum - verify it's for our message
                                if(ack_class == msgClass && ack_id == msgID) {
                                    if(ackType == UBX_ACK_ACK) {
                                        LOG_INFO("GPS", "ACK-ACK received!");
                                        return 1;  // ACK
                                    } else if(ackType == UBX_ACK_NAK) {
                                        LOG_WARN("GPS", "ACK-NAK received (GPS rejected command)");
                                        return 0;  // NAK
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        HAL_Delay(10);  // Short delay before next check
    }

    LOG_WARN("GPS", "ACK timeout (%lu ms)", timeout_ms);
    return -1;  // Timeout
}

/*
 * Helper: Drain any pending data from GPS module
 * Returns: 0 if successful, -1 if error
 */
static int i_GPS_DrainPendingData(I2C_HandleTypeDef* hi2c, uint8_t addr) {
    uint8_t avail_buf[2];
    uint8_t dummy_buf[128];  // Buffer for draining data
    int drain_attempts = 0;

#if IWDG_USED
    extern IWDG_HandleTypeDef hiwdg1;
#endif

    GPS_LOG_DEBUG("Draining...");

    // CRITICAL FIX: Limit attempts to 3 to prevent watchdog timeout
    // 3 attempts × 200ms = 600ms max (safe for 2s watchdog)
    while(drain_attempts < 3) {
#if IWDG_USED
        HAL_IWDG_Refresh(&hiwdg1);
#endif

        // Read available bytes registers (0xFD, 0xFE)
        HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(hi2c, addr, SAM_M10Q_REG_AVAIL_MSB,
                                                  I2C_MEMADD_SIZE_8BIT, avail_buf, 2, 100);

        if(ret != HAL_OK) {
            LOG_WARN("GPS", "Drain failed (0x%02lX) - GPS may not be connected", hi2c->ErrorCode);
            // Don't retry if GPS not responding - save watchdog time
            return -1;
        }

        // Parse available bytes (big-endian)
        uint16_t avail_bytes = (avail_buf[0] << 8) | avail_buf[1];

        if(avail_bytes == 0 || avail_bytes == 0xFFFF) {
            GPS_LOG_DEBUG("Drain OK (avail=0x%04X)", avail_bytes);
            return 0;  // Success
        }

        GPS_LOG_DEBUG("Drain - %d bytes avail", avail_bytes);

        // Read and discard data
        uint16_t to_read = (avail_bytes > sizeof(dummy_buf)) ? sizeof(dummy_buf) : avail_bytes;
        ret = HAL_I2C_Mem_Read(hi2c, addr, SAM_M10Q_REG_STREAM,
                               I2C_MEMADD_SIZE_8BIT, dummy_buf, to_read, 200);

        if(ret != HAL_OK) {
            LOG_ERROR("GPS", "Drain read fail");
            return -1;
        }

        HAL_Delay(50);  // Short delay
        drain_attempts++;
    }

    LOG_WARN("GPS", "Drain - max attempts");
    return -1;
}

void v_GPS_Init(void) {
    LOG_INFO("GPS", "*** GPS INIT START (FW v2025-01-14 + SPARKFUN FIX) ***");

    // STEP 1: Verify I2C3 is ready and ensure interrupts are enabled
    // I2C3 is already initialized in main.c during boot - no need to reset
    extern I2C_HandleTypeDef hi2c3;

    GPS_LOG_DEBUG("I2C3 State=0x%02X (0x20=READY)", hi2c3.State);

    // CRITICAL: Ensure I2C3 interrupts are enabled for interrupt mode operation
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
    GPS_LOG_DEBUG("I2C3 interrupts verified and enabled");

    // Initialize GPS communication state
    e_gps_comm = COMM_STAT_READY;
    v_I2C3_Set_Comm_Ready();
    GPS_LOG_DEBUG("I2C3 ready for GPS communication");

    // Setup transport layer
    _x_SAM_M10Q_TRANS_t trans;
    trans.i_write = i_GPS_Write;
    trans.i_read = i_GPS_Read;
    trans.i_bus = i_GPS_Bus;
    trans.u32_getTime = u32_GPS_GetTime;

    // Initialize driver
    int ret = i_SAM_M10Q_Init(px_gps, &trans, SAM_M10Q_I2C_ADDR_DEFAULT);
    e_gps_init = COMM_STAT_READY;

    if(ret == SAM_M10Q_RET_OK) {
        LOG_INFO("GPS", "Driver initialized (I2C3, addr=0x%02X)", SAM_M10Q_I2C_ADDR_DEFAULT);
    } else {
        LOG_ERROR("GPS", "Init failed (ret=%d)", ret);
    }

    // STEP 2: No additional delay needed - main.c already waited 2 seconds after 12V enable
    LOG_INFO("GPS", "Starting config (12V already enabled, 2s boot complete)");

    // STEP 3: Send CFG-VALSET to disable NMEA (M10 generation uses CFG-VALSET, not CFG-PRT!)
    uint8_t cfg_buf[64];  // Reduced size - only 2 config keys now
    extern int i_UBX_CreateCfgI2C(uint8_t* pu8_buf, uint16_t u16_bufSize,
                                   uint8_t u8_layer, uint8_t u8_i2cAddr, bool b_enableNMEA);
    int cfg_len = i_UBX_CreateCfgI2C(cfg_buf, sizeof(cfg_buf),
                                      0x01,  // RAM only (SparkFun method)
                                      SAM_M10Q_I2C_ADDR_DEFAULT,
                                      false);  // Disable NMEA

    if(cfg_len < 0) {
        LOG_ERROR("GPS", "Failed to create CFG-VALSET message");
        e_gps_comm = COMM_STAT_READY;
        return;
    }

    LOG_INFO("GPS", "Sending CFG-VALSET (%d bytes) to disable NMEA", cfg_len);

    // Hex dump first 32 bytes for verification
    GPS_LOG_DEBUG("CFG-VALSET created (%d bytes)", cfg_len);

    int cfg_attempts = 0;
    int cfg_ack_status = -1;  // -1=not tried, 0=NAK, 1=ACK

    // Up to 3 attempts with ACK verification
    while(cfg_attempts < 3 && cfg_ack_status != 1) {
#if IWDG_USED
        HAL_IWDG_Refresh(&hiwdg1);
#endif

        GPS_LOG_DEBUG("CFG-VALSET attempt %d", cfg_attempts + 1);

        // Drain before EVERY attempt (GPS continuously outputs NMEA)
        int drain_ret = i_GPS_DrainPendingData(&hi2c3, ADDR_GPS);
        if(drain_ret != 0) {
            if(cfg_attempts == 0) {
                // First attempt drain failed - GPS not connected
                LOG_WARN("GPS", "No GPS detected - skipping config");
                e_gps_comm = COMM_STAT_READY;
                return;  // Exit early - no GPS present
            } else {
                // Retry drain failure - try sending anyway
                LOG_WARN("GPS", "Drain failed on retry %d - trying write anyway", cfg_attempts + 1);
            }
        }

        // Send CFG-VALSET immediately after drain
        HAL_StatusTypeDef write_ret = HAL_I2C_Mem_Write(&hi2c3, ADDR_GPS, SAM_M10Q_REG_STREAM,
                                                         I2C_MEMADD_SIZE_8BIT, cfg_buf, cfg_len, 500);

        if(write_ret != HAL_OK) {
            LOG_ERROR("GPS", "CFG-VALSET write fail (0x%02lX)", hi2c3.ErrorCode);
            cfg_attempts++;
            continue;
        }

        GPS_LOG_DEBUG("CFG-VALSET sent, waiting for ACK...");

        // Wait for ACK-ACK response (Class=0x06, ID=0x8A for CFG-VALSET)
        cfg_ack_status = i_GPS_WaitForACK(&hi2c3, ADDR_GPS, 0x06, 0x8A, 500);

        if(cfg_ack_status == 1) {
            // ACK received - GPS accepted the command
            LOG_INFO("GPS", "CFG-VALSET accepted! NMEA disabled, UBX only");

            // CRITICAL: Wait 500ms for GNSS subsystem restart
            // Per u-blox documentation: "Any change to the signal configuration items triggers
            // a restart of the GNSS subsystem. This takes a short time period, so the host
            // application should wait for message acknowledgement and a margin of 0.5 seconds
            // prior to sending any further commands."
            GPS_LOG_DEBUG("Waiting 500ms for GNSS subsystem restart...");
            HAL_Delay(500);
#if IWDG_USED
            HAL_IWDG_Refresh(&hiwdg1);
#endif
            LOG_INFO("GPS", "GNSS subsystem ready");

            break;
        } else if(cfg_ack_status == 0) {
            // NAK received - GPS rejected the command
            LOG_WARN("GPS", "CFG-VALSET rejected (NAK) - retrying");
        } else {
            // Timeout or error
            LOG_WARN("GPS", "CFG-VALSET ACK timeout - retrying");
        }

        cfg_attempts++;
    }

    // STEP 4: Configuration complete (RAM layer - no CFG-SAVE needed)
    // SparkFun method: RAM-only config, no flash save required
    if(cfg_ack_status == 1) {
        LOG_INFO("GPS", "Config complete (RAM layer active - will reset on power cycle)");
    } else {
        LOG_ERROR("GPS", "Config failed after 3 attempts");
    }

    // Configuration complete - reset communication state
    e_gps_comm = COMM_STAT_READY;

    LOG_INFO("GPS", "Init complete - handler will start polling");
}

/*
 * brief: Deinitialize GPS module
 */
void v_GPS_Deinit(void) {
    e_gps_init = COMM_STAT_READY;
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
            GPS_LOG_DEBUG("Available bytes=%d", px_gps->u16_availBytes);
        } else {
            // FIX: Add buffer overflow protection
            if(u16_len <= sizeof(px_gps->u8_rxBuf)) {
                memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);
                px_gps->u16_rxLen = u16_len;
                GPS_LOG_DEBUG("Received %d bytes", u16_len);
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
 * brief: Initialization state machine
 */
e_COMM_STAT_t e_GPS_Ready(void) {
    // Simple initialization - driver handles protocol
    if(e_gps_init == COMM_STAT_READY) {
        GPS_LOG_DEBUG("Ready - handler will detect device");
        e_gps_init = COMM_STAT_DONE;
    }
    return e_gps_init;
}

/*
 * brief: Main processing handler
 */
void v_GPS_Handler(void) {
#if IWDG_USED
    // CRITICAL: Refresh watchdog during GPS processing
    // GPS I2C communication can take significant time (UBX protocol)
    extern IWDG_HandleTypeDef hiwdg1;
    HAL_IWDG_Refresh(&hiwdg1);
#endif

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
            GPS_LOG_DEBUG("Bus: DONE → READY (returning 0)");
            last_debug_log = now;
        }
        e_gps_comm = COMM_STAT_READY;
        return 0;  // Transaction completed, ready for next operation
    }

    // Check if GPS communication channel is ready
    if(e_gps_comm == COMM_STAT_READY) {
        if(should_log) {
            GPS_LOG_DEBUG("Bus: READY (returning 0)");
            last_debug_log = now;
        }
        return 0;  // Bus is ready for next operation
    }

    // Bus is busy (COMM_STAT_BUSY or COMM_STAT_OK - waiting for callback)
    if(should_log) {
        GPS_LOG_DEBUG("Bus: BUSY (e_gps_comm=%d, returning -1)", e_gps_comm);
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
