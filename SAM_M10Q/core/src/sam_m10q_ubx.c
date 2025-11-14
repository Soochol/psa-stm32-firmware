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

/*
 * brief: Create UBX CFG-PRT message for I2C port configuration (Legacy)
 * note: CFG-PRT is deprecated but simpler and smaller than CFG-VALSET
 * return: Message length (28 bytes) if successful, error code otherwise
 */
int i_UBX_CreateCfgPRT(uint8_t* pu8_buf, uint16_t u16_bufSize,
                       uint8_t u8_i2cAddr, bool b_enableNMEA) {
    if(!pu8_buf || u16_bufSize < 28) return SAM_M10Q_RET_ERR_SIZE;

    uint16_t idx = 0;

    // Header
    pu8_buf[idx++] = UBX_SYNC_CHAR_1;         // 0xB5
    pu8_buf[idx++] = UBX_SYNC_CHAR_2;         // 0x62
    pu8_buf[idx++] = 0x06;                    // Class: CFG
    pu8_buf[idx++] = 0x00;                    // ID: PRT (Port)
    pu8_buf[idx++] = 0x14;                    // Length LSB (20 bytes payload)
    pu8_buf[idx++] = 0x00;                    // Length MSB

    // Payload
    pu8_buf[idx++] = 0x00;                    // Port ID (0 = I2C/DDC)
    pu8_buf[idx++] = 0x00;                    // Reserved
    pu8_buf[idx++] = 0x00;                    // txReady LSB
    pu8_buf[idx++] = 0x00;                    // txReady MSB
    pu8_buf[idx++] = u8_i2cAddr;              // I2C slave address (7-bit)
    pu8_buf[idx++] = 0x00;                    // Reserved
    pu8_buf[idx++] = 0x00;                    // Reserved
    pu8_buf[idx++] = 0x00;                    // Reserved

    // Protocol masks (0x0001=UBX, 0x0002=NMEA, 0x0003=Both)
    uint16_t protoMask = 0x0001;  // UBX only by default
    if(b_enableNMEA) {
        protoMask = 0x0003;  // UBX + NMEA
    }

    pu8_buf[idx++] = protoMask & 0xFF;        // inProtoMask LSB
    pu8_buf[idx++] = (protoMask >> 8) & 0xFF; // inProtoMask MSB
    pu8_buf[idx++] = protoMask & 0xFF;        // outProtoMask LSB
    pu8_buf[idx++] = (protoMask >> 8) & 0xFF; // outProtoMask MSB

    pu8_buf[idx++] = 0x00;                    // flags LSB
    pu8_buf[idx++] = 0x00;                    // flags MSB
    pu8_buf[idx++] = 0x00;                    // reserved
    pu8_buf[idx++] = 0x00;                    // reserved

    // Calculate checksum over class, ID, length, and payload (bytes 2-25)
    uint8_t ckA, ckB;
    v_UBX_CalcChecksum(&pu8_buf[2], 24, &ckA, &ckB);
    pu8_buf[idx++] = ckA;
    pu8_buf[idx++] = ckB;

    return idx;  // 28 bytes total
}

/*
 * brief: Create UBX CFG-CFG message to save configuration
 * note: Saves current RAM config to Flash/BBR/EEPROM (persistent storage)
 * return: Message length (21 bytes) if successful, error code otherwise
 *
 * Message structure (CFG-CFG):
 * Header (6 bytes): B5 62 06 09 0D 00
 * Payload (13 bytes):
 *   - clearMask (4 bytes): Sections to clear before save (usually 0)
 *   - saveMask (4 bytes): Sections to save (0x1F = I/O Port settings)
 *   - loadMask (4 bytes): Sections to load after save (usually 0)
 *   - deviceMask (1 byte): Target device (0x17 = Flash+BBR+EEPROM)
 * Footer (2 bytes): Checksum
 */
int i_UBX_CreateCfgSave(uint8_t* pu8_buf, uint16_t u16_bufSize, uint32_t u32_saveMask) {
    if(!pu8_buf || u16_bufSize < 21) return SAM_M10Q_RET_ERR_SIZE;

    uint16_t idx = 0;

    // Header
    pu8_buf[idx++] = UBX_SYNC_CHAR_1;         // 0xB5
    pu8_buf[idx++] = UBX_SYNC_CHAR_2;         // 0x62
    pu8_buf[idx++] = UBX_CLASS_CFG;           // 0x06
    pu8_buf[idx++] = UBX_CFG_CFG;             // 0x09
    pu8_buf[idx++] = 0x0D;                    // Length LSB (13 bytes payload)
    pu8_buf[idx++] = 0x00;                    // Length MSB

    // Payload: clearMask (4 bytes, little-endian) - don't clear anything
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;

    // Payload: saveMask (4 bytes, little-endian)
    pu8_buf[idx++] = (u32_saveMask >> 0) & 0xFF;
    pu8_buf[idx++] = (u32_saveMask >> 8) & 0xFF;
    pu8_buf[idx++] = (u32_saveMask >> 16) & 0xFF;
    pu8_buf[idx++] = (u32_saveMask >> 24) & 0xFF;

    // Payload: loadMask (4 bytes, little-endian) - don't load after save
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;
    pu8_buf[idx++] = 0x00;

    // Payload: deviceMask (1 byte) - 0x17 = devBBR | devFlash | devEEPROM
    pu8_buf[idx++] = 0x17;

    // Calculate checksum over class, ID, length, and payload (bytes 2-18)
    uint8_t ckA, ckB;
    v_UBX_CalcChecksum(&pu8_buf[2], 17, &ckA, &ckB);
    pu8_buf[idx++] = ckA;
    pu8_buf[idx++] = ckB;

    return idx;  // 21 bytes total
}

