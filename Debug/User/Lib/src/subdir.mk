################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Lib/src/filter.c \
../User/Lib/src/lib_ascii.c \
../User/Lib/src/lib_buf.c \
../User/Lib/src/lib_buzz.c \
../User/Lib/src/lib_commbuf.c \
../User/Lib/src/lib_console.c \
../User/Lib/src/lib_dim.c \
../User/Lib/src/lib_drgb.c \
../User/Lib/src/lib_fir.c \
../User/Lib/src/lib_iir.c \
../User/Lib/src/lib_key.c \
../User/Lib/src/lib_lpf.c \
../User/Lib/src/lib_math.c \
../User/Lib/src/lib_mode.c \
../User/Lib/src/lib_peakfind.c \
../User/Lib/src/lib_pid.c \
../User/Lib/src/lib_ringbuf.c \
../User/Lib/src/lib_savgol.c \
../User/Lib/src/lib_ssf.c \
../User/Lib/src/lib_steinhart.c \
../User/Lib/src/lib_tim.c 

OBJS += \
./User/Lib/src/filter.o \
./User/Lib/src/lib_ascii.o \
./User/Lib/src/lib_buf.o \
./User/Lib/src/lib_buzz.o \
./User/Lib/src/lib_commbuf.o \
./User/Lib/src/lib_console.o \
./User/Lib/src/lib_dim.o \
./User/Lib/src/lib_drgb.o \
./User/Lib/src/lib_fir.o \
./User/Lib/src/lib_iir.o \
./User/Lib/src/lib_key.o \
./User/Lib/src/lib_lpf.o \
./User/Lib/src/lib_math.o \
./User/Lib/src/lib_mode.o \
./User/Lib/src/lib_peakfind.o \
./User/Lib/src/lib_pid.o \
./User/Lib/src/lib_ringbuf.o \
./User/Lib/src/lib_savgol.o \
./User/Lib/src/lib_ssf.o \
./User/Lib/src/lib_steinhart.o \
./User/Lib/src/lib_tim.o 

C_DEPS += \
./User/Lib/src/filter.d \
./User/Lib/src/lib_ascii.d \
./User/Lib/src/lib_buf.d \
./User/Lib/src/lib_buzz.d \
./User/Lib/src/lib_commbuf.d \
./User/Lib/src/lib_console.d \
./User/Lib/src/lib_dim.d \
./User/Lib/src/lib_drgb.d \
./User/Lib/src/lib_fir.d \
./User/Lib/src/lib_iir.d \
./User/Lib/src/lib_key.d \
./User/Lib/src/lib_lpf.d \
./User/Lib/src/lib_math.d \
./User/Lib/src/lib_mode.d \
./User/Lib/src/lib_peakfind.d \
./User/Lib/src/lib_pid.d \
./User/Lib/src/lib_ringbuf.d \
./User/Lib/src/lib_savgol.d \
./User/Lib/src/lib_ssf.d \
./User/Lib/src/lib_steinhart.d \
./User/Lib/src/lib_tim.d 


# Each subdirectory must supply rules for building sources it contributes
User/Lib/src/%.o User/Lib/src/%.su User/Lib/src/%.cyclo: ../User/Lib/src/%.c User/Lib/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I../MINIMP3/core/inc -I../MINIMP3/platform/inc -I../ES8388/core/inc -I../ES8388/platform/inc -I../User/Drv/inc -I../User/Edit/inc -I../User/Lib/inc -I../VL53L0X/core/inc -I../VL53L0X/platform/inc -I../AS6221/core/inc -I../AS6221/platform/inc -I../SK6812/core/inc -I../SK6812/platform/inc -I../ICM42670P/core/inc -I../ICM42670P/platform/inc -I../ADS111x/core/inc -I../ADS111x/platform/inc -I../MLX90640/core/inc -I../MLX90640/platform/inc -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-User-2f-Lib-2f-src

