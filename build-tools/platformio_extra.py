Import("env")
import os

# Get the project directory (where platformio.ini is located)
project_dir = env.subst("$PROJECT_DIR")

# Add all source directories
source_dirs = [
    "Core/Src",
    "Core/Startup",
    "Drivers/STM32H7xx_HAL_Driver/Src",
    "FATFS/App",
    "FATFS/Target",
    "Middlewares/Third_Party/FatFs/src",
    "Middlewares/Third_Party/FatFs/src/option",
    "ADS111x/core/src",
    "ADS111x/platform/src",
    "AS6221/core/src",
    "AS6221/platform/src",
    "ES8388/core/src",
    "ES8388/platform/src",
    "ICM42670P/core/src",
    "ICM42670P/platform/src",
    "MINIMP3/platform/src",
    "MLX90640/core/src",
    "MLX90640/platform/src",
    "SAM_M10Q/core/src",
    "SAM_M10Q/platform/src",
    "SK6812/platform/src",
    "SK6812/platform/src/src",
    "VL53L0X/core/src",
    "VL53L0X/platform/src",
    "User/Drv/src",
    "User/Edit/src",
    "User/Lib/src",
]

# Add source files
for src_dir in source_dirs:
    full_path = os.path.join(project_dir, src_dir)
    if os.path.exists(full_path):
        env.BuildSources(
            os.path.join("$BUILD_DIR", src_dir),
            full_path,
            src_filter=["+<*.c>", "+<*.s>", "+<*.S>"]
        )

print("Extra sources added successfully")
