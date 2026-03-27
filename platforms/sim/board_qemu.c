/*
 * QEMU 仿真专用 board 覆盖文件
 * 解决两个 QEMU 兼容性问题：
 *   1. SystemClock_Config 中 HAL_RCC_OscConfig 在 QEMU 返回错误 → 提供空实现
 *   2. HAL_UART_Init 等待 USART 就绪标志 → 用 semihosting 替换控制台输出
 */

#include <rthw.h>
#include <rtthread.h>
#include "main.h"

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE (64 * 1024)
static rt_uint8_t rt_heap[RT_HEAP_SIZE];

RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

/* ── QEMU 兼容的时钟配置（跳过 RCC，直接用复位后默认 HSI 16MHz）──────── */
void SystemClock_Config(void)
{
    /* QEMU olimex-stm32-h405 不完整实现 RCC，直接跳过。
     * 复位后默认使用 HSI 16MHz，足够 RT-Thread 运行。 */
    SystemCoreClockUpdate();
}

/* ── SysTick ────────────────────────────────────────────────────────────── */
void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

/* ── board 初始化（最小化，避免 QEMU 兼容性问题）──────────────────────── */
void rt_hw_board_init(void)
{
    /* 不调用 HAL_Init()，避免 NVIC 优先级分组修改干扰 RT-Thread 调度。
     * 只做 SysTick 配置，SystemCoreClock 在 startup 里已由 SystemInit 设置。 */
    SystemCoreClockUpdate();
    HAL_SYSTICK_Config(SystemCoreClock / RT_TICK_PER_SECOND);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}

/* ── ARM semihosting SYS_WRITE0 ─────────────────────────────────────────
 * QEMU 启动时加 -semihosting 参数后，此调用把字符串输出到宿主机终端。
 * ─────────────────────────────────────────────────────────────────────── */
static void semihost_write0(const char *str)
{
    __asm volatile (
        "mov r0, #0x04  \n"   /* SYS_WRITE0 */
        "mov r1, %0     \n"
        "bkpt 0xAB      \n"   /* semihosting trap */
        :
        : "r" (str)
        : "r0", "r1", "memory"
    );
}

#ifdef RT_USING_CONSOLE
void rt_hw_console_output(const char *str)
{
    semihost_write0(str);
}
#endif

#ifdef RT_USING_FINSH
char rt_hw_console_getchar(void)
{
    return -1;
}
#endif