clean-User-2f-Lib-2f-src:
	-$(RM) ./User/Lib/src/filter.cyclo ./User/Lib/src/filter.d ./User/Lib/src/filter.o ./User/Lib/src/filter.su ./User/Lib/src/lib_ascii.cyclo ./User/Lib/src/lib_ascii.d ./User/Lib/src/lib_ascii.o ./User/Lib/src/lib_ascii.su ./User/Lib/src/lib_buf.cyclo ./User/Lib/src/lib_buf.d ./User/Lib/src/lib_buf.o ./User/Lib/src/lib_buf.su ./User/Lib/src/lib_buzz.cyclo ./User/Lib/src/lib_buzz.d ./User/Lib/src/lib_buzz.o ./User/Lib/src/lib_buzz.su ./User/Lib/src/lib_commbuf.cyclo ./User/Lib/src/lib_commbuf.d ./User/Lib/src/lib_commbuf.o ./User/Lib/src/lib_commbuf.su ./User/Lib/src/lib_console.cyclo ./User/Lib/src/lib_console.d ./User/Lib/src/lib_console.o ./User/Lib/src/lib_console.su ./User/Lib/src/lib_dim.cyclo ./User/Lib/src/lib_dim.d ./User/Lib/src/lib_dim.o ./User/Lib/src/lib_dim.su ./User/Lib/src/lib_drgb.cyclo ./User/Lib/src/lib_drgb.d ./User/Lib/src/lib_drgb.o ./User/Lib/src/lib_drgb.su ./User/Lib/src/lib_fir.cyclo ./User/Lib/src/lib_fir.d ./User/Lib/src/lib_fir.o ./User/Lib/src/lib_fir.su ./User/Lib/src/lib_iir.cyclo ./User/Lib/src/lib_iir.d ./User/Lib/src/lib_iir.o ./User/Lib/src/lib_iir.su ./User/Lib/src/lib_key.cyclo ./User/Lib/src/lib_key.d ./User/Lib/src/lib_key.o ./User/Lib/src/lib_key.su ./User/Lib/src/lib_lpf.cyclo ./User/Lib/src/lib_lpf.d ./User/Lib/src/lib_lpf.o ./User/Lib/src/lib_lpf.su ./User/Lib/src/lib_math.cyclo ./User/Lib/src/lib_math.d ./User/Lib/src/lib_math.o ./User/Lib/src/lib_math.su ./User/Lib/src/lib_mode.cyclo ./User/Lib/src/lib_mode.d ./User/Lib/src/lib_mode.o ./User/Lib/src/lib_mode.su ./User/Lib/src/lib_peakfind.cyclo ./User/Lib/src/lib_peakfind.d ./User/Lib/src/lib_peakfind.o ./User/Lib/src/lib_peakfind.su ./User/Lib/src/lib_pid.cyclo ./User/Lib/src/lib_pid.d ./User/Lib/src/lib_pid.o ./User/Lib/src/lib_pid.su ./User/Lib/src/lib_ringbuf.cyclo ./User/Lib/src/lib_ringbuf.d ./User/Lib/src/lib_ringbuf.o ./User/Lib/src/lib_ringbuf.su ./User/Lib/src/lib_savgol.cyclo ./User/Lib/src/lib_savgol.d ./User/Lib/src/lib_savgol.o ./User/Lib/src/lib_savgol.su ./User/Lib/src/lib_ssf.cyclo ./User/Lib/src/lib_ssf.d ./User/Lib/src/lib_ssf.o ./User/Lib/src/lib_ssf.su ./User/Lib/src/lib_steinhart.cyclo ./User/Lib/src/lib_steinhart.d ./User/Lib/src/lib_steinhart.o ./User/Lib/src/lib_steinhart.su ./User/Lib/src/lib_tim.cyclo ./User/Lib/src/lib_tim.d ./User/Lib/src/lib_tim.o ./User/Lib/src/lib_tim.su

.PHONY: clean-User-2f-Lib-2f-src

