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

// External debug printf function
extern void v_printf_poll(const char* format, ...);

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

    // Debug: Log state transitions (every 100 calls to reduce spam)
    static uint32_t debug_call_count = 0;
    static _e_SAM_M10Q_STATE_t prev_state = SAM_M10Q_STATE_IDLE;
    debug_call_count++;

    if(prev_state != px_drv->e_state && (debug_call_count % 10) == 0) {
        v_printf_poll("GPS_DRV: State %d->%d (calls=%lu)\r\n",
                      prev_state, px_drv->e_state, debug_call_count);
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
            {
                int bus_ready = px_drv->tr.i_bus();
                if(bus_ready == 0) {  // Check if bus is ready
                    int read_ret = px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_AVAIL_MSB, 2);
                    if(read_ret == 0) {
                        px_drv->e_state = SAM_M10Q_STATE_WAIT_AVAIL;
                        px_drv->u32_stateEnterTime = currentTime;  // Record state entry time
                    }
                    // I2C read failed, stay in this state
                }
                // Bus busy, stay in this state
            }
            break;

        case SAM_M10Q_STATE_WAIT_AVAIL:
            // Wait for I2C read callback to complete
            // Timeout check (2 seconds)
            if((currentTime - px_drv->u32_stateEnterTime) > 2000) {
                v_printf_poll("GPS: WAIT_AVAIL timeout, reset to IDLE\r\n");
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
                break;
            }

            if(px_drv->tr.i_bus() == 0) {  // Callback finished, bus is ready
                px_drv->e_state = SAM_M10Q_STATE_READ_DATA;
            }
            break;

        case SAM_M10Q_STATE_READ_DATA:
            // After I2C read complete (callback sets u16_availBytes):
            if(px_drv->u16_availBytes > 0) {
                // Read data from stream register (0xFF)
                // CRITICAL: Limit to 64 bytes to match I2C3 buffer size
                uint16_t readLen = px_drv->u16_availBytes;
                if(readLen > 64) readLen = 64;  // I2C3_RD_SIZE limit
                if(readLen > sizeof(px_drv->u8_rxBuf)) readLen = sizeof(px_drv->u8_rxBuf);

                if(px_drv->tr.i_bus() == 0) {
                    if(px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_STREAM, readLen) == 0) {
                        px_drv->e_state = SAM_M10Q_STATE_WAIT_DATA;
                        px_drv->u32_stateEnterTime = currentTime;  // Record state entry time
                    }
                }
            } else {
                // No data available, poll for PVT
                px_drv->e_state = SAM_M10Q_STATE_POLL_PVT;
            }
            break;

        case SAM_M10Q_STATE_WAIT_DATA:
            // Wait for data stream read callback
            // Timeout check (2 seconds)
            if((currentTime - px_drv->u32_stateEnterTime) > 2000) {
                v_printf_poll("GPS: WAIT_DATA timeout, reset to IDLE\r\n");
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
                break;
            }

            if(px_drv->tr.i_bus() == 0) {  // Callback finished
                px_drv->e_state = SAM_M10Q_STATE_PARSE;
            }
            break;

        case SAM_M10Q_STATE_PARSE:
            // Accumulate received data into UBX buffer
            if(px_drv->u16_rxLen > 0) {
                // Check buffer overflow
                if(px_drv->u16_ubxIdx + px_drv->u16_rxLen > sizeof(px_drv->u8_ubxBuf)) {
                    // Buffer overflow - reset and discard
                    v_printf_poll("GPS: UBX buffer overflow, reset\r\n");
                    px_drv->u16_ubxIdx = 0;
                    px_drv->e_state = SAM_M10Q_STATE_IDLE;
                    break;
                }

                // Append received data to UBX accumulation buffer
                memcpy(&px_drv->u8_ubxBuf[px_drv->u16_ubxIdx],
                       px_drv->u8_rxBuf, px_drv->u16_rxLen);
                px_drv->u16_ubxIdx += px_drv->u16_rxLen;

                // Search for UBX header (0xB5 0x62)
                bool messageFound = false;
                for(uint16_t i = 0; i < px_drv->u16_ubxIdx - 1; i++) {
                    if(px_drv->u8_ubxBuf[i] == UBX_SYNC_CHAR_1 &&
                       px_drv->u8_ubxBuf[i+1] == UBX_SYNC_CHAR_2) {

                        // Found header - check if complete message available
                        if((i + 6) <= px_drv->u16_ubxIdx) {
                            // Extract payload length (little-endian at offset 4-5)
                            uint16_t payloadLen = px_drv->u8_ubxBuf[i+4] |
                                                 (px_drv->u8_ubxBuf[i+5] << 8);
                            uint16_t totalLen = 6 + payloadLen + 2;  // Header + Payload + Checksum

                            if((i + totalLen) <= px_drv->u16_ubxIdx) {
                                // Complete message received!
                                int validate_ret = i_UBX_ValidateMessage(&px_drv->u8_ubxBuf[i], totalLen);
                                if(validate_ret == SAM_M10Q_RET_OK) {
                                    // Check if NAV-PVT message
                                    if(px_drv->u8_ubxBuf[i+2] == UBX_CLASS_NAV &&
                                       px_drv->u8_ubxBuf[i+3] == UBX_NAV_PVT) {
                                        i_UBX_ParsePVT(&px_drv->u8_ubxBuf[i+6], &px_drv->x_pvt);
                                        px_drv->b_pvtValid = true;
                                        px_drv->u32_lastUpdate = currentTime;
                                        v_printf_poll("GPS: PVT message parsed successfully\r\n");
                                    }
                                }
                                // Clear buffer after processing
                                px_drv->u16_ubxIdx = 0;
                                messageFound = true;
                                break;
                            } else {
                                // Incomplete message - need more data
                                // Shift incomplete message to start of buffer
                                if(i > 0) {
                                    uint16_t remaining = px_drv->u16_ubxIdx - i;
                                    memmove(px_drv->u8_ubxBuf, &px_drv->u8_ubxBuf[i], remaining);
                                    px_drv->u16_ubxIdx = remaining;
                                }
                                // Continue reading
                                px_drv->e_state = SAM_M10Q_STATE_CHECK_AVAIL;
                                return SAM_M10Q_RET_OK;
                            }
                        }
                    }
                }

                // No valid message found or message processed
                if(messageFound) {
                    px_drv->e_state = SAM_M10Q_STATE_IDLE;
                } else {
                    // No header found yet - might need more data or garbage
                    if(px_drv->u16_ubxIdx > 100) {
                        // Too much data without header - discard and restart
                        v_printf_poll("GPS: No UBX header found in %d bytes, reset\r\n", px_drv->u16_ubxIdx);
                        px_drv->u16_ubxIdx = 0;
                    }
                    px_drv->e_state = SAM_M10Q_STATE_IDLE;
                }
            } else {
                // No data received
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
            }
            break;

        case SAM_M10Q_STATE_POLL_PVT:
            // Send poll request for NAV-PVT
            px_drv->u16_txLen = i_UBX_CreatePollPVT(px_drv->u8_txBuf, sizeof(px_drv->u8_txBuf));

            if(px_drv->u16_txLen > 0 && px_drv->tr.i_bus() == 0) {
                if(px_drv->tr.i_write(px_drv->u8_i2cAddr, SAM_M10Q_REG_STREAM,
                                       px_drv->u8_txBuf, px_drv->u16_txLen) == 0) {
                    px_drv->e_state = SAM_M10Q_STATE_WAIT_POLL;
                    px_drv->u32_stateEnterTime = currentTime;  // Record state entry time
                }
            }
            break;

        case SAM_M10Q_STATE_WAIT_POLL:
            // Wait for poll write callback
            // Timeout check (2 seconds)
            if((currentTime - px_drv->u32_stateEnterTime) > 2000) {
                v_printf_poll("GPS: WAIT_POLL timeout, reset to IDLE\r\n");
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
                break;
            }

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
