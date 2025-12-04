# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 🎯 Claude Code Workflow Guidelines

**IMPORTANT**: When working on this project, you MUST follow the enhanced workflow defined in [.claude/commands/ss-enhanced.md](../.claude/commands/ss-enhanced.md).

This command provides:

- **Context-aware task analysis** - Automatically loads relevant memories and documentation
- **Intelligent command selection** - Domain-filtered `/sc:` commands for embedded systems
- **Parallel execution optimization** - Maximizes efficiency with parallel tool coordination
- **Session context recovery** - Prevents redundant analysis by reusing previous work
- **Memory management protocol** - Structured documentation for future sessions

**Key Workflow Phases**:

1. **Phase 0**: Load relevant memories from `.serena/memories/` to avoid redundant work
2. **Phase 1**: Analyze task domain (embedded), complexity, and scope
3. **Phase 2**: Select appropriate `/sc:` commands or specialized agents
4. **Phase 3**: Expand context (CLAUDE.md, dependencies, related code) if needed
5. **Phase 4**: Execute with parallel tool coordination for optimal performance
6. **Phase 5**: Save findings to memory using structured format (`.templates/memory_template.md`)

**Use `/sc:` commands for embedded tasks**:

- `/sc:implement --type driver|service` - Peripheral drivers, RTOS tasks
- `/sc:analyze --focus performance|security` - Firmware analysis, memory safety
- `/sc:troubleshoot --type build|runtime` - Build errors, HAL issues
- `/sc:test --type unit|integration` - Embedded unit tests
- `/sc:document --type api|guide` - Driver documentation

**Refer to [.claude/commands/ss-enhanced.md](.claude/commands/ss-enhanced.md) for complete details.**

## Project Overview

This is an embedded firmware project for the STM32H723VGTx microcontroller, targeting a product called "WithForce" (version 1.00.34). The device appears to be a therapeutic/wellness device with heating, cooling, force sensing, audio playback, and multiple sensor capabilities.

## Target Hardware

- **MCU**: STM32H723VGTx (ARM Cortex-M7, 192 MHz)
- **FPU**: FPv5-D16 with hard float ABI
- **Development Environment**: STM32CubeIDE (Eclipse-based)
- **Toolchain**: GNU ARM Embedded Toolchain (arm-none-eabi-gcc)

## Build System

### Building the Project

The project uses STM32CubeIDE's managed build system:

```bash
# Build Debug configuration (from Debug directory)
cd Debug
make all

# Parallel build (faster)
make -j4

# Clean build
make clean
```

**PlatformIO Support**: A `platformio.ini` configuration file is included and **fully functional**. Both STM32CubeIDE and PlatformIO can be used for building this project.

```bash
# Build with PlatformIO
pio run -e debug

# Build and upload
pio run -t upload

# Open serial monitor
pio device monitor
```

See [VSCODE_SETUP.md](VSCODE_SETUP.md) for complete VS Code + PlatformIO development setup.

The build produces:
- `WithForce_1.00.34.elf` - Main executable
- `WithForce_1.00.34.hex` - Intel HEX format for flashing
- `WithForce_1.00.34.map` - Memory map file
- `WithForce_1.00.34.list` - Disassembly listing

### Build Configurations

- **Debug**: Optimization level O1, debug symbols enabled
- **Release**: Optimization level Os (size), no debug info

### Key Build Defines

```c
DEBUG                // Debug build only
USE_PWR_LDO_SUPPLY  // Power supply configuration
USE_HAL_DRIVER      // ST HAL library
STM32H723xx         // MCU family
```

## Code Architecture

### Directory Structure

```
├── Core/               # STM32 HAL initialization and main loop
│   ├── Inc/           # Core headers
│   └── Src/           # main.c, interrupts, system init
├── Drivers/           # STM32 HAL and CMSIS drivers
├── User/              # Application-specific code
│   ├── Drv/          # Hardware driver wrappers (ADC, I2C, UART, etc.)
│   ├── Edit/         # Application logic (mode control, communication)
│   └── Lib/          # Utility libraries (filters, math, ring buffers)
├── Middlewares/       # FatFS filesystem middleware
├── FATFS/            # FatFS application layer
├── [Sensor Modules]   # Individual sensor driver modules (see below)
```

### Sensor/Peripheral Modules

Each sensor follows a **core/platform split architecture**:
- `*/core/` - Hardware-agnostic driver logic
- `*/platform/` - STM32-specific platform integration

Supported modules:
- **ADS111x**: ADC for force sensing (FSR)
- **AS6221**: Temperature sensors (internal/external)
- **ES8388**: Audio codec
- **ICM42670P**: 6-axis IMU (gyroscope/accelerometer)
- **MLX90640**: IR thermal camera array
- **VL53L0X**: Time-of-flight distance sensors
- **SK6812**: Digital RGB LEDs
- **MINIMP3**: MP3 decoder library

### Application Flow

