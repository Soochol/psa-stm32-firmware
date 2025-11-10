################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ADS111x/platform/src/ads111x_platform.c 

OBJS += \
./ADS111x/platform/src/ads111x_platform.o 

C_DEPS += \
./ADS111x/platform/src/ads111x_platform.d 


# Each subdirectory must supply rules for building sources it contributes
ADS111x/platform/src/%.o ADS111x/platform/src/%.su ADS111x/platform/src/%.cyclo: ../ADS111x/platform/src/%.c ADS111x/platform/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../MINIMP3/core/inc -I../MINIMP3/platform/inc -I../ES8388/core/inc -I../ES8388/platform/inc -I../User/Drv/inc -I../User/Edit/inc -I../User/Lib/inc -I../VL53L0X/core/inc -I../VL53L0X/platform/inc -I../AS6221/core/inc -I../AS6221/platform/inc -I../SK6812/core/inc -I../SK6812/platform/inc -I../ICM42670P/core/inc -I../ICM42670P/platform/inc -I../ADS111x/core/inc -I../ADS111x/platform/inc -I../MLX90640/core/inc -I../MLX90640/platform/inc -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ADS111x-2f-platform-2f-src

clean-ADS111x-2f-platform-2f-src:
	-$(RM) ./ADS111x/platform/src/ads111x_platform.cyclo ./ADS111x/platform/src/ads111x_platform.d ./ADS111x/platform/src/ads111x_platform.o ./ADS111x/platform/src/ads111x_platform.su

.PHONY: clean-ADS111x-2f-platform-2f-src

