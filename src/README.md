# Source Directory

This directory is required by PlatformIO but is intentionally kept minimal.

All actual source files are located in their respective directories:
- `Core/Src/` - STM32 HAL initialization and main application
- `Drivers/` - STM32 HAL drivers
- Sensor modules (`ADS111x/`, `AS6221/`, etc.)
- `User/` - Application-specific code

These are automatically included via the `platformio_extra.py` script.
