# STM32F407 平台参数
set(STM32_FAMILY    "F4")
set(STM32_DEVICE    "STM32F407xx")
set(CPU_FLAGS       "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

add_compile_definitions(
    CONFIG_PLATFORM_STM32F407
    STM32F407xx
    USE_HAL_DRIVER
    HSE_VALUE=8000000U
    ARM_MATH_CM4
)

set(CMAKE_C_FLAGS_INIT   "${CMAKE_C_FLAGS_INIT} ${CPU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} ${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CMAKE_ASM_FLAGS_INIT} ${CPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} ${CPU_FLAGS}")

set(STARTUP_FILE "${CMAKE_SOURCE_DIR}/platforms/stm32/startup/startup_stm32f407xx.s")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/platforms/stm32/linker/stm32f407_flash.ld")

# Flash/RAM 大小（用于链接脚本生成参考）
set(FLASH_SIZE 0x100000)   # 1MB
set(SRAM_SIZE  0x20000)    # 128KB
set(CCMRAM_SIZE 0x10000)   # 64KB CCM
