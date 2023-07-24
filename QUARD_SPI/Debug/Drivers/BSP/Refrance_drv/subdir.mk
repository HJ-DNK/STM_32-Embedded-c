################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.c 

OBJS += \
./Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.o 

C_DEPS += \
./Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Refrance_drv/%.o Drivers/BSP/Refrance_drv/%.su: ../Drivers/BSP/Refrance_drv/%.c Drivers/BSP/Refrance_drv/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L496xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/home/dnk067/workspace/QUARD_SPI/Drivers" -I"/home/dnk067/workspace/QUARD_SPI/Drivers/BSP" -I"/home/dnk067/workspace/QUARD_SPI/Drivers/BSP/Refrance_drv" -I"/home/dnk067/workspace/QUARD_SPI/Core/Src" -I"/home/dnk067/workspace/QUARD_SPI/Drivers/BSP/Flash" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Refrance_drv

clean-Drivers-2f-BSP-2f-Refrance_drv:
	-$(RM) ./Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.d ./Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.o ./Drivers/BSP/Refrance_drv/MX25L_QSPI_Flash.su

.PHONY: clean-Drivers-2f-BSP-2f-Refrance_drv

