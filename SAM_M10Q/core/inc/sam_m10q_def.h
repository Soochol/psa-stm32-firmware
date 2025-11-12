/*
 * sam_m10q_def.h
 *
 * SAM-M10Q GPS Module - Core Definitions
 * Hardware-agnostic constants, enums, and data structures
 *
 * Created: 2025-01-11
 */

#ifndef __JH_SAM_M10Q_DEF_H
#define __JH_SAM_M10Q_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*
 * I2C Register Addresses
 */
#define SAM_M10Q_I2C_ADDR_DEFAULT    0x42  // 7-bit address
#define SAM_M10Q_REG_STREAM          0xFF  // Data stream register
#define SAM_M10Q_REG_AVAIL_MSB       0xFD  // Available bytes MSB
#define SAM_M10Q_REG_AVAIL_LSB       0xFE  // Available bytes LSB

/*
 * UBX Protocol Constants
 */
#define UBX_SYNC_CHAR_1              0xB5
#define UBX_SYNC_CHAR_2              0x62
#define UBX_MAX_PAYLOAD_SIZE         512   // Maximum UBX payload

// UBX Message Classes
#define UBX_CLASS_NAV                0x01  // Navigation results
#define UBX_CLASS_CFG                0x06  // Configuration input
#define UBX_CLASS_RXM                0x02  // Receiver manager

// UBX Message IDs (Class NAV)
#define UBX_NAV_PVT                  0x07  // Position, Velocity, Time
#define UBX_NAV_STATUS               0x03  // Receiver navigation status
#define UBX_NAV_SAT                  0x35  // Satellite information

// UBX Message IDs (Class CFG)
#define UBX_CFG_VALSET               0x8A  // Set configuration values
#define UBX_CFG_VALGET               0x8B  // Get configuration values

/*
 * GPS Fix Types
 */
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_DEAD_RECKONING = 1,
    GPS_FIX_2D = 2,
    GPS_FIX_3D = 3,
    GPS_FIX_GNSS_DEAD_RECKONING = 4,
    GPS_FIX_TIME_ONLY = 5
} _e_GPS_FIX_TYPE_t;

/*
 * Validity Flags (from UBX-NAV-PVT valid field)
 */
#define GPS_VALID_DATE               0x01  // Valid UTC date
#define GPS_VALID_TIME               0x02  // Valid UTC time
#define GPS_VALID_FULLY_RESOLVED     0x04  // UTC time fully resolved

/*
 * Return Codes
 */
typedef enum {
    SAM_M10Q_RET_OK = 0,
    SAM_M10Q_RET_BUSY,
    SAM_M10Q_RET_NO_DATA,
    SAM_M10Q_RET_CHECKSUM_ERR,
    SAM_M10Q_RET_ERR_SIZE,
    SAM_M10Q_RET_ERR_ARG,
    SAM_M10Q_RET_ERR_TIMEOUT,
    SAM_M10Q_RET_ERR_I2C,
} _e_SAM_M10Q_RET_t;

/*
 * GPS Position Data Structure (simplified from UBX-NAV-PVT)
 */
typedef struct {
    // Time (UTC)
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  valid;        // Validity flags

    // Position
    int32_t  lon;          // Longitude (1e-7 degrees)
    int32_t  lat;          // Latitude (1e-7 degrees)
    int32_t  height;       // Height above ellipsoid (mm)
    int32_t  hMSL;         // Height above mean sea level (mm)

    // Accuracy
    uint32_t hAcc;         // Horizontal accuracy estimate (mm)
    uint32_t vAcc;         // Vertical accuracy estimate (mm)

    // Status
    uint8_t  fixType;      // GNSS fix type
    uint8_t  numSV;        // Number of satellites used
    uint16_t pDOP;         // Position DOP (0.01)

    // Velocity
    int32_t  gSpeed;       // Ground speed (mm/s)
    int32_t  headMot;      // Heading of motion (1e-5 degrees)

    // Timestamp
    uint32_t iTOW;         // GPS time of week (ms)
} _x_GPS_PVT_t;

#ifdef __cplusplus
}
#endif

#endif // __JH_SAM_M10Q_DEF_H
