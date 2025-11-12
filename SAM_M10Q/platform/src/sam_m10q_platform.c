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

    // Initialize driver
    int ret = i_SAM_M10Q_Init(px_gps, &trans, SAM_M10Q_I2C_ADDR_DEFAULT);

    e_gps_comm = COMM_STAT_READY;
    e_gps_init = COMM_STAT_READY;

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
            v_printf_poll("GPS: Available bytes=%d\r\n", px_gps->u16_availBytes);
        } else {
            // Actual GPS data stream
            if(u16_len < sizeof(px_gps->u8_rxBuf)) {
                memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);
                px_gps->u16_rxLen = u16_len;
                v_printf_poll("GPS: Received %d bytes\r\n", u16_len);
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
    if((e_gps_comm == COMM_STAT_BUSY) &&
       _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)) {
        // Abort I2C transaction
        extern I2C_HandleTypeDef hi2c3;
        HAL_I2C_Master_Abort_IT(&hi2c3, ADDR_GPS);
        e_gps_comm = COMM_STAT_READY;

        // GPS timeout - log warning but don't enter ERROR mode
        // This allows system to continue operating without GPS
        v_printf_poll("GPS: I2C timeout (no response)\r\n");
    }
}

// ========== Transport Layer Functions ==========

static int i_GPS_Write(uint8_t u8_addr, uint16_t u16_reg, uint8_t* pu8_arr, uint16_t u16_len) {
    u32_toutRef = u32_Tim_1msGet();
    e_gps_comm = COMM_STAT_BUSY;
    return i_I2C3_Write(ADDR_GPS, u16_reg, pu8_arr, u16_len);
}

static int i_GPS_Read(uint8_t u8_addr, uint16_t u16_reg, uint16_t u16_len) {
    u32_toutRef = u32_Tim_1msGet();
    e_gps_comm = COMM_STAT_BUSY;
    int ret = i_I2C3_Read(ADDR_GPS, u16_reg, u16_len);
    if(ret != 0) {
        v_printf_poll("GPS: I2C Read failed (reg=0x%02X, len=%d, ret=%d)\r\n", u16_reg, u16_len, ret);
    }
    return ret;
}

static int i_GPS_Bus(void) {
    return (e_gps_comm == COMM_STAT_READY) ? 0 : -1;
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
