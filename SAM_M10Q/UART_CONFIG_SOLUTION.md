# GPS UART Configuration Solution

**Date**: 2025-01-14
**Problem**: GPS in UART-only mode, not responding to I2C
**Solution**: Configure GPS via UART to enable I2C mode

---

## Root Cause Confirmation

**Log Analysis**:
```
GPS: I2C3 State = 32 = 0x20 = HAL_I2C_STATE_READY ✅
GPS: CFG-PRT send failed (ErrorCode=0x00000004) ❌
```

- I2C3 peripheral: **Working correctly** (State = READY)
- Error 0x04 = HAL_I2C_ERROR_AF: **GPS not responding** (NAK)
- **Conclusion**: GPS is in UART-only mode

---

## Solution: UART-Based Configuration

### Prerequisites

1. **GPS UART Connection Required**:
   - GPS TX → STM32 UART RX
   - GPS RX → STM32 UART TX
   - Common GND
   - Default baud: 9600 or 38400 (u-blox default)

2. **Available UART**:
   - Check which UART is connected to GPS
   - Options: UART3, UART4, UART5, UART8

### Step 1: Add UART Configuration Function

**File**: `SAM_M10Q/platform/inc/sam_m10q_uart_config.h`

```c
#ifndef __SAM_M10Q_UART_CONFIG_H
#define __SAM_M10Q_UART_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configure GPS to I2C mode via UART
 * @param huart Pointer to UART handle (e.g., &huart3)
 * @param i2c_addr I2C address to set (e.g., 0x42)
 * @param save_to_flash true = save to flash, false = RAM only
 * @return 0 on success, -1 on failure
 */
int i_GPS_ConfigureI2C_ViaUART(UART_HandleTypeDef* huart,
                               uint8_t i2c_addr,
                               bool save_to_flash);

/**
 * @brief Send UBX message via UART and wait for ACK
 * @param huart UART handle
 * @param msg UBX message buffer
 * @param len Message length
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success (ACK), -1 on NAK or timeout
 */
int i_GPS_SendUBX_UART(UART_HandleTypeDef* huart,
                       const uint8_t* msg,
                       uint16_t len,
                       uint32_t timeout_ms);

#endif
```

### Step 2: Implement UART Configuration

**File**: `SAM_M10Q/platform/src/sam_m10q_uart_config.c`

```c
#include "sam_m10q_uart_config.h"
#include "sam_m10q_ubx.h"
#include "main.h"
#include <string.h>

// Send UBX message via UART
int i_GPS_SendUBX_UART(UART_HandleTypeDef* huart,
                       const uint8_t* msg,
                       uint16_t len,
                       uint32_t timeout_ms) {
    if(!huart || !msg || len == 0) return -1;

    // Send message
    HAL_StatusTypeDef ret = HAL_UART_Transmit(huart, (uint8_t*)msg, len, timeout_ms);
    if(ret != HAL_OK) {
        return -1;
    }

    // Wait for ACK (UBX-ACK-ACK: B5 62 05 01 ...)
    uint8_t ack_buf[10];
    ret = HAL_UART_Receive(huart, ack_buf, 10, timeout_ms);
    if(ret != HAL_OK) {
        return -1;  // Timeout or error
    }

    // Check for ACK (B5 62 05 01)
    if(ack_buf[0] == 0xB5 && ack_buf[1] == 0x62 &&
       ack_buf[2] == 0x05 && ack_buf[3] == 0x01) {
        return 0;  // ACK received
    }

    // Check for NAK (B5 62 05 00)
    if(ack_buf[2] == 0x05 && ack_buf[3] == 0x00) {
        return -1;  // NAK received
    }

    return -1;  // Unknown response
}

// Configure GPS to I2C mode via UART
int i_GPS_ConfigureI2C_ViaUART(UART_HandleTypeDef* huart,
                               uint8_t i2c_addr,
                               bool save_to_flash) {
    if(!huart) return -1;

    extern void v_printf_poll(const char* format, ...);

    v_printf_poll("\r\n*** GPS UART CONFIG START ***\r\n");
    v_printf_poll("GPS: Configuring I2C mode via UART...\r\n");

    // Step 1: Send CFG-PRT to enable I2C
    uint8_t cfg_prt[28];
    int cfg_len = i_UBX_CreateCfgPRT(cfg_prt, sizeof(cfg_prt), i2c_addr, false);

    v_printf_poll("GPS: Sending CFG-PRT via UART (%d bytes)\r\n", cfg_len);

    int ret = i_GPS_SendUBX_UART(huart, cfg_prt, cfg_len, 1000);
    if(ret != 0) {
        v_printf_poll("GPS: CFG-PRT failed (no ACK)\r\n");
        return -1;
    }

    v_printf_poll("GPS: CFG-PRT ACK received!\r\n");

    // Step 2: Save to flash if requested
    if(save_to_flash) {
        HAL_Delay(100);  // Wait for GPS to process

        uint8_t cfg_save[21];
        int save_len = i_UBX_CreateCfgSave(cfg_save, sizeof(cfg_save), 0x0000001F);

        v_printf_poll("GPS: Sending CFG-SAVE via UART (%d bytes)\r\n", save_len);

        ret = i_GPS_SendUBX_UART(huart, cfg_save, save_len, 1000);
        if(ret != 0) {
            v_printf_poll("GPS: CFG-SAVE failed (no ACK)\r\n");
            return -1;
        }

        v_printf_poll("GPS: CFG-SAVE ACK received! Config saved to flash.\r\n");
    }

    v_printf_poll("GPS: I2C mode enabled successfully!\r\n");
    v_printf_poll("*** GPS UART CONFIG COMPLETE ***\r\n\r\n");

    return 0;
}
```

