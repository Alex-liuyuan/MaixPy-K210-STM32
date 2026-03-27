# QEMU 仿真专用构建配置
# 目标：STM32F407VGT6 on QEMU netduino2 machine
# 不依赖 bindings / tflite_micro，直接编译 RT-Thread Nano + STM32 HAL + main.c

cmake_minimum_required(VERSION 3.20)

set(ROOT     ${CMAKE_SOURCE_DIR})
set(BSP_DIR  ${ROOT}/third_party/rtthread-nano/rt-thread/bsp/stm32f407-msh)
set(RTOS_DIR ${BSP_DIR}/Middlewares/Third_Party/RealThread_RTOS)
set(HAL_DIR  ${BSP_DIR}/Drivers/STM32F4xx_HAL_Driver)
set(CMSIS_DIR ${BSP_DIR}/Drivers/CMSIS)

# ── RT-Thread Nano 内核 ──────────────────────────────────────────────────────
set(RTOS_SRCS
    ${RTOS_DIR}/src/clock.c
    ${RTOS_DIR}/src/components.c
    ${RTOS_DIR}/src/cpu.c
    ${RTOS_DIR}/src/idle.c
    ${RTOS_DIR}/src/ipc.c
    ${RTOS_DIR}/src/irq.c
    ${RTOS_DIR}/src/kservice.c
    ${RTOS_DIR}/src/mem.c
    ${RTOS_DIR}/src/memheap.c
    ${RTOS_DIR}/src/mempool.c
    ${RTOS_DIR}/src/object.c
    ${RTOS_DIR}/src/scheduler.c
    ${RTOS_DIR}/src/signal.c
    ${RTOS_DIR}/src/slab.c
    ${RTOS_DIR}/src/thread.c
    ${RTOS_DIR}/src/timer.c
    ${RTOS_DIR}/libcpu/arm/cortex-m4/cpuport.c
    ${RTOS_DIR}/libcpu/arm/cortex-m4/context_gcc.S
    ${RTOS_DIR}/components/finsh/shell.c
    ${RTOS_DIR}/components/finsh/msh.c
    ${RTOS_DIR}/components/finsh/msh_parse.c
    ${RTOS_DIR}/components/finsh/cmd.c
    ${RTOS_DIR}/components/device/device.c
    ${RTOS_DIR}/bsp/_template/cubemx_config/board.c
)

# ── STM32F4 HAL（只编译 main.c 实际用到的模块）────────────────────────────
file(GLOB HAL_SRCS
    ${HAL_DIR}/Src/stm32f4xx_hal.c
    ${HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${HAL_DIR}/Src/stm32f4xx_hal_dma.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_tim.c
    ${HAL_DIR}/Src/stm32f4xx_hal_tim_ex.c
)

# ── BSP Core（CubeMX 生成）──────────────────────────────────────────────────
set(CORE_SRCS
    ${BSP_DIR}/Core/Src/main.c
    ${BSP_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${BSP_DIR}/Core/Src/stm32f4xx_it.c
    ${BSP_DIR}/Core/Src/system_stm32f4xx.c
)

# ── 启动文件（GCC 版本）────────────────────────────────────────────────────
set(STARTUP_SRC
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
)

# ── 汇总所有源文件 ──────────────────────────────────────────────────────────
set(ALL_SRCS
    ${RTOS_SRCS}
    ${HAL_SRCS}
    ${CORE_SRCS}
    ${STARTUP_SRC}
)

# ── 头文件路径 ───────────────────────────────────────────────────────────────
set(ALL_INCLUDES
    ${RTOS_DIR}/include
    ${RTOS_DIR}/components/finsh
    ${RTOS_DIR}/finsh
    ${BSP_DIR}/RT-Thread          # rtconfig.h
    ${HAL_DIR}/Inc
    ${CMSIS_DIR}/Device/ST/STM32F4xx/Include
    ${CMSIS_DIR}/Core/Include
    ${CMSIS_DIR}/Include
    ${BSP_DIR}/Core/Inc
)

# ── 编译宏 ───────────────────────────────────────────────────────────────────
set(ALL_DEFS
    STM32F407xx
    USE_HAL_DRIVER
    HSE_VALUE=8000000U
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_STM32F407
)
