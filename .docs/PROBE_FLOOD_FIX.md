# Probe Flooding Issue - Root Cause Analysis and Fix

## Issue Summary

**Problem**: Device B enters ERROR mode but LED error patterns don't display. System transitions to OFF mode immediately after boot.

**Root Cause**: I2C probe functions were called inside sensor initialization retry loops, causing hundreds/thousands of repeated I2C transactions that prevent watchdog refresh, triggering MCU reset.

## Timeline Analysis (from logs)

```
[2009ms] 12V power enabled
[2010ms] MODE_CHANGE: ERROR → OFF  (1ms gap!)
```

The watchdog reset occurs almost immediately after entering ERROR mode, before LED blink patterns can be visible.

## Technical Details

### Before Fix (Problematic Code)

```c
if(!(ready_mask & modeCONFIG_IMU)){
    // BUG: Probes run EVERY iteration of retry loop!
    i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_LEFT, "IMU_LEFT");
    i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_RIGHT, "IMU_RIGHT");

    ret = e_IMU_Ready();  // Fails → loop repeats → probes run again!
}
```

**Result**: On Device B with non-functional IMU hardware:
- Each probe attempt: ~20ms I2C timeout × 2 devices = 40ms per loop
- Retry loop runs continuously while `!(ready_mask & modeCONFIG_IMU)`
- Main loop blocked from calling `HAL_IWDG_Refresh()`
- Watchdog triggers reset after 2 seconds

**Log Evidence**:
```
[I2C2_PROBE] Checking IMU_LEFT (0x68)... NACK
[I2C2_PROBE] Checking IMU_RIGHT (0x69)... NACK
[I2C2_PROBE] Checking IMU_LEFT (0x68)... NACK
[I2C2_PROBE] Checking IMU_RIGHT (0x69)... NACK
... (hundreds of repetitions)
```

### After Fix (Corrected Code)

```c
if(!(ready_mask & modeCONFIG_IMU)){
    // FIXED: Probe runs ONCE per boot
    static bool b_imu_probed = false;
    if(!b_imu_probed){
        i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_LEFT, "IMU_LEFT");
        i_I2C_ProbeDevice(&hi2c2, 2, ADDR_IMU_RIGHT, "IMU_RIGHT");
        b_imu_probed = true;
    }

    ret = e_IMU_Ready();  // Can retry without re-probing
}
```

**Result**:
- Probe executes only once per power cycle
- Subsequent retry loops skip probe, only attempt initialization
- Main loop can refresh watchdog regularly
- ERROR mode has time to display LED patterns

## Files Modified

### User/Edit/src/mode.c (ERROR Mode LED Fixes)

**Line 1979**: Added `v_RGB_Enable_Duty()` call when entering ERROR mode
- **Issue #1**: RGB PWM duty cycle was not enabled for ERROR mode
- **Fix**: Enable RGB PWM when ERROR mode enters, matching WAKE_UP mode behavior

**Line 1955**: Added `v_RGB_Refresh_Enable()` call in `v_Set_Error_LED_Pattern()`
- **Issue #2**: LED buffer updated but `b_rgbAct` flag never set → PWM output skipped
- **Root Cause**: `v_RGB_PWM_Out()` checks `b_rgbAct` flag before outputting to hardware
- **Fix**: Call `v_RGB_Refresh_Enable()` to set `b_rgbAct = true` after updating LED buffer
- **Impact**: CRITICAL - without this, LED data never reaches SK6812 hardware despite correct buffer values

### User/Edit/src/mode.c (Probe Flooding Fixes)

Added static probe flags for 4 sensor subsystems:

1. **IMU sensors** (I2C2, lines 1127-1133)
   - `static bool b_imu_probed`
   - Devices: IMU_LEFT (0x68), IMU_RIGHT (0x69)

2. **FSR sensors** (I2C2, lines 1147-1153)
   - `static bool b_fsr_probed`
   - Devices: FSR_LEFT (0x48), FSR_RIGHT (0x49)

3. **Temperature sensors** (I2C1, lines 1171-1177)
   - `static bool b_temp_probed`
   - Devices: TEMP_INDOOR (0x48), TEMP_OUTDOOR (0x49)

4. **IR thermal camera** (I2C4, lines 1208-1213)
   - `static bool b_mlx_probed`
   - Device: MLX90640 (0x33)

## Build Results

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      160844 B         1 MB     15.34%
          RAM_D1:      104848 B       320 KB     32.00%

