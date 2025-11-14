/*
 * sam_m10q_platform.h
 *
 * SAM-M10Q GPS Module - STM32 Platform API
 * Application interface for GPS module
 *
 * Created: 2025-01-11
 */

#ifndef __JH_SAM_M10Q_PLATFORM_H
#define __JH_SAM_M10Q_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sam_m10q_driver.h"
#include "lib_def.h"  // For e_COMM_STAT_t

/*
 * Platform API
 */

// Initialization
void v_GPS_Init(void);
void v_GPS_Deinit(void);

// I2C callbacks (called from i2c.c interrupt handlers)
void v_GPS_Write_DoneHandler(uint8_t u8_addr);
void v_GPS_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len);

// Communication status
e_COMM_STAT_t e_GPS_Ready(void);
void v_GPS_Reset_Comm(void);  // Reset communication state to READY

// Main processing
void v_GPS_Handler(void);
void v_GPS_Tout_Handler(void);

/*
 * Data Access Functions (application-friendly)
 */

// Status
bool b_GPS_HasFix(void);
uint8_t u8_GPS_GetFixType(void);
uint8_t u8_GPS_GetNumSatellites(void);

// Position (returns in degrees, altitude in meters)
float f_GPS_GetLatitude(void);      // Degrees
float f_GPS_GetLongitude(void);     // Degrees
float f_GPS_GetAltitude(void);      // Meters above sea level
float f_GPS_GetSpeed(void);         // m/s
float f_GPS_GetHeading(void);       // Degrees

// Accuracy
float f_GPS_GetHorizontalAccuracy(void);  // Meters
float f_GPS_GetVerticalAccuracy(void);    // Meters

// Time (UTC)
void v_GPS_GetDateTime(uint16_t* pu16_year, uint8_t* pu8_month, uint8_t* pu8_day,
                       uint8_t* pu8_hour, uint8_t* pu8_min, uint8_t* pu8_sec);
bool b_GPS_IsTimeValid(void);

// Raw PVT access
_x_GPS_PVT_t* px_GPS_GetPVT(void);

// Test function
void v_GPS_Test(void);

#ifdef __cplusplus
}
#endif

#endif // __JH_SAM_M10Q_PLATFORM_H
