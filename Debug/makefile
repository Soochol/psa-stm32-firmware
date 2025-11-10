################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include VL53L0X/platform/src/subdir.mk
-include VL53L0X/core/src/subdir.mk
-include User/Lib/src/subdir.mk
-include User/Edit/src/subdir.mk
-include User/Drv/src/subdir.mk
-include SK6812/platform/src/src/subdir.mk
-include SK6812/platform/src/subdir.mk
-include Middlewares/Third_Party/FatFs/src/option/subdir.mk
-include Middlewares/Third_Party/FatFs/src/subdir.mk
-include MLX90640/platform/src/subdir.mk
-include MLX90640/core/src/subdir.mk
-include MINIMP3/platform/src/subdir.mk
-include ICM42670P/platform/src/subdir.mk
-include ICM42670P/core/src/subdir.mk
-include FATFS/Target/subdir.mk
-include FATFS/App/subdir.mk
-include ES8388/platform/src/subdir.mk
-include ES8388/core/src/subdir.mk
-include Drivers/STM32H7xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include AS6221/platform/src/subdir.mk
-include AS6221/core/src/subdir.mk
-include ADS111x/platform/src/subdir.mk
-include ADS111x/core/src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := WithForce_1.00.34
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
WithForce_1.00.34.elf \

MAP_FILES += \
WithForce_1.00.34.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
WithForce_1.00.34.list \

OBJCOPY_HEX += \
WithForce_1.00.34.hex \


# All Target
all: main-build

# Main-build Target
main-build: WithForce_1.00.34.elf secondary-outputs

# Tool invocations
WithForce_1.00.34.elf WithForce_1.00.34.map: $(OBJS) $(USER_OBJS) C:\Users\spoat\Desktop\Project\P_WithForce\FW\Source\app\1.00.34\STM32H723VGTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "WithForce_1.00.34.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"C:\Users\spoat\Desktop\Project\P_WithForce\FW\Source\app\1.00.34\STM32H723VGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="WithForce_1.00.34.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

WithForce_1.00.34.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "WithForce_1.00.34.list"
	@echo 'Finished building: $@'
	@echo ' '

WithForce_1.00.34.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "WithForce_1.00.34.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) WithForce_1.00.34.elf WithForce_1.00.34.hex WithForce_1.00.34.list WithForce_1.00.34.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
