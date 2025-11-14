/*
 * sam_m10q_uart_config.c
 *
 * SAM-M10Q GPS Module - UART Configuration Implementation
 * Enables I2C mode when GPS is in UART-only mode
 *
 * Created: 2025-01-14
 */

#include "sam_m10q_uart_config.h"
#include "sam_m10q_ubx.h"
#include "sam_m10q_def.h"
#include <string.h>

// External printf function
extern void v_printf_poll(const char* format, ...);

/**
 * @brief Test if GPS is responding on UART
 */
int i_GPS_TestUART(UART_HandleTypeDef* huart) {
    if(!huart) return -1;

    // Send UBX-MON-VER poll request (no payload)
    uint8_t poll_ver[] = {
        0xB5, 0x62,  // Sync
        0x0A, 0x04,  // MON-VER
        0x00, 0x00,  // Length = 0
        0x0E, 0x34   // Checksum
    };

    HAL_StatusTypeDef ret = HAL_UART_Transmit(huart, poll_ver, sizeof(poll_ver), 500);
    if(ret != HAL_OK) {
        return -1;
    }

    // Wait for any response
    uint8_t rx_buf[10];
    ret = HAL_UART_Receive(huart, rx_buf, 2, 1000);  // Just check for sync bytes
    if(ret == HAL_OK && rx_buf[0] == 0xB5 && rx_buf[1] == 0x62) {
        return 0;  // GPS responded
    }

    return -1;  // No response
}

/**
 * @brief Send UBX message via UART and wait for ACK
 */
int i_GPS_SendUBX_UART(UART_HandleTypeDef* huart,
                       const uint8_t* msg,
                       uint16_t len,
                       uint32_t timeout_ms) {
    if(!huart || !msg || len == 0) return -1;

    v_printf_poll("GPS UART: Sending UBX message (%d bytes)\r\n", len);

    // Hex dump first 16 bytes for debugging
    v_printf_poll("GPS UART: TX = ");
    uint16_t dump_len = (len > 16) ? 16 : len;
    for(uint16_t i = 0; i < dump_len; i++) {
        v_printf_poll("%02X ", msg[i]);
    }
    v_printf_poll("%s\r\n", (len > 16) ? "..." : "");

    // Send message
    HAL_StatusTypeDef ret = HAL_UART_Transmit(huart, (uint8_t*)msg, len, timeout_ms);
    if(ret != HAL_OK) {
        v_printf_poll("GPS UART: Transmit failed (ret=%d)\r\n", ret);
        return -1;
    }

    v_printf_poll("GPS UART: Waiting for ACK/NAK...\r\n");

    // Wait for ACK or NAK
    // ACK: B5 62 05 01 02 00 <class> <id> <checksum>
    // NAK: B5 62 05 00 02 00 <class> <id> <checksum>
    uint8_t ack_buf[10];
    memset(ack_buf, 0, sizeof(ack_buf));

    ret = HAL_UART_Receive(huart, ack_buf, 10, timeout_ms);
    if(ret != HAL_OK) {
        v_printf_poll("GPS UART: No response (timeout)\r\n");
        return -1;
    }

    // Hex dump response
    v_printf_poll("GPS UART: RX = ");
    for(int i = 0; i < 10; i++) {
        v_printf_poll("%02X ", ack_buf[i]);
    }
    v_printf_poll("\r\n");

    // Check for ACK (B5 62 05 01)
    if(ack_buf[0] == 0xB5 && ack_buf[1] == 0x62 &&
       ack_buf[2] == 0x05 && ack_buf[3] == 0x01) {
        v_printf_poll("GPS UART: ACK received ✓\r\n");
        return 0;
    }

    // Check for NAK (B5 62 05 00)
    if(ack_buf[0] == 0xB5 && ack_buf[1] == 0x62 &&
       ack_buf[2] == 0x05 && ack_buf[3] == 0x00) {
        v_printf_poll("GPS UART: NAK received ✗\r\n");
        return -1;
    }

    v_printf_poll("GPS UART: Unknown response\r\n");
    return -1;
}

/**
 * @brief Configure GPS to I2C mode via UART
 */
int i_GPS_ConfigureI2C_ViaUART(UART_HandleTypeDef* huart,
                               uint8_t i2c_addr,
                               bool save_to_flash) {
    if(!huart) return -1;

    v_printf_poll("\r\n=== GPS UART CONFIGURATION START ===\r\n");
    v_printf_poll("GPS: Configuring I2C mode (addr=0x%02X) via UART\r\n", i2c_addr);

    // Test if GPS is responding on UART
    v_printf_poll("GPS: Testing UART connection...\r\n");
    int test_ret = i_GPS_TestUART(huart);
    if(test_ret != 0) {
        v_printf_poll("GPS: WARNING - No response on UART\r\n");
        v_printf_poll("GPS: Check UART connection and baud rate\r\n");
        // Continue anyway - GPS might respond to config commands
    } else {
        v_printf_poll("GPS: UART connection OK ✓\r\n");
    }

    // Step 1: Send CFG-PRT to enable I2C
    uint8_t cfg_prt[28];
    int cfg_len = i_UBX_CreateCfgPRT(cfg_prt, sizeof(cfg_prt), i2c_addr, false);

    v_printf_poll("GPS: Sending CFG-PRT (enable I2C, disable NMEA)\r\n");

    int ret = i_GPS_SendUBX_UART(huart, cfg_prt, cfg_len, 2000);
    if(ret != 0) {
        v_printf_poll("GPS: CFG-PRT failed - GPS may already be in I2C mode\r\n");
        return -1;
    }

    v_printf_poll("GPS: CFG-PRT succeeded ✓\r\n");
    HAL_Delay(100);  // Wait for GPS to process

    // Step 2: Save to flash if requested
    if(save_to_flash) {
        uint8_t cfg_save[21];
        int save_len = i_UBX_CreateCfgSave(cfg_save, sizeof(cfg_save), 0x0000001F);

        v_printf_poll("GPS: Sending CFG-SAVE (save to flash)\r\n");

        ret = i_GPS_SendUBX_UART(huart, cfg_save, save_len, 2000);
        if(ret != 0) {
            v_printf_poll("GPS: CFG-SAVE failed\r\n");
            return -1;
        }

        v_printf_poll("GPS: CFG-SAVE succeeded ✓\r\n");
        v_printf_poll("GPS: Configuration saved to flash (permanent)\r\n");
    } else {
        v_printf_poll("GPS: Configuration applied to RAM only (temporary)\r\n");
    }

    v_printf_poll("GPS: I2C mode enabled successfully!\r\n");
    v_printf_poll("=== GPS UART CONFIGURATION COMPLETE ===\r\n\r\n");

    v_printf_poll("GPS: Please wait 1 second for GPS to reboot...\r\n");

    return 0;
}
