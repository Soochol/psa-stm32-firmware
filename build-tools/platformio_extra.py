Import("env")
import os
import re

# Get the project root directory from PlatformIO environment
project_dir = env.subst("$PROJECT_DIR")

# Read MODE_IMU_USED from mode.h
def read_define(header_path, define_name):
    try:
        with open(header_path, "r") as f:
            for line in f:
                m = re.match(r"^\s*#define\s+" + define_name + r"\s+(\d+)", line)
                if m:
                    return int(m.group(1))
    except FileNotFoundError:
        pass
    return 1  # default enabled

mode_imu_used = read_define(
    os.path.join(project_dir, "User", "Edit", "inc", "mode.h"), "MODE_IMU_USED"
)

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
    "MINIMP3/platform/src",
    "MINIMP3/flash_audio/src",
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
    "RTT/src",
]

if mode_imu_used:
    source_dirs += [
        "ICM42670P/core/src",
        "ICM42670P/platform/src",
    ]

# Add source files
for src_dir in source_dirs:
    full_path = os.path.join(project_dir, src_dir)
    if os.path.exists(full_path):
        env.BuildSources(
            os.path.join("$BUILD_DIR", src_dir),
            os.path.join("$PROJECT_DIR", src_dir),
            src_filter=["+<*.c>", "+<*.s>", "+<*.S>"]
        )

print("Extra sources added successfully")
