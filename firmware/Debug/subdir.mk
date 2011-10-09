################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../avr_adc.o \
../avr_math.o \
../avr_twi.o \
../avr_uart.o \
../ssc_main.o 

C_SRCS += \
../avr_adc.c \
../avr_math.c \
../avr_spi.c \
../avr_twi.c \
../avr_uart.c \
../ssc_main.c 

OBJS += \
./avr_adc.o \
./avr_math.o \
./avr_spi.o \
./avr_twi.o \
./avr_uart.o \
./ssc_main.o 

C_DEPS += \
./avr_adc.d \
./avr_math.d \
./avr_spi.d \
./avr_twi.d \
./avr_uart.d \
./ssc_main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields -mmcu=atmega8 -DF_CPU=8000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


