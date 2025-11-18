/*
 * sam_m10q_driver.c
 *
 * SAM-M10Q GPS Module - Core Driver Implementation
 * State machine and driver logic
 *
 * Created: 2025-01-11
 */

#include "sam_m10q_driver.h"
#include "lib_log.h"
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

    // Debug: Print state changes (including IDLE transitions)
    if(px_drv->e_state != prev_state) {
        const char* state_names[] = {"IDLE", "CHECK_AVAIL", "WAIT_AVAIL", "READ_DATA",
                                     "WAIT_DATA", "PARSE", "POLL_PVT", "WAIT_POLL", "ERROR"};
        if(px_drv->e_state < 9) {
            LOG_DEBUG("GPS", "State: %s", state_names[px_drv->e_state]);
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
            {
                int bus_status = px_drv->tr.i_bus();
                if(bus_status == 0) {  // Check if bus is ready
                    int read_ret = px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_AVAIL_MSB, 2);
                    if(read_ret == 0) {
                        px_drv->e_state = SAM_M10Q_STATE_WAIT_AVAIL;
                    } else {
                        LOG_ERROR("GPS", "CHECK_AVAIL - I2C read failed (ret=%d)", read_ret);
                    }
                } else {
                    static uint32_t last_bus_log = 0;
                    if((currentTime - last_bus_log) > 5000) {  // Log every 5s
                        LOG_DEBUG("GPS", "CHECK_AVAIL - Bus not ready (status=%d)", bus_status);
                        last_bus_log = currentTime;
                    }
                }
            }
            break;

        case SAM_M10Q_STATE_WAIT_AVAIL:
            // Wait for I2C read callback to complete
            if(px_drv->tr.i_bus() == 0) {  // Callback finished, bus is ready
                // Data is already set by callback (u16_availBytes)
                px_drv->e_state = SAM_M10Q_STATE_READ_DATA;
            }
            break;

        case SAM_M10Q_STATE_READ_DATA:
            // After I2C read complete (callback sets u16_availBytes):
            if(px_drv->u16_availBytes > 0) {
                // Read data from stream register (0xFF)
                if(px_drv->tr.i_bus() == 0) {
                    uint16_t readLen = (px_drv->u16_availBytes < sizeof(px_drv->u8_rxBuf)) ?
                                        px_drv->u16_availBytes : sizeof(px_drv->u8_rxBuf);

                    LOG_DEBUG("GPS", "Reading %d bytes from stream...", readLen);

                    int read_ret = px_drv->tr.i_read(px_drv->u8_i2cAddr, SAM_M10Q_REG_STREAM, readLen);
                    if(read_ret == 0) {
                        px_drv->u16_availBytes = 0;  // Clear to prevent re-entry
                        px_drv->e_state = SAM_M10Q_STATE_WAIT_DATA;
                    } else {
                        LOG_ERROR("GPS", "Read failed (ret=%d)", read_ret);
                        px_drv->u16_availBytes = 0;  // Clear on error
                        px_drv->e_state = SAM_M10Q_STATE_IDLE;
                    }
                }
                // If bus busy, stay in READ_DATA, will retry next iteration silently
            } else {
                // No data available - return to IDLE and wait for Auto PVT
                // (Auto PVT is enabled via CFG_MSGOUT_UBX_NAV_PVT_I2C=1, no manual polling needed)
                px_drv->e_state = SAM_M10Q_STATE_IDLE;
            }
            break;

        case SAM_M10Q_STATE_WAIT_DATA:
            // Wait for data stream read callback
            if(px_drv->tr.i_bus() == 0) {  // Callback finished
                // FIX: Verify data was received before parsing
                if(px_drv->u16_rxLen > 0) {
                    px_drv->e_state = SAM_M10Q_STATE_PARSE;
                } else {
                    // No data received, return to idle
                    LOG_WARN("GPS", "No data received after read");
                    px_drv->e_state = SAM_M10Q_STATE_IDLE;
                }
            }
            break;

        case SAM_M10Q_STATE_PARSE:
            // Parse received UBX message (callback provides data in u8_rxBuf)
            if(px_drv->u16_rxLen > 0) {
                // Check message type and silently discard non-UBX data
                // UBX sync: 0xB5 0x62 (must be at start)
                if(px_drv->u16_rxLen >= 2 &&
                   px_drv->u8_rxBuf[0] == 0xB5 &&
                   px_drv->u8_rxBuf[1] == 0x62) {
                    // Valid UBX sync - proceed with validation
                    int validate_ret = i_UBX_ValidateMessage(px_drv->u8_rxBuf, px_drv->u16_rxLen);
                    if(validate_ret == SAM_M10Q_RET_OK) {
                        // Check if NAV-PVT message
                        if(px_drv->u8_rxBuf[2] == UBX_CLASS_NAV &&
                           px_drv->u8_rxBuf[3] == UBX_NAV_PVT) {
                            i_UBX_ParsePVT(&px_drv->u8_rxBuf[6], &px_drv->x_pvt);
                            px_drv->b_pvtValid = true;
                            px_drv->u32_lastUpdate = currentTime;

                            // Enhanced UBX-NAV-PVT output with full data structure
                            LOG_DEBUG("GPS", "NAV-PVT parsed:");
                            LOG_DEBUG("GPS", "  Fix=%d, Sats=%d, pDOP=%d.%02d",
                                          px_drv->x_pvt.fixType, px_drv->x_pvt.numSV,
                                          px_drv->x_pvt.pDOP / 100, px_drv->x_pvt.pDOP % 100);

                            // Position (lat/lon in 1e-7 degrees, convert to decimal)
                            int32_t lat_deg = px_drv->x_pvt.lat / 10000000;
                            int32_t lat_frac = px_drv->x_pvt.lat % 10000000;
                            if(lat_frac < 0) lat_frac = -lat_frac;
                            int32_t lon_deg = px_drv->x_pvt.lon / 10000000;
                            int32_t lon_frac = px_drv->x_pvt.lon % 10000000;
                            if(lon_frac < 0) lon_frac = -lon_frac;
                            LOG_DEBUG("GPS", "  Lat=%ld.%07ld° Lon=%ld.%07ld°",
                                          lat_deg, lat_frac, lon_deg, lon_frac);

                            // Altitude (hMSL in mm), Speed (gSpeed in mm/s), Heading (headMot in 1e-5 degrees)
                            int32_t alt_m = px_drv->x_pvt.hMSL / 1000;
                            int32_t alt_mm = px_drv->x_pvt.hMSL % 1000;
                            if(alt_mm < 0) alt_mm = -alt_mm;
                            int32_t spd_ms = px_drv->x_pvt.gSpeed / 1000;
                            int32_t spd_mms = px_drv->x_pvt.gSpeed % 1000;
                            if(spd_mms < 0) spd_mms = -spd_mms;
                            int32_t head_deg = px_drv->x_pvt.headMot / 100000;
                            int32_t head_frac = px_drv->x_pvt.headMot % 100000;
                            if(head_frac < 0) head_frac = -head_frac;
                            LOG_DEBUG("GPS", "  Alt=%ld.%03ldm Speed=%ld.%03ldm/s Head=%ld.%05ld°",
                                          alt_m, alt_mm, spd_ms, spd_mms, head_deg, head_frac);

                            // Accuracy (hAcc, vAcc in mm - uint32_t)
                            LOG_DEBUG("GPS", "  Acc H=%lu.%03lum V=%lu.%03lum",
                                          (unsigned long)(px_drv->x_pvt.hAcc / 1000),
                                          (unsigned long)(px_drv->x_pvt.hAcc % 1000),
                                          (unsigned long)(px_drv->x_pvt.vAcc / 1000),
                                          (unsigned long)(px_drv->x_pvt.vAcc % 1000));

                            // Time (UTC)
                            LOG_DEBUG("GPS", "  Time=%04d-%02d-%02d %02d:%02d:%02d UTC (valid=0x%02X)",
                                          px_drv->x_pvt.year, px_drv->x_pvt.month, px_drv->x_pvt.day,
                                          px_drv->x_pvt.hour, px_drv->x_pvt.min, px_drv->x_pvt.sec,
                                          px_drv->x_pvt.valid);
                        }
                    }
                    // UBX validation failed - silently discard
                } else {
                    // Not UBX format (NMEA or garbage data) - silently discard
                    // No logging to reduce clutter in UBX-only mode
                }
            }
            // Clear rx length after parsing
            px_drv->u16_rxLen = 0;
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
