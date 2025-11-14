/*
 * sam_m10q_uart_config.h
 *
 * SAM-M10Q GPS Module - UART Configuration Interface
 * Used to configure GPS to I2C mode when GPS is in UART-only mode
 *
 * Created: 2025-01-14
 */

#ifndef __SAM_M10Q_UART_CONFIG_H
#define __SAM_M10Q_UART_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"

/**
 * @brief Configure GPS to I2C mode via UART
 *
 * This function sends UBX CFG-PRT and CFG-SAVE messages via UART to enable I2C mode.
 * Use this when GPS is in UART-only mode and not responding to I2C commands.
 *
 * @param huart Pointer to UART handle (e.g., &huart3, &huart4)
 * @param i2c_addr I2C address to set (e.g., 0x42 for default)
 * @param save_to_flash true = save config to flash (permanent), false = RAM only (temporary)
 * @return 0 on success (ACK received), -1 on failure (NAK or timeout)
 *
 * @note This is a one-time configuration. Once saved to flash, GPS will boot in I2C mode.
 * @note Default GPS UART baud rate is usually 9600 or 38400
 * @note GPS will send ACK (0xB5 0x62 0x05 0x01) on success, NAK (0xB5 0x62 0x05 0x00) on failure
 */
int i_GPS_ConfigureI2C_ViaUART(UART_HandleTypeDef* huart,
                               uint8_t i2c_addr,
                               bool save_to_flash);

/**
 * @brief Send UBX message via UART and wait for ACK
 *
 * Low-level function to send any UBX message via UART and wait for GPS response.
 *
 * @param huart UART handle
 * @param msg UBX message buffer (must include sync bytes 0xB5 0x62)
 * @param len Message length in bytes
 * @param timeout_ms Timeout in milliseconds (typically 1000-2000ms)
 * @return 0 on success (ACK received), -1 on NAK or timeout
 */
int i_GPS_SendUBX_UART(UART_HandleTypeDef* huart,
                       const uint8_t* msg,
                       uint16_t len,
                       uint32_t timeout_ms);

/**
 * @brief Test if GPS is responding on UART
 *
 * Sends a simple poll request to check if GPS is connected and responding.
 *
 * @param huart UART handle
 * @return 0 if GPS responds, -1 if no response
 */
int i_GPS_TestUART(UART_HandleTypeDef* huart);

#ifdef __cplusplus
}
#endif

#endif // __SAM_M10Q_UART_CONFIG_H
