/*
 * sam_m10q_driver.h
 *
 * SAM-M10Q GPS Module - Core Driver API
 * Hardware-agnostic driver interface
 *
 * Created: 2025-01-11
 */

#ifndef __JH_SAM_M10Q_DRIVER_H
#define __JH_SAM_M10Q_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sam_m10q_def.h"
#include "sam_m10q_transport.h"
#include "sam_m10q_ubx.h"

/*
 * Driver State Machine
 */
typedef enum {
    SAM_M10Q_STATE_IDLE = 0,
    SAM_M10Q_STATE_CHECK_AVAIL,
    SAM_M10Q_STATE_WAIT_AVAIL,      // Wait for available bytes read
    SAM_M10Q_STATE_READ_DATA,
    SAM_M10Q_STATE_WAIT_DATA,       // Wait for data stream read
    SAM_M10Q_STATE_PARSE,
    SAM_M10Q_STATE_POLL_PVT,
    SAM_M10Q_STATE_WAIT_POLL,       // Wait for poll write
    SAM_M10Q_STATE_ERROR,
} _e_SAM_M10Q_STATE_t;

/*
 * Driver Instance Structure
 */
typedef struct {
    // Transport layer
    _x_SAM_M10Q_TRANS_t tr;

    // State machine
    _e_SAM_M10Q_STATE_t e_state;
    uint8_t u8_i2cAddr;

    // Buffers
    uint8_t u8_rxBuf[256];      // I2C receive buffer
    uint16_t u16_rxLen;
    uint8_t u8_txBuf[16];       // I2C transmit buffer (poll messages)
    uint16_t u16_txLen;

    // UBX message parsing
    uint16_t u16_availBytes;    // Bytes available from GPS
    uint8_t u8_ubxBuf[256];     // UBX message assembly buffer
    uint16_t u16_ubxIdx;

    // GPS data
    _x_GPS_PVT_t x_pvt;         // Latest PVT data
    bool b_pvtValid;            // PVT data valid flag

    // Timing
    uint32_t u32_lastUpdate;    // Last successful update timestamp
    uint32_t u32_pollInterval;  // Poll interval (ms)

} _x_SAM_M10Q_DRV_t;

/*
 * Driver Initialization
 * px_drv: Driver instance
 * px_trans: Transport layer function pointers
 * u8_i2cAddr: I2C address (7-bit)
 * return: SAM_M10Q_RET_OK if successful
 */
int i_SAM_M10Q_Init(_x_SAM_M10Q_DRV_t* px_drv,
                     _x_SAM_M10Q_TRANS_t* px_trans,
                     uint8_t u8_i2cAddr);

/*
 * Main Processing Handler
 * Call periodically from main loop or mode handler
 * px_drv: Driver instance
 * return: SAM_M10Q_RET_OK if successful
 */
int i_SAM_M10Q_Handler(_x_SAM_M10Q_DRV_t* px_drv);

/*
 * Get Latest PVT Data
 * px_drv: Driver instance
 * return: Pointer to PVT structure, or NULL if invalid
 */
_x_GPS_PVT_t* px_SAM_M10Q_GetPVT(_x_SAM_M10Q_DRV_t* px_drv);

/*
 * Check if PVT Data is Valid
 * px_drv: Driver instance
 * return: true if valid, false otherwise
 */
bool b_SAM_M10Q_IsPVTValid(_x_SAM_M10Q_DRV_t* px_drv);

/*
 * Reset Driver State
 * px_drv: Driver instance
 */
void v_SAM_M10Q_Reset(_x_SAM_M10Q_DRV_t* px_drv);

#ifdef __cplusplus
}
#endif

#endif // __JH_SAM_M10Q_DRIVER_H
