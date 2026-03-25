cmake_minimum_required(VERSION 3.20)

# 当前产品主线只维护 RT-Thread Nano + STM32F407
set(PLATFORM "rtthread" CACHE STRING "Target platform: rtthread")

if(NOT PLATFORM STREQUAL "rtthread")
    message(FATAL_ERROR "Only PLATFORM=rtthread is supported in the current product branch")
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake" CACHE FILEPATH "" FORCE)
include("${CMAKE_SOURCE_DIR}/cmake/stm32f407.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/rtthread.cmake")

project(MaixPy_Nano_RTThread C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(MAIX_FIRMWARE_BASENAME "MaixPy_rtthread")
set(MAIX_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/stm32f407_flash.ld")
if(IS_ABSOLUTE "${STARTUP_FILE}")
    set(MAIX_STARTUP_FILE "${STARTUP_FILE}")
else()
    set(MAIX_STARTUP_FILE "${CMAKE_SOURCE_DIR}/startup/${STARTUP_FILE}")
endif()

if(DEFINED CPU_FLAGS AND NOT CPU_FLAGS STREQUAL "")
    string(REPLACE "--specs=nano.specs" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "--specs=nano.specs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${CPU_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS}")
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${CPU_FLAGS} -x assembler-with-cpp")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CPU_FLAGS}")
endif()

# 公共头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/components/hal
    ${CMAKE_SOURCE_DIR}/components/drivers/stm32
)

# 子目录
add_subdirectory(components/drivers/stm32)

add_subdirectory(
    rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Middlewares/Third_Party/RealThread_RTOS
    rtos
)
include(cmake/tflite_micro.cmake)

add_executable(maix_hal_module ${MAIX_STARTUP_FILE})
target_sources(maix_hal_module PRIVATE
    ${CMAKE_SOURCE_DIR}/app/maix_runtime_app.c
    ${CMAKE_SOURCE_DIR}/components/drivers/stm32/stm32_syscalls.c
)
target_include_directories(maix_hal_module PRIVATE
    ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(maix_hal_module PRIVATE
    stm32_drivers
    rtthread_nano
    tflite_micro
)

set_target_properties(maix_hal_module PROPERTIES
    LINKER_LANGUAGE C
    SUFFIX ".elf"
    OUTPUT_NAME "${MAIX_FIRMWARE_BASENAME}"
)

target_link_options(maix_hal_module PRIVATE
    -T${MAIX_LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${MAIX_FIRMWARE_BASENAME}.map
)

add_custom_command(TARGET maix_hal_module POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:maix_hal_module>
            ${CMAKE_BINARY_DIR}/${MAIX_FIRMWARE_BASENAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:maix_hal_module>
            ${CMAKE_BINARY_DIR}/${MAIX_FIRMWARE_BASENAME}.bin
    COMMENT "生成 HEX/BIN 固件"
)
