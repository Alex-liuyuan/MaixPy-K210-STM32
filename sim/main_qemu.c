/*
 * QEMU 仿真专用 main.c
 * 去掉 MX_USART1_UART_Init（QEMU USART 不完整），
 * 保留 RT-Thread 主循环，通过 semihosting 输出 hello SYSU_AIOTOS!
 */

#include "main.h"
#include <rtthread.h>
#include "maix_runtime_app.h"

void maix_board_app_init(void)
{
    /*
     * QEMU 路径不初始化虚拟 LED，也不伪造 GPIO 可见结果。
     * 仅复用与真板相同的 RT-Thread 应用逻辑和日志输出。
     */
}

void maix_board_fill_profile(maix_runtime_profile_t *profile)
{
    if (profile == RT_NULL)
    {
        return;
    }

    profile->board_name = "olimex-stm32-h405";
    profile->runtime_name = "qemu/olimex-stm32-h405";
    profile->console_name = "semihosting";
    profile->cpu_arch = "arm-cortex-m4";
    profile->led = MAIX_CAP_UNAVAILABLE;
    profile->storage = MAIX_CAP_UNAVAILABLE;
    profile->camera = MAIX_CAP_UNAVAILABLE;
    profile->display = MAIX_CAP_UNAVAILABLE;
    profile->python_vm = MAIX_CAP_PLANNED;
    profile->model_runtime = MAIX_CAP_PLANNED;
}

void maix_board_heartbeat(unsigned long heartbeat_count)
{
    (void)heartbeat_count;
}

int main(void)
{
    return maix_runtime_main();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
