################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Src/cli_uart.c \
../App/Src/logger.c \
../App/Src/scheduler.c \
../App/Src/sensor.c 

OBJS += \
./App/Src/cli_uart.o \
./App/Src/logger.o \
./App/Src/scheduler.o \
./App/Src/sensor.o 

C_DEPS += \
./App/Src/cli_uart.d \
./App/Src/logger.d \
./App/Src/scheduler.d \
./App/Src/sensor.d 


# Each subdirectory must supply rules for building sources it contributes
App/Src/%.o App/Src/%.su App/Src/%.cyclo: ../App/Src/%.c App/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../App/Inc -I../App/Src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Src

clean-App-2f-Src:
	-$(RM) ./App/Src/cli_uart.cyclo ./App/Src/cli_uart.d ./App/Src/cli_uart.o ./App/Src/cli_uart.su ./App/Src/logger.cyclo ./App/Src/logger.d ./App/Src/logger.o ./App/Src/logger.su ./App/Src/scheduler.cyclo ./App/Src/scheduler.d ./App/Src/scheduler.o ./App/Src/scheduler.su ./App/Src/sensor.cyclo ./App/Src/sensor.d ./App/Src/sensor.o ./App/Src/sensor.su

.PHONY: clean-App-2f-Src

