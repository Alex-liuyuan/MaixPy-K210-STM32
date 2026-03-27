/**
 * @file k210_uart.c
 * @brief K210 UART驱动实现
 *
 * K210有3个UART控制器（UART1/UART2/UART3），UART0被调试口占用。
 * 本驱动映射：uart_id 0→UART1, 1→UART2, 2→UART3。
 */

#include "../hal/hal_uart.h"
#include <string.h>
#include <stdlib.h>

#if defined(CONFIG_PLATFORM_K210)

#include "uart.h"
#include "fpioa.h"
#include "sysctl.h"

#define K210_UART_MAX 3

typedef struct {
    uart_device_number_t dev;
    uint32_t baudrate;
    bool initialized;
} k210_uart_ctx_t;

static k210_uart_ctx_t s_uart_ctx[K210_UART_MAX];

hal_ret_t k210_uart_init(hal_uart_handle_t* handle, uint32_t uart_id,
                          const hal_uart_config_t* config) {
    if (!handle || !config || uart_id >= K210_UART_MAX) return MAIX_HAL_INVALID_PARAM;

    k210_uart_ctx_t* ctx = &s_uart_ctx[uart_id];
    /* K210 UART编号：UART_DEVICE_1=0, UART_DEVICE_2=1, UART_DEVICE_3=2 */
    ctx->dev = (uart_device_number_t)(uart_id + 1);
    ctx->baudrate = config->baudrate;

    /* 数据位/停止位/校验位转换 */
    uart_stopbit_t stopbits = (config->stopbits == HAL_UART_STOPBITS_2)
                              ? UART_STOP_2 : UART_STOP_1;
    uart_parity_t parity;
    switch (config->parity) {
        case HAL_UART_PARITY_EVEN: parity = UART_PARITY_EVEN; break;
        case HAL_UART_PARITY_ODD:  parity = UART_PARITY_ODD;  break;
        default:                   parity = UART_PARITY_NONE;  break;
    }

    uart_init(ctx->dev);
    uart_configure(ctx->dev, config->baudrate, UART_BITWIDTH_8BIT,
                   stopbits, parity);

    ctx->initialized = true;
    *handle = (hal_uart_handle_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t k210_uart_deinit(hal_uart_handle_t handle) {
    if (!handle) return MAIX_HAL_INVALID_PARAM;
    k210_uart_ctx_t* ctx = (k210_uart_ctx_t*)handle;
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t k210_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data,
                              size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !tx_data) return MAIX_HAL_INVALID_PARAM;
    k210_uart_ctx_t* ctx = (k210_uart_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    uart_send_data(ctx->dev, (const char*)tx_data, size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data,
                             size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !rx_data) return MAIX_HAL_INVALID_PARAM;
    k210_uart_ctx_t* ctx = (k210_uart_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    /* K210 UART接收是阻塞的，逐字节读取 */
    for (size_t i = 0; i < size; i++) {
        rx_data[i] = (uint8_t)uart_getc(ctx->dev);
    }
    return MAIX_HAL_OK;
}

#endif /* CONFIG_PLATFORM_K210 */
