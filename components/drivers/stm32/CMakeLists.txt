cmake_minimum_required(VERSION 3.20)

set(DRIVER_SRCS
    stm32_hal.c
    stm32_gpio_irq.c
    stm32_spi.c
    stm32_i2c.c
    stm32_uart.c
    stm32_dcmi.c
    stm32_lcd.c
    stm32_pwm.c
    stm32_adc.c
)

set(STM32_VENDOR_SRCS "")
set(STM32_VENDOR_INCLUDES "")

set(STM32_F407_BSP_DIR
    "${CMAKE_SOURCE_DIR}/third_party/rtthread-nano/rt-thread/bsp/stm32f407-msh")

if((PLATFORM STREQUAL "rtthread" OR PLATFORM STREQUAL "stm32f407")
   AND EXISTS "${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h")
    list(APPEND STM32_VENDOR_INCLUDES
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
        ${STM32_F407_BSP_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
        ${STM32_F407_BSP_DIR}/Drivers/CMSIS/Include
        ${STM32_F407_BSP_DIR}/Core/Inc
        ${STM32_F407_BSP_DIR}/RT-Thread
    )
    if(DEFINED RTOS_DIR)
        list(APPEND STM32_VENDOR_INCLUDES
            ${RTOS_DIR}/include
            ${RTOS_DIR}/components/finsh
            ${RTOS_DIR}/finsh
        )
    endif()
    list(APPEND STM32_VENDOR_SRCS
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dcmi.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
        ${STM32_F407_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
        ${STM32_F407_BSP_DIR}/Core/Src/main.c
        ${STM32_F407_BSP_DIR}/Core/Src/stm32f4xx_hal_msp.c
        ${STM32_F407_BSP_DIR}/Core/Src/stm32f4xx_it.c
        ${STM32_F407_BSP_DIR}/Core/Src/system_stm32f4xx.c
    )
endif()

add_library(stm32_drivers STATIC ${DRIVER_SRCS})

target_sources(stm32_drivers PRIVATE ${STM32_VENDOR_SRCS})

target_include_directories(stm32_drivers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/hal
    ${CMAKE_SOURCE_DIR}/runtime
    ${STM32_VENDOR_INCLUDES}
)

# STM32 HAL库路径（用户需提供或通过 -DSTM32_HAL_DIR= 指定）
if(DEFINED STM32_HAL_DIR)
    target_include_directories(stm32_drivers PUBLIC ${STM32_HAL_DIR}/Inc)
endif()

target_compile_definitions(stm32_drivers PUBLIC
    HAL_ADC_MODULE_ENABLED
    HAL_DCMI_MODULE_ENABLED
    HAL_I2C_MODULE_ENABLED
    HAL_SPI_MODULE_ENABLED
    HAL_TIM_MODULE_ENABLED
    CONFIG_PLATFORM_STM32F407
)