### Step 3: Usage Example

**In main.c or GPS init**:

```c
// Before calling v_GPS_Init(), configure GPS via UART if needed
extern UART_HandleTypeDef huart3;  // Or whichever UART is connected to GPS

// Configure GPS to I2C mode (one-time setup)
int uart_ret = i_GPS_ConfigureI2C_ViaUART(&huart3, 0x42, true);
if(uart_ret == 0) {
    v_printf_poll("GPS configured to I2C mode successfully!\r\n");
    HAL_Delay(1000);  // Wait for GPS to reboot
} else {
    v_printf_poll("GPS UART config failed - GPS may already be in I2C mode\r\n");
}

// Now initialize I2C interface
v_GPS_Init();
```

---

## Alternative: Manual Configuration via u-center

If UART is not connected to STM32:

1. Connect GPS to PC via USB-to-UART adapter
2. Open u-blox u-center software
3. Configure I2C port:
   - View → Configuration View
   - PRT (Ports) → Port 0 (I2C)
   - Set address: 0x42
   - Protocol: UBX only (0x0001)
   - Click "Send"
4. Save to flash:
   - CFG → Save current configuration
   - Select all layers (BBR, Flash, etc.)
   - Click "Send"
5. Disconnect from PC, connect to STM32

---

## Verification

After configuration, GPS should respond to I2C:

```c
// Test I2C communication
extern I2C_HandleTypeDef hi2c3;
HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(&hi2c3, 0x42<<1, 3, 100);

if(ret == HAL_OK) {
    v_printf_poll("GPS I2C: Device ready! ✅\r\n");
} else {
    v_printf_poll("GPS I2C: No response (ErrorCode=0x%08lX) ❌\r\n", hi2c3.ErrorCode);
}
```

Expected output after successful configuration:
```
GPS I2C: Device ready! ✅
GPS: CFG-PRT sent OK
GPS: CFG-SAVE sent OK
GPS State: CHECK_AVAIL → WAIT_AVAIL
GPS: Available bytes=98
GPS: NAV-PVT parsed (Fix=0, Sats=0)
```

---

## Troubleshooting

### GPS still NAKing after UART config
- Check GPS power cycle (may need reboot)
- Verify UART baud rate (try 9600, 38400, 115200)
- Check UART TX/RX not swapped

### No UART connection available
- Use u-center on PC for one-time configuration
- Or use factory default I2C address (check GPS datasheet)

### ACK not received
- GPS may already be in I2C mode (not an error)
- Try I2C communication directly

---

## Next Steps

1. **Identify GPS UART connection** on your board
2. **Add UART config files** to project
3. **Call config function** before `v_GPS_Init()`
4. **Test I2C communication** with `HAL_I2C_IsDeviceReady()`
5. **If successful**, remove UART config code (one-time setup)

---

## Summary

**Problem**: GPS in UART mode → I2C NAK
**Solution**: Send CFG-PRT via UART → Enable I2C → Save to flash
**Result**: GPS responds to I2C ✅

This is a **one-time configuration**. Once saved to flash, GPS will boot in I2C mode permanently.
