################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Drv/src/adc.c \
../User/Drv/src/dac.c \
../User/Drv/src/i2c.c \
../User/Drv/src/io.c \
../User/Drv/src/myDiskio.c \
../User/Drv/src/sd.c \
../User/Drv/src/spi.c \
../User/Drv/src/tim.c \
../User/Drv/src/uart.c 

OBJS += \
./User/Drv/src/adc.o \
./User/Drv/src/dac.o \
./User/Drv/src/i2c.o \
./User/Drv/src/io.o \
./User/Drv/src/myDiskio.o \
./User/Drv/src/sd.o \
./User/Drv/src/spi.o \
./User/Drv/src/tim.o \
./User/Drv/src/uart.o 

C_DEPS += \
./User/Drv/src/adc.d \
./User/Drv/src/dac.d \
./User/Drv/src/i2c.d \
./User/Drv/src/io.d \
./User/Drv/src/myDiskio.d \
./User/Drv/src/sd.d \
./User/Drv/src/spi.d \
./User/Drv/src/tim.d \
./User/Drv/src/uart.d 


# Each subdirectory must supply rules for building sources it contributes
User/Drv/src/%.o User/Drv/src/%.su User/Drv/src/%.cyclo: ../User/Drv/src/%.c User/Drv/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../MINIMP3/core/inc -I../MINIMP3/platform/inc -I../ES8388/core/inc -I../ES8388/platform/inc -I../User/Drv/inc -I../User/Edit/inc -I../User/Lib/inc -I../VL53L0X/core/inc -I../VL53L0X/platform/inc -I../AS6221/core/inc -I../AS6221/platform/inc -I../SK6812/core/inc -I../SK6812/platform/inc -I../ICM42670P/core/inc -I../ICM42670P/platform/inc -I../ADS111x/core/inc -I../ADS111x/platform/inc -I../MLX90640/core/inc -I../MLX90640/platform/inc -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-User-2f-Drv-2f-src

clean-User-2f-Drv-2f-src:
	-$(RM) ./User/Drv/src/adc.cyclo ./User/Drv/src/adc.d ./User/Drv/src/adc.o ./User/Drv/src/adc.su ./User/Drv/src/dac.cyclo ./User/Drv/src/dac.d ./User/Drv/src/dac.o ./User/Drv/src/dac.su ./User/Drv/src/i2c.cyclo ./User/Drv/src/i2c.d ./User/Drv/src/i2c.o ./User/Drv/src/i2c.su ./User/Drv/src/io.cyclo ./User/Drv/src/io.d ./User/Drv/src/io.o ./User/Drv/src/io.su ./User/Drv/src/myDiskio.cyclo ./User/Drv/src/myDiskio.d ./User/Drv/src/myDiskio.o ./User/Drv/src/myDiskio.su ./User/Drv/src/sd.cyclo ./User/Drv/src/sd.d ./User/Drv/src/sd.o ./User/Drv/src/sd.su ./User/Drv/src/spi.cyclo ./User/Drv/src/spi.d ./User/Drv/src/spi.o ./User/Drv/src/spi.su ./User/Drv/src/tim.cyclo ./User/Drv/src/tim.d ./User/Drv/src/tim.o ./User/Drv/src/tim.su ./User/Drv/src/uart.cyclo ./User/Drv/src/uart.d ./User/Drv/src/uart.o ./User/Drv/src/uart.su

.PHONY: clean-User-2f-Drv-2f-src

