cmake_minimum_required(VERSION 3.20)

set(HAL_SRCS
    hal_common.c
    hal_gpio.c
    hal_spi.c
    hal_i2c.c
    hal_uart.c
    hal_pwm.c
    hal_adc.c
    hal_display.c
    hal_camera.c
    hal_audio.c
)

add_library(maix_hal STATIC ${HAL_SRCS})

target_include_directories(maix_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(maix_hal PUBLIC)
