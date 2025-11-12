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
#include <string.h>

// I2C address for GPS is defined in i2c.h as ADDR_GPS

// Driver instance
static _x_SAM_M10Q_DRV_t x_gps_drv;
static _x_SAM_M10Q_DRV_t* px_gps = &x_gps_drv;

// Communication state
static volatile e_COMM_STAT_t e_gps_comm;
static uint32_t u32_toutRef;

// Initialization state
static e_COMM_STAT_t e_gps_init;

// Device presence tracking (for runtime disconnection detection)
static uint8_t u8_consecutiveErrors = 0;
static uint32_t u32_lastValidData = 0;
static bool b_devicePresent = false;
static uint8_t u8_consecutiveZeros = 0;  // Count consecutive "available=0" responses

// Forward declarations of transport functions
static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len);
static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len);
static int i_GPS_Bus(void);
static uint32_t u32_GPS_GetTime(void);

/*
 * brief: Initialize GPS module
 */
void v_GPS_Init(void) {
    // Setup transport layer
    _x_SAM_M10Q_TRANS_t trans;
    trans.i_write = i_GPS_Write;
    trans.i_read = i_GPS_Read;
    trans.i_bus = i_GPS_Bus;
    trans.u32_getTime = u32_GPS_GetTime;

    // Initialize driver (no immediate I2C communication)
    int ret = i_SAM_M10Q_Init(px_gps, &trans, SAM_M10Q_I2C_ADDR_DEFAULT);

    e_gps_comm = COMM_STAT_READY;
    e_gps_init = COMM_STAT_READY;

    // Initialize device presence tracking
    u8_consecutiveErrors = 0;
    u8_consecutiveZeros = 0;
    u32_lastValidData = 0;
    b_devicePresent = false;

    if(ret == SAM_M10Q_RET_OK) {
        v_printf_poll("GPS: Driver initialized (I2C3, addr=0x%02X)\r\n", SAM_M10Q_I2C_ADDR_DEFAULT);
    } else {
        v_printf_poll("GPS: Init failed (ret=%d)\r\n", ret);
    }
}

/*
 * brief: Deinitialize GPS module
 */
void v_GPS_Deinit(void) {
    e_gps_init = COMM_STAT_READY;
    e_gps_comm = COMM_STAT_READY;
}

/*
 * brief: I2C write done callback
 */
void v_GPS_Write_DoneHandler(uint8_t u8_addr) {
    if(u8_addr == ADDR_GPS) {
        // Write completed successfully
        e_gps_comm = COMM_STAT_DONE;
        // Reset error counter on successful transaction
        u8_consecutiveErrors = 0;
        u32_lastValidData = u32_Tim_1msGet();
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

            // CRITICAL: Detect pullup garbage (0xFFFF indicates disconnected device)
            if(px_gps->u16_availBytes == 0xFFFF) {
                u8_consecutiveErrors++;
                u8_consecutiveZeros = 0;
                v_printf_poll("GPS: Invalid data 0xFFFF (pullup garbage? err=%d)\r\n", u8_consecutiveErrors);
            } else if(px_gps->u16_availBytes == 0) {
                // 0 bytes available *can* be valid (GPS initializing or no data yet)
                // But if we get 10+ consecutive zeros, device is likely disconnected
                u8_consecutiveZeros++;
                if(u8_consecutiveZeros >= 10) {
                    u8_consecutiveErrors++;
                    v_printf_poll("GPS: Too many zeros (cnt=%d, err=%d) - device disconnected?\r\n",
                                  u8_consecutiveZeros, u8_consecutiveErrors);
                    u8_consecutiveZeros = 0;  // Reset to avoid spam
                }
                // Don't log every zero - too verbose
            } else {
                // Valid byte count (1-65534)
                u8_consecutiveErrors = 0;
                u8_consecutiveZeros = 0;
                u32_lastValidData = u32_Tim_1msGet();
                if(!b_devicePresent) {
                    b_devicePresent = true;
                    v_printf_poll("GPS: Device connected\r\n");
                }
                // Reduce log verbosity - only log when data > 0
                static uint8_t u8_logCounter = 0;
                if(++u8_logCounter >= 20) {  // Log every 20th read with data
                    v_printf_poll("GPS: Available bytes=%d\r\n", px_gps->u16_availBytes);
                    u8_logCounter = 0;
                }
            }
        } else {
            // Actual GPS data stream
            if(u16_len < sizeof(px_gps->u8_rxBuf)) {
                // Check if data is all 0xFF (pullup garbage)
                bool b_allFF = true;
                for(uint16_t i = 0; i < u16_len; i++) {
                    if(pu8_arr[i] != 0xFF) {
                        b_allFF = false;
                        break;
                    }
                }

                if(b_allFF && u16_len > 10) {
                    // Likely pullup garbage (all 0xFF for >10 bytes is suspicious)
                    u8_consecutiveErrors++;
                    v_printf_poll("GPS: Invalid data all-0xFF (pullup garbage? err=%d)\r\n", u8_consecutiveErrors);
                } else {
                    // Valid data
                    memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);
                    px_gps->u16_rxLen = u16_len;
                    u8_consecutiveErrors = 0;
                    u32_lastValidData = u32_Tim_1msGet();
                    if(!b_devicePresent) {
                        b_devicePresent = true;
                        v_printf_poll("GPS: Device connected\r\n");
                    }
                    // Reduce log verbosity - only log occasionally
                    static uint8_t u8_dataLogCounter = 0;
                    if(++u8_dataLogCounter >= 20) {  // Log every 20th data read
                        v_printf_poll("GPS: Received %d bytes\r\n", u16_len);
                        u8_dataLogCounter = 0;
                    }
                }
            }
        }
        e_gps_comm = COMM_STAT_DONE;
    }
}

