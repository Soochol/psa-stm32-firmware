/*
 * sam_m10q_ubx.c
 *
 * SAM-M10Q GPS Module - UBX Protocol Implementation
 * Checksum calculation, message validation, parsing
 *
 * Created: 2025-01-11
 */

#include "sam_m10q_ubx.h"
#include <string.h>

/*
 * brief: Calculate UBX checksum (Fletcher-8 algorithm)
 * note: Checksum is calculated over message class, ID, length, and payload
 */
void v_UBX_CalcChecksum(uint8_t* pu8_data, uint16_t u16_len,
                        uint8_t* pu8_ckA, uint8_t* pu8_ckB) {
    uint8_t ckA = 0, ckB = 0;
    for(uint16_t i = 0; i < u16_len; i++) {
        ckA += pu8_data[i];
        ckB += ckA;
    }
    *pu8_ckA = ckA;
    *pu8_ckB = ckB;
}

/*
 * brief: Validate UBX message structure and checksum
 * return: SAM_M10Q_RET_OK if valid, error code otherwise
 */
int i_UBX_ValidateMessage(uint8_t* pu8_msg, uint16_t u16_len) {
    if(u16_len < 8) return SAM_M10Q_RET_ERR_SIZE;  // Min: header + footer

    // Check sync characters
    if(pu8_msg[0] != UBX_SYNC_CHAR_1 || pu8_msg[1] != UBX_SYNC_CHAR_2) {
        return SAM_M10Q_RET_ERR_ARG;
    }

    // Extract payload length (little-endian)
    uint16_t payloadLen = pu8_msg[4] | (pu8_msg[5] << 8);

    // Verify total length
    if(u16_len < (6 + payloadLen + 2)) {
        return SAM_M10Q_RET_ERR_SIZE;
    }

    // Calculate checksum (over class, ID, length, payload)
    uint8_t ckA_calc, ckB_calc;
    v_UBX_CalcChecksum(&pu8_msg[2], 4 + payloadLen, &ckA_calc, &ckB_calc);

    // Compare with message checksum
    uint8_t ckA_msg = pu8_msg[6 + payloadLen];
    uint8_t ckB_msg = pu8_msg[7 + payloadLen];

    if(ckA_calc != ckA_msg || ckB_calc != ckB_msg) {
        return SAM_M10Q_RET_CHECKSUM_ERR;
    }

    return SAM_M10Q_RET_OK;
}

/*
 * brief: Parse UBX-NAV-PVT payload into simplified GPS_PVT structure
 * return: SAM_M10Q_RET_OK if successful
 */
int i_UBX_ParsePVT(uint8_t* pu8_payload, _x_GPS_PVT_t* px_pvt) {
    if(!pu8_payload || !px_pvt) return SAM_M10Q_RET_ERR_ARG;

    // Cast payload to structured format (assuming little-endian STM32)
    _x_UBX_NAV_PVT_PAYLOAD_t* pPayload = (_x_UBX_NAV_PVT_PAYLOAD_t*)pu8_payload;

    // Copy relevant fields
    px_pvt->iTOW    = pPayload->iTOW;
    px_pvt->year    = pPayload->year;
    px_pvt->month   = pPayload->month;
    px_pvt->day     = pPayload->day;
    px_pvt->hour    = pPayload->hour;
    px_pvt->min     = pPayload->min;
    px_pvt->sec     = pPayload->sec;
    px_pvt->valid   = pPayload->valid;
    px_pvt->fixType = pPayload->fixType;
    px_pvt->numSV   = pPayload->numSV;
    px_pvt->lon     = pPayload->lon;
    px_pvt->lat     = pPayload->lat;
    px_pvt->height  = pPayload->height;
    px_pvt->hMSL    = pPayload->hMSL;
    px_pvt->hAcc    = pPayload->hAcc;
    px_pvt->vAcc    = pPayload->vAcc;
    px_pvt->pDOP    = pPayload->pDOP;
    px_pvt->gSpeed  = pPayload->gSpeed;
    px_pvt->headMot = pPayload->headMot;

    return SAM_M10Q_RET_OK;
}

/*
 * brief: Create UBX poll message for NAV-PVT
 * note: Format: B5 62 01 07 00 00 CK_A CK_B (8 bytes total)
 * return: Message length (8) if successful, error code otherwise
 */
int i_UBX_CreatePollPVT(uint8_t* pu8_buf, uint16_t u16_bufSize) {
    if(!pu8_buf || u16_bufSize < 8) return SAM_M10Q_RET_ERR_SIZE;

    pu8_buf[0] = UBX_SYNC_CHAR_1;
    pu8_buf[1] = UBX_SYNC_CHAR_2;
    pu8_buf[2] = UBX_CLASS_NAV;
    pu8_buf[3] = UBX_NAV_PVT;
    pu8_buf[4] = 0x00;  // Payload length LSB (0 for poll)
    pu8_buf[5] = 0x00;  // Payload length MSB

    // Calculate checksum over bytes 2-5
    uint8_t ckA, ckB;
    v_UBX_CalcChecksum(&pu8_buf[2], 4, &ckA, &ckB);
    pu8_buf[6] = ckA;
    pu8_buf[7] = ckB;

    return 8;  // Message length
}
