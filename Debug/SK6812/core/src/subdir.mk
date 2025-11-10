################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SK6812/core/src/sk6812_api.c 

OBJS += \
./SK6812/core/src/sk6812_api.o 

C_DEPS += \
./SK6812/core/src/sk6812_api.d 


# Each subdirectory must supply rules for building sources it contributes
SK6812/core/src/%.o SK6812/core/src/%.su SK6812/core/src/%.cyclo: ../SK6812/core/src/%.c SK6812/core/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../MINIMP3/core/inc -I../MINIMP3/platform/inc -I../ES8388/core/inc -I../ES8388/platform/inc -I../User/Drv/inc -I../User/Edit/inc -I../User/Lib/inc -I../VL53L0X/core/inc -I../VL53L0X/platform/inc -I../AS6221/core/inc -I../AS6221/platform/inc -I../SK6812/core/inc -I../SK6812/platform/inc -I../ICM42670P/core/inc -I../ICM42670P/platform/inc -I../ADS111x/core/inc -I../ADS111x/platform/inc -I../MLX90640/core/inc -I../MLX90640/platform/inc -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-SK6812-2f-core-2f-src

clean-SK6812-2f-core-2f-src:
	-$(RM) ./SK6812/core/src/sk6812_api.cyclo ./SK6812/core/src/sk6812_api.d ./SK6812/core/src/sk6812_api.o ./SK6812/core/src/sk6812_api.su

.PHONY: clean-SK6812-2f-core-2f-src

