/*
 * sam_m10q_driver.c
 *
 * SAM-M10Q GPS Module - Core Driver Implementation
 * State machine and driver logic
 *
 * Created: 2025-01-11
 */

#include "sam_m10q_driver.h"
#include <string.h>

/*
 * brief: Initialize SAM-M10Q driver instance
 * return: SAM_M10Q_RET_OK if successful
 */
int i_SAM_M10Q_Init(_x_SAM_M10Q_DRV_t* px_drv,
                     _x_SAM_M10Q_TRANS_t* px_trans,
                     uint8_t u8_i2cAddr) {
    if(!px_drv || !px_trans) return SAM_M10Q_RET_ERR_ARG;

    memset(px_drv, 0, sizeof(_x_SAM_M10Q_DRV_t));

    px_drv->tr = *px_trans;
    px_drv->u8_i2cAddr = u8_i2cAddr;
    px_drv->e_state = SAM_M10Q_STATE_IDLE;
    px_drv->u32_pollInterval = 1000;  // 1 Hz default
    px_drv->b_pvtValid = false;

    return SAM_M10Q_RET_OK;
}

/*
 * brief: Main driver handler - state machine
 * note: Call periodically from main loop or mode handler
 * return: SAM_M10Q_RET_OK if successful
 */
int i_SAM_M10Q_Handler(_x_SAM_M10Q_DRV_t* px_drv) {
    if(!px_drv) return SAM_M10Q_RET_ERR_ARG;

    uint32_t currentTime = px_drv->tr.u32_getTime();
    static _e_SAM_M10Q_STATE_t prev_state = SAM_M10Q_STATE_IDLE;

    // Debug: Print state changes
    if(px_drv->e_state != prev_state && px_drv->e_state != SAM_M10Q_STATE_IDLE) {
        extern void v_printf_poll(const char* format, ...);
        const char* state_names[] = {"IDLE", "CHECK_AVAIL", "WAIT_AVAIL", "READ_DATA",
                                     "WAIT_DATA", "PARSE", "POLL_PVT", "WAIT_POLL", "ERROR"};
        if(px_drv->e_state < 9) {
            v_printf_poll("GPS State: %s\r\n", state_names[px_drv->e_state]);
        }
        prev_state = px_drv->e_state;
    }

    switch(px_drv->e_state) {
        case SAM_M10Q_STATE_IDLE:
            // Check if poll interval elapsed
            if((currentTime - px_drv->u32_lastUpdate) >= px_drv->u32_pollInterval) {
                px_drv->e_state = SAM_M10Q_STATE_CHECK_AVAIL;
            }
            break;

        case SAM_M10Q_STATE_CHECK_AVAIL:
            // Read available byte count from registers 0xFD, 0xFE
            if(px_drv->tr.i_bus() == 0) {  // Check if bus is ready
                int read_ret = px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_AVAIL_MSB, 2);
                if(read_ret == 0) {
                    px_drv->e_state = SAM_M10Q_STATE_WAIT_AVAIL;
                } else {
                    // I2C read failed, stay in this state
                }
            }
            break;

        case SAM_M10Q_STATE_WAIT_AVAIL:
            // Wait for I2C read callback to complete
            if(px_drv->tr.i_bus() == 0) {  // Callback finished, bus is ready
                px_drv->e_state = SAM_M10Q_STATE_READ_DATA;
            }
            break;

        case SAM_M10Q_STATE_READ_DATA:
            // After I2C read complete (callback sets u16_availBytes):
            if(px_drv->u16_availBytes > 0) {
                // Read data from stream register (0xFF)
                uint16_t readLen = (px_drv->u16_availBytes < sizeof(px_drv->u8_rxBuf)) ?
                                    px_drv->u16_availBytes : sizeof(px_drv->u8_rxBuf);

                if(px_drv->tr.i_bus() == 0) {
                    if(px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_STREAM, readLen) == 0) {
                        px_drv->e_state = SAM_M10Q_STATE_WAIT_DATA;
                    }
                }
            } else {
                // No data available, poll for PVT
                px_drv->e_state = SAM_M10Q_STATE_POLL_PVT;
            }
            break;

        case SAM_M10Q_STATE_WAIT_DATA:
            // Wait for data stream read callback
            if(px_drv->tr.i_bus() == 0) {  // Callback finished
                px_drv->e_state = SAM_M10Q_STATE_PARSE;
            }
            break;

        case SAM_M10Q_STATE_PARSE:
            // Parse received UBX message (callback provides data in u8_rxBuf)
            if(px_drv->u16_rxLen > 0) {
                int validate_ret = i_UBX_ValidateMessage(px_drv->u8_rxBuf, px_drv->u16_rxLen);
                if(validate_ret == SAM_M10Q_RET_OK) {
                    // Check if NAV-PVT message
                    if(px_drv->u8_rxBuf[2] == UBX_CLASS_NAV &&
                       px_drv->u8_rxBuf[3] == UBX_NAV_PVT) {
                        i_UBX_ParsePVT(&px_drv->u8_rxBuf[6], &px_drv->x_pvt);
                        px_drv->b_pvtValid = true;
                        px_drv->u32_lastUpdate = currentTime;
                    }
                }
            }
            px_drv->e_state = SAM_M10Q_STATE_IDLE;
            break;

        case SAM_M10Q_STATE_POLL_PVT:
            // Send poll request for NAV-PVT
            px_drv->u16_txLen = i_UBX_CreatePollPVT(px_drv->u8_txBuf, sizeof(px_drv->u8_txBuf));

            if(px_drv->u16_txLen > 0 && px_drv->tr.i_bus() == 0) {
                if(px_drv->tr.i_write(px_drv->u8_i2cAddr, SAM_M10Q_REG_STREAM,
                                       px_drv->u8_txBuf, px_drv->u16_txLen) == 0) {
                    px_drv->e_state = SAM_M10Q_STATE_WAIT_POLL;
                }
            }
            break;

        case SAM_M10Q_STATE_WAIT_POLL:
            // Wait for poll write callback
            if(px_drv->tr.i_bus() == 0) {  // Callback finished
                px_drv->u32_lastUpdate = currentTime;
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
            }
            break;

        case SAM_M10Q_STATE_ERROR:
            // Error recovery
            px_drv->e_state = SAM_M10Q_STATE_IDLE;
            break;
    }

    return SAM_M10Q_RET_OK;
}

/*
 * brief: Get latest PVT data
 * return: Pointer to PVT structure, or NULL if invalid
 */
_x_GPS_PVT_t* px_SAM_M10Q_GetPVT(_x_SAM_M10Q_DRV_t* px_drv) {
    if(!px_drv) return NULL;
    return &px_drv->x_pvt;
}

/*
 * brief: Check if PVT data is valid
 * return: true if valid, false otherwise
 */
bool b_SAM_M10Q_IsPVTValid(_x_SAM_M10Q_DRV_t* px_drv) {
    if(!px_drv) return false;
    return px_drv->b_pvtValid;
}

/*
 * brief: Reset driver state
 */
void v_SAM_M10Q_Reset(_x_SAM_M10Q_DRV_t* px_drv) {
    if(!px_drv) return;
    px_drv->e_state = SAM_M10Q_STATE_IDLE;
    px_drv->b_pvtValid = false;
}
