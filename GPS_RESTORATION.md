# GPS Module Restoration

## Summary

GPS functionality has been fully restored after being temporarily disabled for Device B hardware testing.

## Changes Made

### 1. GPS Initialization Re-enabled

**File**: [Core/Src/main.c](Core/Src/main.c#L315-L334)

**Before**:
```c
// GPS DISABLED FOR TESTING - Device B GPS module not responding on I2C3
#if 0
  // ... GPS init code ...
#endif // GPS DISABLED
```

**After**:
```c
// GPS initialization after 12V power is stable
#if 1  // GPS ENABLED - SAM-M10Q on I2C3
  // Wait for GPS module to boot (SAM-M10Q needs ~1-2 seconds)
  HAL_Delay(2000);

  HAL_IWDG_Refresh(&hiwdg1);  // Refresh after GPS boot delay

  printf("[POWER] GPS boot delay complete at t=%lu ms (elapsed=%lu ms)\r\n",
         HAL_GetTick(), HAL_GetTick() - t_12v_on);

  // Initialize GPS after 12V power is stable
  v_GPS_Init();
  printf("[POWER] GPS init completed at t=%lu ms\r\n", HAL_GetTick());

  HAL_IWDG_Refresh(&hiwdg1);  // Refresh after GPS init
#endif // GPS initialization
```

### 2. GPS Handler Restored in Main Loop

**File**: [Core/Src/main.c](Core/Src/main.c#L363-L364)

**Before**:
```c
v_Key_Handler();
// GPS DISABLED FOR TESTING
// v_GPS_Handler();
v_Mode_Handler();
```

**After**:
```c
v_Key_Handler();
v_GPS_Handler();
v_GPS_Tout_Handler();  // GPS I2C timeout monitoring
v_Mode_Handler();
```

**Added**: `v_GPS_Tout_Handler()` call for I2C timeout monitoring (was missing before)

## Build Results

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      166948 B         1 MB     15.92%
          RAM_D1:      104880 B       320 KB     32.01%

Flash: [==        ]  15.9% (used 166212 bytes from 1048576 bytes)
RAM:   [==        ]  24.2% (used 79260 bytes from 327680 bytes)
```

**Flash Usage**: +6104 bytes from previous build (GPS code re-enabled)

## GPS Module Details

**Hardware**: SAM-M10Q GNSS Module
- **Interface**: I2C3 (address 0x42)
- **Power**: Requires 12V rail to be enabled
- **Boot Time**: ~1-2 seconds after power-on
- **Protocol**: UBX binary protocol (NMEA disabled)

## GPS Data Flow

1. **Initialization** (main.c):
   - 12V power enabled
   - 2-second boot delay
   - `v_GPS_Init()` configures UBX protocol
   - Disables NMEA, enables UBX-NAV-PVT messages

2. **Runtime Operation** (main loop):
   - `v_GPS_Handler()`: Polls for UBX-NAV-PVT messages (Position, Velocity, Time)
   - `v_GPS_Tout_Handler()`: Monitors I2C timeout (2s limit)
   - Parsed data stored in `_x_GPS_PVT_t` structure

3. **Data Transmission** (comm_esp.c):
   - GPS data sent to ESP32 every 100ms
   - Format: 10 bytes (lat, lon, numSV, fixType)
   - Big-Endian encoding for ESP32 compatibility

## Expected Boot Sequence

```
[POWER] 12V enabled at t=2010 ms
[POWER] GPS boot delay complete at t=4010 ms (elapsed=2000 ms)
[GPS_INIT] Configuring SAM-M10Q on I2C3 (addr=0x42)...
[GPS_INIT] UBX-NAV-PVT enabled, rate=1Hz
[POWER] GPS init completed at t=4250 ms

... (during operation)
╔════════════════════ GPS STATUS [0001] ════════════════════╗
║ STATUS: ○ ACQUIRING                                       ║
║ ---------------------------------------------------------- ║
║ Fix Type:    0 (0=No Fix, 2=2D, 3=3D)                     ║
║ Satellites:   4 (Need 4+ for 3D fix)                      ║
║ Status:      Waiting for satellite lock...                ║
╚════════════════════════════════════════════════════════════╝
```

Once GPS acquires fix (outdoors, clear sky):
```
╔════════════════════ GPS STATUS [0025] ════════════════════╗
║ STATUS: ✓ 3D FIX                                          ║
║ ---------------------------------------------------------- ║
║ Fix Type:    3 (3D Fix)                                   ║
║ Satellites:  8                                            ║
║ Latitude:    37.5665° N                                   ║
║ Longitude:  126.9780° E                                   ║
╚════════════════════════════════════════════════════════════╝
```

## Error Handling

**GPS Timeout** (I2C3):
- Monitored by `v_GPS_Tout_Handler()`
- 2-second timeout limit
- On timeout: I2C transaction aborted, communication state reset

**No GPS Fix**:
- GPS data transmission sends zeros if no fix available
- ESP32 receives: `{lat=0, lon=0, numSV=0, fixType=0}`
- Normal behavior indoors or during acquisition

## Testing Notes

- **Indoor Testing**: GPS will NOT acquire fix (requires open sky view)
- **Acquisition Time**: 30-60 seconds outdoors (cold start)
- **Warm Start**: <10 seconds with valid ephemeris data
- **Device B**: Previous hardware issue may still exist, monitor I2C3 errors

## Related Files

### GPS Platform Code
- [SAM_M10Q/platform/src/sam_m10q_platform.c](SAM_M10Q/platform/src/sam_m10q_platform.c) - GPS driver
- [SAM_M10Q/core/src/sam_m10q_ubx.c](SAM_M10Q/core/src/sam_m10q_ubx.c) - UBX protocol parser

### I2C Integration
- [User/Drv/src/i2c.c](User/Drv/src/i2c.c) - I2C3 callbacks (lines 806-807, 899-900)
- [User/Drv/inc/i2c.h](User/Drv/inc/i2c.h) - I2C API declarations

### Data Transmission
- [User/Edit/src/comm_esp.c](User/Edit/src/comm_esp.c#L643-L678) - GPS data to ESP32

## Validation Checklist

- [x] Code compiles successfully
- [x] GPS initialization enabled
- [x] GPS handler active in main loop
- [x] GPS timeout handler added
- [ ] Test GPS boot sequence (monitor serial log)
- [ ] Test GPS acquisition outdoors (clear sky)
- [ ] Verify GPS data transmitted to ESP32
- [ ] Test I2C3 timeout handling (disconnect GPS module)
