# RT-Thread Nano 平台参数
# 复用 stm32f407.cmake 的 CPU 标志（cortex-m4 + fpv4-sp-d16 + hard ABI）

set(RTOS_DIR "${CMAKE_SOURCE_DIR}/rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Middlewares/Third_Party/RealThread_RTOS")
set(RTTHREAD_BSP_DIR "${CMAKE_SOURCE_DIR}/rtthread-nano-master/rt-thread/bsp/stm32f407-msh")

add_compile_definitions(
    CONFIG_PLATFORM_RTTHREAD
    CONFIG_PLATFORM_STM32F407
)
