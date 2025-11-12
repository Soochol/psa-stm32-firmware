/*
 * sam_m10q_ubx.h
 *
 * SAM-M10Q GPS Module - UBX Protocol Definitions
 * UBX message structures and protocol utilities
 *
 * Created: 2025-01-11
 */

#ifndef __JH_SAM_M10Q_UBX_H
#define __JH_SAM_M10Q_UBX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sam_m10q_def.h"

/*
 * UBX Message Header
 */
typedef struct {
    uint8_t sync1;         // 0xB5
    uint8_t sync2;         // 0x62
    uint8_t msgClass;
    uint8_t msgID;
    uint16_t payloadLen;   // Little-endian
} __attribute__((packed)) _x_UBX_HEADER_t;

/*
 * UBX Message Footer
 */
typedef struct {
    uint8_t ckA;           // Checksum A
    uint8_t ckB;           // Checksum B
} __attribute__((packed)) _x_UBX_FOOTER_t;

/*
 * Complete UBX-NAV-PVT Message Payload (92 bytes)
 */
typedef struct {
    uint32_t iTOW;         // GPS time of week (ms)
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  valid;
    uint32_t tAcc;
    int32_t  nano;
    uint8_t  fixType;
    uint8_t  flags;
    uint8_t  flags2;
    uint8_t  numSV;
    int32_t  lon;
    int32_t  lat;
    int32_t  height;
    int32_t  hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t  velN;
    int32_t  velE;
    int32_t  velD;
    int32_t  gSpeed;
    int32_t  headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint8_t  flags3;
    uint8_t  reserved1[5];
    int32_t  headVeh;
    int16_t  magDec;
    uint16_t magAcc;
} __attribute__((packed)) _x_UBX_NAV_PVT_PAYLOAD_t;

/*
 * UBX Checksum Calculation (Fletcher-8 algorithm)
 * pu8_data: Data buffer (starting from class byte)
 * u16_len: Length of data
 * pu8_ckA: Output checksum A
 * pu8_ckB: Output checksum B
 */
void v_UBX_CalcChecksum(uint8_t* pu8_data, uint16_t u16_len,
                        uint8_t* pu8_ckA, uint8_t* pu8_ckB);

/*
 * UBX Message Validation
 * pu8_msg: Complete UBX message buffer
 * u16_len: Total message length
 * return: SAM_M10Q_RET_OK if valid, error code otherwise
 */
int i_UBX_ValidateMessage(uint8_t* pu8_msg, uint16_t u16_len);

/*
 * Parse UBX-NAV-PVT Payload
 * pu8_payload: Pointer to payload (after header)
 * px_pvt: Output simplified PVT structure
 * return: SAM_M10Q_RET_OK if successful
 */
int i_UBX_ParsePVT(uint8_t* pu8_payload, _x_GPS_PVT_t* px_pvt);

/*
 * Create UBX Poll Message for NAV-PVT
 * pu8_buf: Output buffer
 * u16_bufSize: Buffer size (must be >= 8)
 * return: Message length (8 bytes) or error code
 */
int i_UBX_CreatePollPVT(uint8_t* pu8_buf, uint16_t u16_bufSize);

#ifdef __cplusplus
}
#endif

#endif // __JH_SAM_M10Q_UBX_H
