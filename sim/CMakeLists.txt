cmake_minimum_required(VERSION 3.20)

# 项目根目录（sim/ 的上一级）
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" ABSOLUTE)

# 工具链必须在 project() 之前设置
set(CMAKE_TOOLCHAIN_FILE "${PROJECT_ROOT}/cmake/toolchain-arm-none-eabi-sim.cmake"
    CACHE FILEPATH "" FORCE)

# 加载源文件/头文件/宏定义变量（ROOT 变量由 sim_stm32f407.cmake 使用）
set(CMAKE_SOURCE_DIR "${PROJECT_ROOT}")
include(${PROJECT_ROOT}/cmake/sim_stm32f407.cmake)

project(MaixPy_sim C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(MAIX_SIM_FIRMWARE_BASENAME "MaixPy_sim")

# CPU 标志（Cortex-M4 + FPU）
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
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
list(APPEND ALL_SRCS ${PROJECT_ROOT}/app/maix_runtime_app.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/board_qemu.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/main_qemu.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sim_syscalls.c)
list(APPEND ALL_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f407xx_qemu.s)

add_executable(MaixPy_sim ${ALL_SRCS})

target_include_directories(MaixPy_sim PRIVATE ${ALL_INCLUDES})
target_include_directories(MaixPy_sim PRIVATE ${PROJECT_ROOT}/app)

target_compile_definitions(MaixPy_sim PRIVATE ${ALL_DEFS})

target_compile_options(MaixPy_sim PRIVATE
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
target_link_options(MaixPy_sim PRIVATE
    -T${PROJECT_ROOT}/linker/stm32f407_flash.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.map
    -Wl,-eentry
)

set_target_properties(MaixPy_sim PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "${MAIX_SIM_FIRMWARE_BASENAME}"
)

# 生成 bin/hex
find_program(OBJCOPY arm-none-eabi-objcopy)
find_program(SIZE_TOOL arm-none-eabi-size)

add_custom_command(TARGET MaixPy_sim POST_BUILD
    COMMAND ${OBJCOPY} -O ihex   $<TARGET_FILE:MaixPy_sim> ${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.hex
    COMMAND ${OBJCOPY} -O binary $<TARGET_FILE:MaixPy_sim> ${CMAKE_BINARY_DIR}/${MAIX_SIM_FIRMWARE_BASENAME}.bin
    COMMAND ${SIZE_TOOL} $<TARGET_FILE:MaixPy_sim>
    COMMENT "生成 HEX/BIN，打印固件大小"
)
