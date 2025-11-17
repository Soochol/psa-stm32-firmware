# Timeout Handler Standardization

## Summary

All sensor I2C timeout handlers have been standardized to follow the same robust pattern with enhanced diagnostics and proper I2C bus recovery.

## Problem Analysis

**Issue**: ES8388 audio codec timeout handler was missing critical I2C bus recovery logic, risking I2C5 bus lockup during codec communication failures.

**Comparison with other sensors**:
- ✅ MLX90640, AS6221, ADS111x, ICM42670P: All use enhanced diagnostics + I2C abort
- ❌ ES8388: Was missing `HAL_I2C_Master_Abort_IT()` call and diagnostic logging
- ℹ️ GPS: Uses minimal handler (GPS is optional, doesn't trigger ERROR mode)

## Standard Timeout Handler Pattern

All I2C sensor timeout handlers now follow this pattern:

```c
void v_SENSOR_Tout_Handler() {
    extern I2C_HandleTypeDef hi2cX;

    if((e_sensor_evt == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)) {
        // 1. Print diagnostic header
        printf("[SENSOR_TIMEOUT] SENSOR_NAME I2Cx timeout (addr=0x%02X)\r\n", ADDR_SENSOR);

        // 2. Print I2C hardware state
        printf("  ISR=0x%08lX (BUSY=%d, STOPF=%d)\r\n",
               hi2cX.Instance->ISR,
               (hi2cX.Instance->ISR & 0x8000) ? 1 : 0,  // BUSY bit
               (hi2cX.Instance->ISR & 0x0020) ? 1 : 0); // STOPF bit

        // 3. Print HAL error codes with details
        printf("  ErrorCode=0x%08lX\r\n", hi2cX.ErrorCode);
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_BERR)    printf("    - Bus Error\r\n");
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_ARLO)    printf("    - Arbitration Lost\r\n");
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_AF)      printf("    - NACK (device not responding)\r\n");
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_OVR)     printf("    - Overrun\r\n");
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_TIMEOUT) printf("    - HAL Timeout\r\n");
        if(hi2cX.ErrorCode & HAL_I2C_ERROR_DMA)     printf("    - DMA Error\r\n");

        // 4. CRITICAL: Abort I2C transaction to recover bus
        HAL_I2C_Master_Abort_IT(&hi2cX, ADDR_SENSOR);

        // 5. Reset state and enter ERROR mode
        e_sensor_evt = COMM_STAT_READY;
        e_sensor_config = COMM_STAT_ERR;
        v_Mode_Set_Error(modeERR_SENSOR);
        v_Mode_SetNext(modeERROR);
    }
}
```

## Timeout Handler Inventory

| Sensor | I2C Bus | Timeout (ms) | File | Status |
|--------|---------|--------------|------|--------|
| SAM-M10Q GPS | I2C3 | 2000 | SAM_M10Q/platform/src/sam_m10q_platform.c:472 | ⚠️ Special (no ERROR mode) |
| MLX90640 Thermal | I2C4 | 2000 | MLX90640/platform/src/mlx90640_platform.c:115 | ✅ Standard |
| AS6221 Temp (Indoor) | I2C1 | 2000 | AS6221/platform/src/as6221_platform.c:113 | ✅ Standard |
| AS6221 Temp (Outdoor) | I2C1 | 2000 | AS6221/platform/src/as6221_platform.c:136 | ✅ Standard |
| ADS111x FSR (Left) | I2C2 | 2000 | ADS111x/platform/src/ads111x_platform.c:106 | ✅ Standard |
| ADS111x FSR (Right) | I2C2 | 2000 | ADS111x/platform/src/ads111x_platform.c:132 | ✅ Standard |
| ICM42670P IMU (Left) | I2C2 | 2000 | ICM42670P/platform/src/icm42670p_platform.c:180 | ✅ Standard |
| ICM42670P IMU (Right) | I2C2 | 2000 | ICM42670P/platform/src/icm42670p_platform.c:204 | ✅ Standard |
| **ES8388 Codec** | **I2C5** | **2000** | **ES8388/platform/src/es8388_platform.c:66** | **✅ FIXED** |
| ESP32 UART | UART | 1000 | User/Edit/src/comm_esp.c:693 | ℹ️ UART (no I2C abort) |

## Changes Made to ES8388

**File**: [ES8388/platform/src/es8388_platform.c](ES8388/platform/src/es8388_platform.c#L66-L91)

**Before**:
```c
void v_Codec_Tout_Handler(){
	if(i_toutAct && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)){
		//timeout
		i_toutAct = 0;
		e_codec_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_AUDIO);
		v_Mode_SetNext(modeERROR);
	}
}
```

**After**:
```c
void v_Codec_Tout_Handler(){
	extern I2C_HandleTypeDef hi2c5;

	if(i_toutAct && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)){
		// MEDIUM: Abort DMA transaction to prevent I2C bus lockup
		printf("[CODEC_TIMEOUT] ES8388 I2C5 timeout (addr=0x%02X)\r\n", ADDR_CODEC);
		printf("  ISR=0x%08lX (BUSY=%d, STOPF=%d)\r\n",
		       hi2c5.Instance->ISR,
		       (hi2c5.Instance->ISR & 0x8000) ? 1 : 0,  // BUSY bit
		       (hi2c5.Instance->ISR & 0x0020) ? 1 : 0); // STOPF bit
		printf("  ErrorCode=0x%08lX\r\n", hi2c5.ErrorCode);
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_BERR)    printf("    - Bus Error\r\n");
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_ARLO)    printf("    - Arbitration Lost\r\n");
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_AF)      printf("    - NACK (device not responding)\r\n");
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_OVR)     printf("    - Overrun\r\n");
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_TIMEOUT) printf("    - HAL Timeout\r\n");
		if(hi2c5.ErrorCode & HAL_I2C_ERROR_DMA)     printf("    - DMA Error\r\n");

		HAL_I2C_Master_Abort_IT(&hi2c5, ADDR_CODEC);  // ← CRITICAL: Added

		i_toutAct = 0;
		e_codec_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_AUDIO);
		v_Mode_SetNext(modeERROR);
	}
}
```

## Key Improvements

### 1. I2C Bus Recovery
**Added**: `HAL_I2C_Master_Abort_IT(&hi2c5, ADDR_CODEC);`

**Why Critical**:
- Aborts stuck I2C transactions
- Releases I2C bus from locked state
- Prevents cascading failures on I2C5
- Allows system to recover gracefully

**Without this**:
- I2C5 bus can remain locked
- Future I2C transactions fail
- Other devices on I2C5 become inaccessible
- Requires MCU reset to recover

### 2. Enhanced Diagnostics
**Added**: 10 lines of diagnostic logging

**Benefits**:
- Identifies timeout root cause
- Shows I2C hardware state (ISR register)
- Decodes HAL error codes
- Enables faster debugging
- Consistent format across all sensors

**Example output**:
```
[CODEC_TIMEOUT] ES8388 I2C5 timeout (addr=0x22)
  ISR=0x00008003 (BUSY=1, STOPF=0)
  ErrorCode=0x00000004
    - NACK (device not responding)
```

### 3. Standardization
**Before**: 3 different timeout handler patterns
**After**: 1 unified pattern (with special cases documented)

**Benefits**:
- Easier maintenance
- Consistent behavior
- Faster bug identification
- Reduced code review overhead

## Watchdog Safety Analysis

**CRITICAL**: ✅ **NO timeout handlers call `HAL_IWDG_Refresh()`**

This is **CORRECT** and intentional:
- Timeout handlers are called from main loop
- Main loop already refreshes watchdog
- Timeout indicates real problem → should enter ERROR mode
- Refreshing watchdog in timeout handler would mask the issue
- 2-second timeout matches ~2s watchdog window

**If a timeout occurs**:
1. Sensor timeout handler detects 2s timeout
2. I2C transaction aborted
3. System enters ERROR mode
4. Main loop continues (refreshes watchdog in ERROR mode now)
5. LED displays error pattern
6. System stable - no reset

## Build Results

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      167204 B         1 MB     15.95%
          RAM_D1:      104880 B       320 KB     32.01%

Flash: [==        ]  15.9% (used 166468 bytes from 1048576 bytes)
RAM:   [==        ]  24.2% (used 79260 bytes from 327680 bytes)
```

**Flash Usage**: +256 bytes from previous build (diagnostic strings)

## Special Cases Preserved

### GPS Handler (SAM-M10Q)
**Difference**: Does NOT trigger ERROR mode

**Reason**: GPS is optional feature
- Indoor operation is normal without GPS fix
- System continues without GPS
- Only logs warning message
- No ERROR mode transition

**Handler**:
```c
void v_GPS_Tout_Handler(void) {
    if((e_gps_comm == COMM_STAT_BUSY) &&
       _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)) {
        HAL_I2C_Master_Abort_IT(&hi2c3, ADDR_GPS);
        e_gps_comm = COMM_STAT_READY;
        v_printf_poll("GPS: I2C timeout (no response)\r\n");
        // NOTE: NO v_Mode_SetNext(modeERROR)
    }
}
```

### MLX90640 Thermal Camera
**Difference**: Dual timeout condition

**Reason**: Thermal camera has additional error counter for repeated failures

**Handler** (line 116):
```c
if(u32_tempIR_ErrCnt > 10 || ((e_tempIR_evt == COMM_STAT_BUSY) && _b_Tim_Is_OVR(...))) {
    // Timeout OR error count exceeded
}
```

### ESP32 UART
**Difference**: Shorter timeout (1000ms vs 2000ms)

**Reason**: UART has faster response time than I2C
- UART: 115200 baud = fast response
- I2C: 400 kHz + multiple retries = slower
- 1s timeout appropriate for UART

## Error Mode Behavior

When any sensor timeout occurs (except GPS):

1. **Timeout detected** (2 seconds)
2. **Diagnostic logs printed** (I2C state, error codes)
3. **I2C transaction aborted** (bus recovery)
4. **ERROR mode entered** (`v_Mode_SetNext(modeERROR)`)
5. **LED pattern displayed** (5-bit error code)
6. **System remains stable** (watchdog refreshed in ERROR mode)
7. **GPS handler continues** (if needed)

## Testing Recommendations

### Test ES8388 Timeout
1. Disconnect I2C5 SDA/SCL lines while running
2. Observe timeout after 2 seconds
3. Verify diagnostic output
4. Check ERROR mode entry
5. Verify LED pattern displays (modeERR_AUDIO = 0b00110)
6. Reconnect I2C5 and power cycle
7. Verify normal operation resumes

### Verify No I2C Bus Lockup
1. Trigger timeout on each I2C bus (1-5)
2. Power cycle device
3. Verify all I2C devices work normally
4. No stuck transactions or bus lockup

## Related Documentation

- [PROBE_FLOOD_FIX.md](PROBE_FLOOD_FIX.md) - Watchdog and LED error pattern fixes
- [GPS_RESTORATION.md](GPS_RESTORATION.md) - GPS module re-enablement

## Summary

✅ All I2C sensor timeout handlers now follow standardized pattern
✅ ES8388 codec handler fixed to prevent I2C5 bus lockup
✅ Enhanced diagnostics across all sensors
✅ Watchdog safety verified (no handlers call `HAL_IWDG_Refresh()`)
✅ Special cases documented and preserved (GPS, MLX90640, ESP32)
✅ System reliability improved through consistent error handling
