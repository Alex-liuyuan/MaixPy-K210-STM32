cmake_minimum_required(VERSION 3.20)

# 当前真实产品主线只维护 RT-Thread Nano + STM32F407。
# K210 依赖已经在仓库内完成 SDK/toolchain staging，但 RT-Thread Nano BSP 尚未落地。
set(PLATFORM "rtthread" CACHE STRING "Target platform: rtthread|stm32f407|k210")

if(NOT (PLATFORM STREQUAL "rtthread" OR PLATFORM STREQUAL "stm32f407" OR PLATFORM STREQUAL "k210"))
    message(FATAL_ERROR "Unsupported PLATFORM '${PLATFORM}'; supported: rtthread, stm32f407, k210")
endif()

if(PLATFORM STREQUAL "k210")
    message(FATAL_ERROR
        "PLATFORM=k210 is maintained as a separate experimental build target.\n"
        "Use 'python3 project.py build -p k210' or configure from the 'platforms/k210/' directory directly.\n"
        "The top-level CMake project remains STM32-focused.")
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake" CACHE FILEPATH "" FORCE)
include("${CMAKE_SOURCE_DIR}/cmake/stm32f407.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/rtthread.cmake")

project(SYSU_AIOTOS C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("${CMAKE_SOURCE_DIR}/cmake/micropython.cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(MAIX_FIRMWARE_BASENAME "SYSU_AIOTOS_rtthread")
set(MAIX_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/platforms/stm32/linker/stm32f407_flash.ld")
set(MAIX_RUNTIME_BUNDLE_DIR "${CMAKE_SOURCE_DIR}/runtime_bundle" CACHE PATH "Runtime bundle directory")
set(MAIX_RUNTIME_BUNDLE_HEADER "${CMAKE_BINARY_DIR}/generated/maix_runtime_bundle_autogen.h")
file(GLOB_RECURSE MAIX_RUNTIME_BUNDLE_INPUTS CONFIGURE_DEPENDS "${MAIX_RUNTIME_BUNDLE_DIR}/*")

add_custom_command(
    OUTPUT "${MAIX_RUNTIME_BUNDLE_HEADER}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/runtime_bundle.py"
            generate
            --bundle-dir "${MAIX_RUNTIME_BUNDLE_DIR}"
            --output "${MAIX_RUNTIME_BUNDLE_HEADER}"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/runtime_bundle.py" ${MAIX_RUNTIME_BUNDLE_INPUTS}
    COMMENT "生成 SYSU_AIOTOS 运行时资源 bundle"
)
add_custom_target(maix_runtime_bundle DEPENDS "${MAIX_RUNTIME_BUNDLE_HEADER}")
if(IS_ABSOLUTE "${STARTUP_FILE}")
    set(MAIX_STARTUP_FILE "${STARTUP_FILE}")
else()
    set(MAIX_STARTUP_FILE "${CMAKE_SOURCE_DIR}/platforms/stm32/startup/${STARTUP_FILE}")
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
add_subdirectory(components/hal)
add_subdirectory(components/drivers/stm32)

add_subdirectory(
    third_party/rtthread-nano/rt-thread/bsp/stm32f407-msh/Middlewares/Third_Party/RealThread_RTOS
    rtos
)
include(cmake/tflite_micro.cmake)

add_executable(maix_hal_module ${MAIX_STARTUP_FILE})
target_sources(maix_hal_module PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime/maix_ai_backend.c
    ${CMAKE_SOURCE_DIR}/runtime/maix_runtime_app.c
    ${CMAKE_SOURCE_DIR}/runtime/maix_runtime_fs.c
    ${CMAKE_SOURCE_DIR}/runtime/maix_runtime_python.c
    ${CMAKE_SOURCE_DIR}/runtime/maix_runtime_model.c
    ${CMAKE_SOURCE_DIR}/components/drivers/stm32/stm32_syscalls.c
)
target_include_directories(maix_hal_module PRIVATE
    ${CMAKE_SOURCE_DIR}/runtime
    ${CMAKE_BINARY_DIR}/generated
)
add_dependencies(maix_hal_module maix_runtime_bundle)

target_link_libraries(maix_hal_module PRIVATE
    stm32_drivers
    rtthread_nano
    micropython_port
    maix_hal
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