/*
 * brief: Initialization state machine
 * note: Simplified to avoid I2C bus conflicts with handler
 *       Device detection happens naturally when handler tries to communicate
 */
e_COMM_STAT_t e_GPS_Ready(void) {
    if(e_gps_init == COMM_STAT_READY) {
        v_printf_poll("GPS: Ready - handler will detect device\r\n");
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
 * brief: Timeout handler (2 second timeout)
 */
void v_GPS_Tout_Handler(void) {
    static uint32_t debug_log_ref = 0;
    static bool b_disconnectLogged = false;

    // Debug: Log state periodically
    if(_b_Tim_Is_OVR(u32_Tim_1msGet(), debug_log_ref, 5000)) {
        debug_log_ref = u32_Tim_1msGet();
        v_printf_poll("GPS: ToutHandler called (state=%d, BUSY=%d)\r\n", e_gps_comm, COMM_STAT_BUSY);
    }

    // Check for I2C transaction timeout
    if((e_gps_comm == COMM_STAT_BUSY) &&
       _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)) {
        // Abort I2C transaction
        extern I2C_HandleTypeDef hi2c3;
        HAL_I2C_Master_Abort_IT(&hi2c3, ADDR_GPS);
        e_gps_comm = COMM_STAT_READY;

        // CRITICAL: Reset driver state machine to IDLE
        v_SAM_M10Q_Reset(px_gps);

        // GPS timeout - log warning but don't enter ERROR mode
        // This allows system to continue operating without GPS
        v_printf_poll("GPS: I2C timeout - driver and bus reset\r\n");
        u8_consecutiveErrors++;
    }

    // CRITICAL: Device presence detection (runtime disconnection)
    // If 3+ consecutive errors OR no valid data for 5+ seconds → device disconnected
    if(u8_consecutiveErrors >= 3 ||
       (b_devicePresent && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_lastValidData, 5000))) {

        if(b_devicePresent) {
            // Device was present, now disconnected
            b_devicePresent = false;
            b_disconnectLogged = false;  // Allow new disconnect log
            v_printf_poll("GPS: Device disconnected (err=%d)\r\n", u8_consecutiveErrors);
        } else if(!b_disconnectLogged && u8_consecutiveErrors >= 3) {
            // Device never connected or still disconnected
            v_printf_poll("GPS: Device not detected (err=%d)\r\n", u8_consecutiveErrors);
            b_disconnectLogged = true;
        }
    } else if(b_devicePresent && !b_disconnectLogged) {
        // Device is connected and working
        b_disconnectLogged = false;
    }
}

// ========== Transport Layer Functions ==========

