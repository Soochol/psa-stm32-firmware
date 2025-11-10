################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Edit/src/comm_dbg.c \
../User/Edit/src/comm_esp.c \
../User/Edit/src/key.c \
../User/Edit/src/mode.c \
../User/Edit/src/quaternion_mahony.c 

OBJS += \
./User/Edit/src/comm_dbg.o \
./User/Edit/src/comm_esp.o \
./User/Edit/src/key.o \
./User/Edit/src/mode.o \
./User/Edit/src/quaternion_mahony.o 

C_DEPS += \
./User/Edit/src/comm_dbg.d \
./User/Edit/src/comm_esp.d \
./User/Edit/src/key.d \
./User/Edit/src/mode.d \
./User/Edit/src/quaternion_mahony.d 


# Each subdirectory must supply rules for building sources it contributes
User/Edit/src/%.o User/Edit/src/%.su User/Edit/src/%.cyclo: ../User/Edit/src/%.c User/Edit/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../MINIMP3/core/inc -I../MINIMP3/platform/inc -I../ES8388/core/inc -I../ES8388/platform/inc -I../User/Drv/inc -I../User/Edit/inc -I../User/Lib/inc -I../VL53L0X/core/inc -I../VL53L0X/platform/inc -I../AS6221/core/inc -I../AS6221/platform/inc -I../SK6812/core/inc -I../SK6812/platform/inc -I../ICM42670P/core/inc -I../ICM42670P/platform/inc -I../ADS111x/core/inc -I../ADS111x/platform/inc -I../MLX90640/core/inc -I../MLX90640/platform/inc -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-User-2f-Edit-2f-src

clean-User-2f-Edit-2f-src:
	-$(RM) ./User/Edit/src/comm_dbg.cyclo ./User/Edit/src/comm_dbg.d ./User/Edit/src/comm_dbg.o ./User/Edit/src/comm_dbg.su ./User/Edit/src/comm_esp.cyclo ./User/Edit/src/comm_esp.d ./User/Edit/src/comm_esp.o ./User/Edit/src/comm_esp.su ./User/Edit/src/key.cyclo ./User/Edit/src/key.d ./User/Edit/src/key.o ./User/Edit/src/key.su ./User/Edit/src/mode.cyclo ./User/Edit/src/mode.d ./User/Edit/src/mode.o ./User/Edit/src/mode.su ./User/Edit/src/quaternion_mahony.cyclo ./User/Edit/src/quaternion_mahony.d ./User/Edit/src/quaternion_mahony.o ./User/Edit/src/quaternion_mahony.su

.PHONY: clean-User-2f-Edit-2f-src

