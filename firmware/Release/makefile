################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include default/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
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
ssc_module.lss \

FLASH_IMAGE += \
ssc_module.hex \

EEPROM_IMAGE += \
ssc_module.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: ssc_module.elf secondary-outputs

# Tool invocations
ssc_module.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,ssc_module.map -mmcu=atmega8 -o"ssc_module.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

ssc_module.lss: ssc_module.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S ssc_module.elf  >"ssc_module.lss"
	@echo 'Finished building: $@'
	@echo ' '

ssc_module.hex: ssc_module.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -O ihex ssc_module.elf  "ssc_module.hex"
	@echo 'Finished building: $@'
	@echo ' '

ssc_module.eep: ssc_module.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex ssc_module.elf  "ssc_module.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: ssc_module.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega8 ssc_module.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ASM_DEPS)$(EEPROM_IMAGE)$(FLASH_IMAGE)$(ELFS)$(LSS)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS) ssc_module.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
