################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/SHT35/sht3x.c 

OBJS += \
./Drivers/BSP/SHT35/sht3x.o 

C_DEPS += \
./Drivers/BSP/SHT35/sht3x.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/SHT35/%.o Drivers/BSP/SHT35/%.su: ../Drivers/BSP/SHT35/%.c Drivers/BSP/SHT35/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L152xC -c -I../Core/Inc -I../Drivers/STM32L1xx_HAL_Driver/Inc -I../Drivers/STM32L1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L1xx/Include -I../Drivers/CMSIS/Include -I"/home/dnk067/workspace/SHT35_ALE_I2C/Drivers/BSP/SHT35" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-SHT35

clean-Drivers-2f-BSP-2f-SHT35:
	-$(RM) ./Drivers/BSP/SHT35/sht3x.d ./Drivers/BSP/SHT35/sht3x.o ./Drivers/BSP/SHT35/sht3x.su

.PHONY: clean-Drivers-2f-BSP-2f-SHT35