1. **Initialization** ([main.c:152-248](Core/Src/main.c#L152-L248)):
   - MPU configuration for cache regions
   - System clock (192 MHz from PLL)
   - Enable I-Cache and D-Cache
   - Initialize all peripherals
   - Mount SD card and check for MP3 files
   - Enable 12V power rail

2. **Main Loop** ([main.c:252-290](Core/Src/main.c#L252-L290)):
   - Refresh watchdog (IWDG)
   - Process ADC readings
   - Handle UART communication
   - Handle key/button input
   - Execute mode state machine
   - Update RGB LED outputs

### Mode State Machine

The device operates through a state machine defined in [User/Edit/inc/mode.h](User/Edit/inc/mode.h):

**States**:
- `modeBOOTING` - Initial power-on state
- `modeHEALING` - Initialization/self-test phase
- `modeWAITING` - Idle, waiting for user
- `modeFORCE_UP` - Heating phase (target: 52°C)
- `modeFORCE_ON` - Active treatment (60s timeout)
- `modeFORCE_DOWN` - Cool-down phase (30s timeout)
- `modeSLEEP` - Low-power sleep mode (38°C)
- `modeOFF` - Device off
- `modeERROR` - Error state
- `modeTEST` - Factory test mode

**Key Parameters**:
- Temperature limits: 38°C (sleep) to 70°C (max)
- Timeouts: 10s (force-up), 60s (force-on), 30s (force-down)
- PID control for heater: P=100, I=5, D=0.1

### Memory Regions

The firmware uses MPU to configure non-cacheable memory regions for DMA:
- **Region 1**: 0x30004000 (16 KB) - Non-cacheable SRAM
- **Region 2**: 0x38003000 (4 KB) - Non-cacheable SRAM

### Communication Interfaces

- **I2C1, I2C2, I2C4, I2C5**: Sensor buses (400 kHz Fast Mode)
- **UART1, UART3, UART4, UART5, UART8**: Debug and ESP communication (115200 baud)
- **SAI1**: I2S audio interface for ES8388 codec
- **SDMMC2**: SD card interface (4-bit wide bus)
- **SPI**: Available but usage TBD
- **ADC1, ADC3**: Analog inputs (12-bit, DMA circular mode)
- **DAC1**: Audio output via timer trigger
- **TIM2, TIM4**: PWM for fans, heaters, LEDs

### Watchdog

The Independent Watchdog (IWDG) is enabled with ~2 second timeout. **Must refresh regularly** in main loop or the MCU will reset:

```c
HAL_IWDG_Refresh(&hiwdg1);
```

## Development Guidelines

### When Modifying Peripheral Initialization

**WARNING**: The `.ioc` file ([WithForce_1.00.34.ioc](WithForce_1.00.34.ioc)) is the STM32CubeMX configuration. If regenerated, it will overwrite initialization code between `USER CODE BEGIN` and `USER CODE END` markers will be preserved, but other changes will be lost.

### Adding New Sensors

Follow the existing pattern:
1. Create `[SensorName]/core/` with hardware-agnostic driver
2. Create `[SensorName]/platform/` with STM32 HAL integration
3. Add initialization call in `main.c` USER CODE sections
4. Update build system (will auto-detect in STM32CubeIDE)

### Working with the Mode System

- Use `v_Mode_SetNext(e_modeID_t)` to request state transitions
- State changes are debounced and validated
- Each mode has entry/exit handlers in [User/Edit/src/mode.c](User/Edit/src/mode.c)
- LED colors are defined per-mode in [mode.h](User/Edit/inc/mode.h#L18-L61)

### I2C Bus Recovery

Multiple I2C buses have recovery functions (`v_I2C1_Pin_Deinit()`, etc.) called during initialization to handle bus lockup conditions. This is normal for production embedded systems with external sensors.

### Audio Subsystem

- MP3 files are read from SD card (FatFS)
- Decoded via MINIMP3 library
- Output through ES8388 codec via SAI (I2S)
- Volume control: 0-10 scale (default: 5)
- Can be muted via `v_Mode_Set_Speaker_Mute()`

### Power Management

- 12V rail controlled by `DO_12VA_EN` pin
- Must delay 2s after 12V disable before re-enabling
- Battery voltage monitoring with 3 levels (LV1: 36.4V, LV2: 39.4V, Alert: 32.5V)
- Sleep mode maintains 38°C temperature

## Debugging

### Launch Configurations

Three debug launch configurations are available:
- `WithForce_1.00.31 Debug.launch`
- `WithForce_1.00.32 Debug.launch`
- `WithForce_1.00.34 Debug.launch`

These are STM32CubeIDE debug configurations for ST-Link debugging.

### Test Functions

Several test functions are available but commented out in main loop ([main.c:269-288](Core/Src/main.c#L269-L288)):
- `v_AUDIO_Test()` - Codec and speaker test
- `v_Temp_IR_Test()` - Thermal camera test
- `v_Mode_Sound_Test()` - Audio playback test
- `v_ESP_CmdTest()` - ESP communication test
- `v_Mode_JIG_LED_BD()` - LED board test

Uncomment as needed for hardware validation.

### Important Notes

- **IWDG Enabled**: The watchdog is always running in production builds. Disable or refresh regularly during debugging.
- **Cache Coherency**: DMA buffers must be in non-cacheable regions or manually flushed/invalidated.
- **Float Printf**: Enabled via `-u _printf_float` linker flag (adds ~7 KB to binary).
