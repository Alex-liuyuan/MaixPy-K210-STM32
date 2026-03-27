#include <stdint.h>

#include <bsp.h>
#include <fpioa.h>
#include <rtthread.h>
#include <sysctl.h>
#include <uart.h>

extern int rtthread_startup(void);
extern void __libc_init_array(void);

extern char _tp0[];

static void k210_init_runtime_context(void)
{
    init_lma();
    init_bss();

    asm volatile("mv tp, %0" : : "r"(_tp0));
}

void primary_cpu_entry(void)
{
    k210_init_runtime_context();
    __libc_init_array();

    fpioa_init();
    fpioa_set_function(4, FUNC_UART3_RX);
    fpioa_set_function(5, FUNC_UART3_TX);
    uart_debug_init(UART_DEVICE_3);

    rtthread_startup();

    while (1)
    {
    }
}

int pthread_setcancelstate(int state, int *oldstate)
{
    (void)state;
    (void)oldstate;
    return 0;
}
