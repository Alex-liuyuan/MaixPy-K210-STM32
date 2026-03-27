cmake_minimum_required(VERSION 3.20)

set(K210_SRCS
    k210_hal.c
    k210_gpio.c
    k210_spi.c
    k210_i2c.c
    k210_uart.c
)

add_library(k210_drivers STATIC ${K210_SRCS})

target_include_directories(k210_drivers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/components/hal)

target_compile_definitions(k210_drivers PUBLIC CONFIG_PLATFORM_K210)