static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len) {
    // Start transaction - set timeout reference
    u32_toutRef = u32_Tim_1msGet();

    // Attempt I2C write
    int ret = i_I2C3_Write(ADDR_GPS, u16_reg, pu8_arr, u16_len);

    // Convert: COMM_STAT_OK (1) → 0 (success), others → -1 (error)
    if(ret != COMM_STAT_OK) {
        // I2C write failed to start - keep state READY
        e_gps_comm = COMM_STAT_READY;
        return -1;
    }

    // I2C IT started successfully - set BUSY state
    // Will be changed to DONE by callback
    e_gps_comm = COMM_STAT_BUSY;
    return 0;  // Success
}

static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len) {
    static uint32_t read_count = 0;
    read_count++;

    // Start transaction - set timeout reference
    u32_toutRef = u32_Tim_1msGet();

    // Attempt I2C read
    int ret = i_I2C3_Read(ADDR_GPS, u16_reg, u16_len);

    // Convert: COMM_STAT_OK (1) → 0 (success), others → -1 (error)
    if(ret != COMM_STAT_OK) {
        // I2C read failed to start - keep state READY
        e_gps_comm = COMM_STAT_READY;

        // Log specific error types with detailed I2C state
        extern I2C_HandleTypeDef hi2c3;
        uint32_t i2c_error = HAL_I2C_GetError(&hi2c3);
        HAL_I2C_StateTypeDef i2c_state = HAL_I2C_GetState(&hi2c3);

        if(ret == COMM_STAT_ERR) {
            v_printf_poll("GPS_I2C: Read start FAILED (HAL_err=0x%04X, HAL_state=%d)\r\n",
                          i2c_error, i2c_state);
            // Decode common HAL errors
            if(i2c_error & HAL_I2C_ERROR_AF) {
                v_printf_poll("  -> NACK (device not responding)\r\n");
            }
            if(i2c_error & HAL_I2C_ERROR_TIMEOUT) {
                v_printf_poll("  -> Timeout\r\n");
            }
        } else if(ret == COMM_STAT_ERR_LEN) {
            v_printf_poll("GPS_I2C: Buffer too small (len=%d > max)\r\n", u16_len);
        } else if(ret == COMM_STAT_BUSY) {
            v_printf_poll("GPS_I2C: Bus BUSY (HAL_state=%d, err=0x%04X)\r\n",
                          i2c_state, i2c_error);
        } else {
            v_printf_poll("GPS_I2C: Unknown error (ret=%d, HAL_err=0x%04X)\r\n",
                          ret, i2c_error);
        }
        return -1;
    }

    // I2C IT started successfully - set BUSY state
    // Will be changed to DONE by callback
    e_gps_comm = COMM_STAT_BUSY;

    // Debug: Log every 50th read (reduced verbosity)
    if((read_count % 50) == 0) {
        v_printf_poll("GPS: Read #%d started\r\n", read_count);
    }

    return 0;  // Success
}

static int i_GPS_Bus(void) {
    // Bus state check: READY or DONE means available for new transaction
    switch(e_gps_comm) {
        case COMM_STAT_READY:
            return 0;  // Bus ready

        case COMM_STAT_DONE:
            // Callback completed - consume DONE state and mark ready
            e_gps_comm = COMM_STAT_READY;
            return 0;  // Bus ready

        case COMM_STAT_BUSY:
            // Transaction in progress
            return -1;  // Bus busy

        case COMM_STAT_OK:
            // I2C IT started but callback not yet called
            // This is a transient state - treat as busy
            return -1;  // Bus busy

        default:
            // Error states - reset to READY for recovery
            e_gps_comm = COMM_STAT_READY;
            return 0;  // Allow retry
    }
}

static uint32_t u32_GPS_GetTime(void) {
    return u32_Tim_1msGet();
}

// ========== Application Data Access Functions ==========

bool b_GPS_IsConnected(void) {
    return b_devicePresent;
}

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
            // Using v_printf_poll from uart.h
            v_printf_poll("GPS: Lat=%.6f Lon=%.6f Alt=%.1fm Sats=%d Fix=%d\r\n",
                          f_GPS_GetLatitude(),
                          f_GPS_GetLongitude(),
                          f_GPS_GetAltitude(),
                          u8_GPS_GetNumSatellites(),
                          u8_GPS_GetFixType());
        } else {
            v_printf_poll("GPS: No fix (Sats=%d)\r\n", u8_GPS_GetNumSatellites());
        }
    }
}
