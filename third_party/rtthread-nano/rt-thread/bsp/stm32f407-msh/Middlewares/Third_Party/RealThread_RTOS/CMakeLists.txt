cmake_minimum_required(VERSION 3.20)

set(RTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT DEFINED RTTHREAD_BSP_DIR)
    set(RTTHREAD_BSP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
endif()

# RT-Thread Nano 内核源文件
set(RTTHREAD_SRCS
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
    # CPU 端口
    ${RTOS_DIR}/libcpu/arm/cortex-m4/cpuport.c
    ${RTOS_DIR}/libcpu/arm/cortex-m4/context_gcc.S
    # FinSH shell 组件
    ${RTOS_DIR}/components/finsh/shell.c
    ${RTOS_DIR}/components/finsh/msh.c
    ${RTOS_DIR}/components/finsh/msh_parse.c
    ${RTOS_DIR}/components/finsh/cmd.c
    # 设备框架
    ${RTOS_DIR}/components/device/device.c
    # board 初始化（SysTick_Handler + rt_hw_board_init）
    ${RTOS_DIR}/bsp/_template/cubemx_config/board.c
)

add_library(rtthread_nano STATIC ${RTTHREAD_SRCS})

target_include_directories(rtthread_nano PUBLIC
    ${RTOS_DIR}/include
    ${RTOS_DIR}/components/finsh
    ${RTOS_DIR}/finsh
    # rtconfig.h 所在目录
    ${RTTHREAD_BSP_DIR}/RT-Thread
    # STM32F4 HAL 头文件
    ${RTTHREAD_BSP_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${RTTHREAD_BSP_DIR}/Drivers/CMSIS/Include
    # CubeMX 生成的 Core 头文件
    ${RTTHREAD_BSP_DIR}/Core/Inc
)

target_compile_definitions(rtthread_nano PUBLIC
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_STM32F407
    STM32F407xx
    USE_HAL_DRIVER
    HSE_VALUE=8000000U
)
