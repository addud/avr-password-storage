################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include usbdrv/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(NC_DEPS)),)
-include $(NC_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
keyboard.lss \

FLASH_IMAGE += \
keyboard.hex \

SIZEDUMMY += \
sizedummy \


# All Target
all: keyboard.elf secondary-outputs

# Tool invocations
keyboard.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,keyboard.map -mmcu=atmega16 -o "keyboard.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

keyboard.lss: keyboard.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S keyboard.elf  >"keyboard.lss"
	@echo 'Finished building: $@'
	@echo ' '

keyboard.hex: keyboard.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex keyboard.elf  "keyboard.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: keyboard.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega16 keyboard.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(FLASH_IMAGE)$(ELFS)$(NC_DEPS)$(LSS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) keyboard.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