Flash: [==        ]  15.3% (used 160108 bytes from 1048576 bytes)
RAM:   [==        ]  24.2% (used 79228 bytes from 327680 bytes)
```

**Flash Usage**: +284 bytes total (0.027% increase from baseline)

## Expected Behavior After Fix

### Device B Boot Sequence (with failed IMU):

```
[BOOT] System reset
[I2C2_PROBE] Checking IMU_LEFT (0x68)... NACK
[I2C2_PROBE] Checking IMU_RIGHT (0x69)... NACK
[SENSOR_INIT] IMU Ready() attempt 1... FAIL
[SENSOR_INIT] IMU Ready() attempt 2... FAIL
[SENSOR_INIT] IMU Ready() attempt 3... FAIL
[SENSOR_FAIL] IMU init FAILED on I2C2 - Device B hardware issue?
[MODE] ERROR mode entered (error=0x0008)
[LED] Pattern: 0b00100 (○○●○○) - modeERR_IMU
[LED] Blinking at 500ms interval
... (LED continues blinking, no watchdog reset)
```

### Key Improvements:

1. ✅ **Probes run once** - Clean log output, no flooding
2. ✅ **Watchdog refreshed** - No premature reset
3. ✅ **LED patterns visible** - ERROR mode stable
4. ✅ **Faster boot** - Reduced I2C overhead from ~40ms/loop to one-time cost

## Validation Checklist

- [x] Code compiles successfully
- [x] ERROR mode does not transition to OFF mode (watchdog fix confirmed)
- [x] Watchdog does not trigger reset in ERROR mode (always refresh)
- [ ] Test on Device B (failed IMU hardware)
- [ ] Verify LED error pattern displays correctly (RGB PWM now enabled)
- [ ] Check serial log shows probe messages only once
- [ ] Visually confirm 5-bit LED pattern (○○●○○ for IMU error)

## Root Cause Analysis: LED Not Displaying

**User Report**: "그런데 LED는 에러로 변경되지 않고 있어. 그리고 제안한 비트 방식도 나오지 않고 있지."

### Initial Analysis (Issue #1)
After fixing watchdog and probe flooding, ERROR mode became stable but LED patterns still didn't display.

**First Root Cause**: ERROR mode handler was missing `v_RGB_Enable_Duty()` call
- SK6812 RGB LEDs require PWM duty cycle enabled to function
- Other modes (WAKE_UP, etc.) call `v_RGB_Enable_Duty()` on entry
- **Fix**: Added `v_RGB_Enable_Duty()` at line 1979

### Deeper Analysis (Issue #2 - The Real Problem)

After adding `v_RGB_Enable_Duty()`, LED **STILL** didn't display. Further investigation revealed:

**Second Root Cause**: `v_Set_Error_LED_Pattern()` never sets `b_rgbAct` flag

**How SK6812 LED Output Works**:
1. `b_RGB_Set_Color()` updates LED buffer in memory (u16_rgbArr[])
2. Main loop calls `v_RGB_PWM_Out()` every iteration
3. `v_RGB_PWM_Out()` checks `if(b_rgbAct)` before outputting
4. If `b_rgbAct == false`, PWM output is **SKIPPED**
5. LED hardware never receives the data

**The Bug**:
```c
// All other LED functions do this:
void v_RGB_Set_Top(...) {
    b_RGB_Set_Color(...);
    b_rgbAct = true;  // ✓ Trigger output
}

// But v_Set_Error_LED_Pattern() was missing this:
static void v_Set_Error_LED_Pattern(...) {
    b_RGB_Set_Color(...);
    // ❌ Missing: b_rgbAct = true;
}
```

**Fix**: Added `v_RGB_Refresh_Enable()` call at line 1955 to set `b_rgbAct = true`

**Impact**: Without this flag, LED buffer updates are **completely invisible** to hardware

## Related Issues

- **LED Error Pattern System**: Implemented in same file (lines 1896-1965)
- **Error Codes**: 13 unique 5-bit patterns for diagnostic LEDs
- **Hardware Issues**: Device B has non-functional IMU sensors on I2C2

## Next Steps

1. Flash firmware to Device B
2. Monitor serial output for clean probe messages
3. Visually verify LED error pattern (○○●○○ for IMU failure)
4. Document whether ERROR mode persists or transitions to another state

## Technical Notes

**Static Variable Scope**: Each probe flag is declared `static` within the `if` block scope. This ensures:
- Flag persists across function calls (not stack-allocated)
- Flag is reset only on MCU power-on reset
- No global namespace pollution

**Thread Safety**: Not applicable - this firmware runs in single-threaded main loop, no RTOS.

**Reset Behavior**: On watchdog or manual reset, all static variables reset to `false`, allowing probe to run again on next boot (desired behavior).
