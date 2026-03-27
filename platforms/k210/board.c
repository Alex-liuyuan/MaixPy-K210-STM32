#include <rthw.h>
#include <rtthread.h>

#include <bsp.h>
#include <clint.h>
#include <fpioa.h>
#include <syscalls.h>
#include <sysctl.h>
#include <uart.h>

#include "maix_runtime_app.h"

extern void rt_hw_interrupt_init(void);
extern int rt_hw_tick_init(void);

extern char _heap_start[];
extern char _heap_end[];

void maix_board_app_init(void)
{
}

void maix_board_fill_profile(maix_runtime_profile_t *profile)
{
    if (profile == RT_NULL)
    {
        return;
    }

    profile->board_name = "k210_generic";
    profile->runtime_name = "rtthread/k210";
    profile->console_name = "UART3 @ IO5/IO4 115200 8N1";
    profile->cpu_arch = "riscv64-k210";
    profile->led = MAIX_CAP_UNAVAILABLE;
    profile->camera = MAIX_CAP_PLANNED;
    profile->display = MAIX_CAP_PLANNED;
}

void maix_board_heartbeat(unsigned long heartbeat_count)
{
    (void)heartbeat_count;
}

void rt_hw_board_init(void)
{
    rt_hw_interrupt_init();
    rt_hw_tick_init();
    rt_system_heap_init(_heap_start, _heap_end);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

void rt_hw_console_output(const char *str)
{
    if (str == RT_NULL || sys_putchar == RT_NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        if (*str == '\n')
        {
            sys_putchar('\r');
        }
        sys_putchar(*str++);
    }
}

char rt_hw_console_getchar(void)
{
    if (sys_getchar == RT_NULL)
    {
        return (char)-1;
    }

    return (char)sys_getchar();
}
