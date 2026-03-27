/**
 * @file stm32_syscalls.c
 * @brief newlib 最小系统调用，将 printf/read 重定向到 USART2
 */

#include "stm32_hal.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(CONFIG_PLATFORM_STM32F407)

static inline USART_TypeDef *maix_stdio_uart(void)
{
    return USART2;
}

int _write(int file, const char *ptr, int len)
{
    USART_TypeDef *uart = maix_stdio_uart();

    if ((file != 1 && file != 2) || ptr == NULL || len < 0)
    {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < len; ++i)
    {
        if (ptr[i] == '\n')
        {
            while ((uart->SR & USART_SR_TXE) == 0U) {}
            uart->DR = '\r';
        }
        while ((uart->SR & USART_SR_TXE) == 0U) {}
        uart->DR = (uint8_t)ptr[i];
    }

    while ((uart->SR & USART_SR_TC) == 0U) {}
    return len;
}

int _read(int file, char *ptr, int len)
{
    USART_TypeDef *uart = maix_stdio_uart();

    if (file != 0 || ptr == NULL || len <= 0)
    {
        errno = EINVAL;
        return -1;
    }

    while ((uart->SR & USART_SR_RXNE) == 0U) {}
    ptr[0] = (char)(uart->DR & 0xFFU);
    return 1;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    if (st == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    return (file == 0 || file == 1 || file == 2) ? 1 : 0;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

void _exit(int status)
{
    (void)status;
    __disable_irq();
    while (1) {}
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

#endif /* STM32 platforms */
