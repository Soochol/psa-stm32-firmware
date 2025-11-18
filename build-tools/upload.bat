@echo off
echo Building firmware...
pio run -e debug
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

echo.
echo Uploading firmware...
C:\Users\bless\.platformio\packages\tool-openocd\bin\openocd.exe -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program .pio/build/debug/firmware.bin 0x08000000 reset exit"

echo.
echo Upload complete!
