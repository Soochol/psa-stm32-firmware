# Upload Troubleshooting Guide

## Current Status

**Build:** ✅ Working perfectly with PlatformIO
**Upload:** ❌ Getting `ST-LINK error (DEV_TARGET_CMD_ERR)`

## ST-Link Detection

The ST-Link is properly detected:
- Serial Number: `48FF69064988884908411167`
- Firmware: `V2J29S7`
- Voltage: `3.21V`

This confirms the hardware connection is good.

## Connection Modes Attempted

1. **Under Reset (UR) with Hardware Reset**
   ```
   mode=UR freq=4000
   ```
   Result: Error

2. **HOTPLUG with Hardware Reset**
   ```
   mode=HOTPLUG reset=HWrst freq=4000
   ```
   Result: Warning (conflicting modes) + Error

3. **Normal mode with Software Reset**
   ```
   reset=SWrst freq=4000
   ```
   Result: Error

4. **Under Reset mode only**
   ```
   mode=UR
   ```
   Result: Error

## Possible Causes

1. **Target MCU is running and watchdog is enabled**
   - The firmware has IWDG (watchdog) enabled
   - This can prevent connection in certain modes

2. **RDP (Read Protection) or Flash Security**
   - Device may have protection enabled

3. **Boot mode configuration**
   - BOOT0/BOOT1 pins might need adjustment

4. **Different GUI settings**
   - The STM32CubeProgrammer GUI uses different parameters than what we've tried

## Next Steps

### Option 1: Get GUI Settings (RECOMMENDED)

Please check your STM32CubeProgrammer GUI settings and provide:

1. **Connection Mode:**
   - Normal
   - Under Reset
   - Hot Plug

2. **Reset Mode:**
   - Software reset
   - Hardware reset
   - Core reset

3. **Frequency (Speed):**
   - Current value in kHz

4. **Additional Options:**
   - Any checkboxes enabled (e.g., "Verify after programming", "Run after programming")

### Option 2: Manual Upload Script

Use the provided `upload.bat` script:

```batch
upload.bat
```

This builds and uploads using OpenOCD (alternative programmer).

### Option 3: Direct STM32CubeProgrammer CLI Test

Test directly with exact GUI settings. Example:

```bash
"C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe" -c port=SWD mode=UR freq=4000 -w .pio/build/debug/firmware.bin 0x08000000 -v -rst
```

### Option 4: Use GUI for Now

Continue using STM32CubeProgrammer GUI for uploads while we troubleshoot:

1. Build with PlatformIO: `pio run -e debug`
2. Upload with GUI: Load `.pio/build/debug/firmware.bin` at address `0x08000000`

## STM32_Programmer_CLI Parameters Reference

```
-c port=SWD              # Connection port (SWD or JTAG)
mode=UR                  # Under Reset mode
mode=HOTPLUG             # Hot Plug mode (connect anytime)
reset=HWrst              # Hardware reset
reset=SWrst              # Software reset
reset=CORErst            # Core reset
freq=4000                # Speed in kHz (4000 = 4 MHz)
ap=0                     # Access Point (for multi-core)
-w <file> <addr>         # Write file to address
-v                       # Verify after write
-rst                     # Reset and run after write
```

## Temporary Workaround

Current workflow until fixed:

```bash
# Build with PlatformIO
pio run -e debug

# Upload using GUI
# File: .pio\build\debug\firmware.bin
# Address: 0x08000000
```

## Debug Information

To get more debug info from STM32_Programmer_CLI:

```bash
"C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe" -c port=SWD mode=UR -vb 3
```

The `-vb 3` flag enables verbose logging level 3.