/*
 * brief: Create UBX CFG-VALSET message for I2C port configuration
 * note: Configures GPS module to use I2C interface with UBX protocol
 * return: Message length if successful, error code otherwise
 *
 * Message format (CFG-VALSET):
 * Header (6 bytes): Sync1, Sync2, Class, ID, Length(LSB), Length(MSB)
 * Payload:
 *   - Version: 0x00
 *   - Layers: RAM/BBR/FLASH bitfield
 *   - Reserved: 0x00, 0x00
 *   - CFG-DATA: Key-value pairs (key=4 bytes, value=1-8 bytes)
 * Footer (2 bytes): Checksum A, Checksum B
 */
int i_UBX_CreateCfgI2C(uint8_t* pu8_buf, uint16_t u16_bufSize,
                       uint8_t u8_layer, uint8_t u8_i2cAddr, bool b_enableNMEA) {
    if(!pu8_buf || u16_bufSize < 32) return SAM_M10Q_RET_ERR_SIZE;  // Minimal config needs ~24 bytes

    uint16_t idx = 0;

    // Header
    pu8_buf[idx++] = UBX_SYNC_CHAR_1;         // 0xB5
    pu8_buf[idx++] = UBX_SYNC_CHAR_2;         // 0x62
    pu8_buf[idx++] = UBX_CLASS_CFG;           // 0x06
    pu8_buf[idx++] = UBX_CFG_VALSET;          // 0x8A

    // Placeholder for length (will be filled later)
    uint16_t lengthIdx = idx;
    idx += 2;

    // Payload header
    pu8_buf[idx++] = 0x00;                    // Version 0
    pu8_buf[idx++] = CFG_LAYER_RAM;           // Layer: RAM only (0x01) - SparkFun method
    pu8_buf[idx++] = 0x00;                    // Reserved1
    pu8_buf[idx++] = 0x00;                    // Reserved2

    // Helper macro for adding key-value pairs (little-endian)
    #define ADD_CFG_KEY_L(key, val) do { \
        uint32_t k = (key); \
        pu8_buf[idx++] = (k >> 0) & 0xFF; \
        pu8_buf[idx++] = (k >> 8) & 0xFF; \
        pu8_buf[idx++] = (k >> 16) & 0xFF; \
        pu8_buf[idx++] = (k >> 24) & 0xFF; \
        pu8_buf[idx++] = (val) ? 0x01 : 0x00; \
    } while(0)

    #define ADD_CFG_KEY_U1(key, val) do { \
        uint32_t k = (key); \
        pu8_buf[idx++] = (k >> 0) & 0xFF; \
        pu8_buf[idx++] = (k >> 8) & 0xFF; \
        pu8_buf[idx++] = (k >> 16) & 0xFF; \
        pu8_buf[idx++] = (k >> 24) & 0xFF; \
        pu8_buf[idx++] = (val); \
    } while(0)

    // MINIMAL CONFIG - SparkFun method (only change output protocol)
    // DO NOT change I2C address or other settings that could cause rejection

    if(b_enableNMEA) {
        // Enable both UBX and NMEA output
        ADD_CFG_KEY_L(CFG_KEY_I2COUTPROT_UBX, true);
        ADD_CFG_KEY_L(CFG_KEY_I2COUTPROT_NMEA, true);
    } else {
        // Disable NMEA, enable UBX only
        ADD_CFG_KEY_L(CFG_KEY_I2COUTPROT_UBX, true);
        ADD_CFG_KEY_L(CFG_KEY_I2COUTPROT_NMEA, false);
    }

    #undef ADD_CFG_KEY_L
    #undef ADD_CFG_KEY_U1

    // Calculate payload length
    uint16_t payloadLen = idx - 6;

    // Fill in length field (little-endian)
    pu8_buf[lengthIdx] = payloadLen & 0xFF;
    pu8_buf[lengthIdx + 1] = (payloadLen >> 8) & 0xFF;

    // Calculate checksum over class, ID, length, and payload
    uint8_t ckA, ckB;
    v_UBX_CalcChecksum(&pu8_buf[2], 4 + payloadLen, &ckA, &ckB);
    pu8_buf[idx++] = ckA;
    pu8_buf[idx++] = ckB;

    return idx;  // Total message length
}
