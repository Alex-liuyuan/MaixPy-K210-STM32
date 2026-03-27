cmake_minimum_required(VERSION 3.20)

# 项目根目录（platforms/sim/ 的上两级）
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" ABSOLUTE)

# 工具链必须在 project() 之前设置
set(CMAKE_TOOLCHAIN_FILE "${PROJECT_ROOT}/cmake/toolchain-arm-none-eabi-sim.cmake"
    CACHE FILEPATH "" FORCE)

# 加载源文件/头文件/宏定义变量（ROOT 变量由 sim_stm32f407.cmake 使用）
set(CMAKE_SOURCE_DIR "${PROJECT_ROOT}")
include(${PROJECT_ROOT}/cmake/sim_stm32f407.cmake)

project(SYSU_AIOTOS_sim C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(RTTHREAD_BSP_DIR "${PROJECT_ROOT}/third_party/rtthread-nano/rt-thread/bsp/stm32f407-msh")
set(MAIX_RUNTIME_BUNDLE_DIR "${PROJECT_ROOT}/runtime_bundle" CACHE PATH "Runtime bundle directory")
set(MAIX_RUNTIME_BUNDLE_HEADER "${CMAKE_BINARY_DIR}/generated/maix_runtime_bundle_autogen.h")
file(GLOB_RECURSE MAIX_RUNTIME_BUNDLE_INPUTS CONFIGURE_DEPENDS "${MAIX_RUNTIME_BUNDLE_DIR}/*")
set(MAIX_MPY_ARCH_CFLAGS "${CPU_FLAGS}")
set(MAIX_MPY_INCLUDE_DIRS
    "${PROJECT_ROOT}/runtime"
    "${RTTHREAD_BSP_DIR}/Core/Inc"
    "${RTTHREAD_BSP_DIR}/RT-Thread"
    "${RTTHREAD_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc"
    "${RTTHREAD_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
    "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Core/Include"
    "${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Include"
    "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/include"
    "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/components/finsh"
    "${RTTHREAD_BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS/finsh"
)
include(${PROJECT_ROOT}/cmake/micropython.cmake)
include(${PROJECT_ROOT}/cmake/tflite_micro.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(MAIX_SIM_FIRMWARE_BASENAME "SYSU_AIOTOS_sim")

# CPU 标志（Cortex-M4 + FPU）
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${CPU_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CPU_FLAGS}")

# 固件可执行目标
# 替换 board.c → board_qemu.c（semihosting + 简化时钟配置）
# 排除 main.c（其中的 SystemClock_Config 与 board_qemu.c 冲突）
# 替换启动文件 → startup_stm32f407xx_qemu.s（bl entry 替代 bl main）
list(FILTER ALL_SRCS EXCLUDE REGEX "board\\.c$")
list(FILTER ALL_SRCS EXCLUDE REGEX "main\\.c$")
list(FILTER ALL_SRCS EXCLUDE REGEX "startup_stm32f407xx\\.s$")
list(APPEND ALL_SRCS ${PROJECT_ROOT}/runtime/maix_ai_backend.c)
list(APPEND ALL_SRCS ${PROJECT_ROOT}/runtime/maix_runtime_app.c)
list(APPEND ALL_SRCS ${PROJECT_ROOT}/runtime/maix_runtime_fs.c)
list(APPEND ALL_SRCS ${PROJECT_ROOT}/runtime/maix_runtime_python.c)
list(APPEND ALL_SRCS ${PROJECT_ROOT}/runtime/maix_runtime_model.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/board_qemu.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main_qemu.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sim_syscalls.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f407xx_qemu.s)

add_custom_command(
    OUTPUT "${MAIX_RUNTIME_BUNDLE_HEADER}"
    COMMAND ${Python3_EXECUTABLE} "${PROJECT_ROOT}/tools/runtime_bundle.py"
            generate
            --bundle-dir "${MAIX_RUNTIME_BUNDLE_DIR}"
            --output "${MAIX_RUNTIME_BUNDLE_HEADER}"
    DEPENDS "${PROJECT_ROOT}/tools/runtime_bundle.py" ${MAIX_RUNTIME_BUNDLE_INPUTS}
    COMMENT "生成 SYSU_AIOTOS 运行时资源 bundle"
)
add_custom_target(maix_runtime_bundle DEPENDS "${MAIX_RUNTIME_BUNDLE_HEADER}")

add_executable(SYSU_AIOTOS_sim ${ALL_SRCS})

target_include_directories(SYSU_AIOTOS_sim PRIVATE ${ALL_INCLUDES})
target_include_directories(SYSU_AIOTOS_sim PRIVATE ${PROJECT_ROOT}/runtime ${CMAKE_BINARY_DIR}/generated)
add_dependencies(SYSU_AIOTOS_sim maix_runtime_bundle)

target_compile_definitions(SYSU_AIOTOS_sim PRIVATE ${ALL_DEFS})
target_link_libraries(SYSU_AIOTOS_sim PRIVATE
    micropython_port
    tflite_micro
)

target_compile_options(SYSU_AIOTOS_sim PRIVATE
    -g3
    -O0
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-implicit-fallthrough
)

# 链接脚本
target_link_options(SYSU_AIOTOS_sim PRIVATE
    -T${PROJECT_ROOT}/platforms/stm32/linker/stm32f407_flash.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.map
    -Wl,-eentry
)

set_target_properties(SYSU_AIOTOS_sim PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "${MAIX_SIM_FIRMWARE_BASENAME}"
)

# 生成 bin/hex
find_program(OBJCOPY arm-none-eabi-objcopy)
find_program(SIZE_TOOL arm-none-eabi-size)

add_custom_command(TARGET SYSU_AIOTOS_sim POST_BUILD
    COMMAND ${OBJCOPY} -O ihex   $<TARGET_FILE:SYSU_AIOTOS_sim> ${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.hex
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:SYSU_AIOTOS_sim> ${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.bin
    COMMAND ${SIZE_TOOL} $<TARGET_FILE:SYSU_AIOTOS_sim>
    COMMENT "生成 HEX/BIN，打印固件大小"
)
