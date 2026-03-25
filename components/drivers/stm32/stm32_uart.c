/**
 * @file stm32_uart.c
 * @brief STM32 UART完整实现（DMA+IDLE中断不定长接收）
 */

#include "stm32_hal.h"
#include "hal_uart.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407) || defined(CONFIG_PLATFORM_STM32F767) || defined(CONFIG_PLATFORM_STM32H743)

#define UART_INSTANCE_MAX 8
#define UART_RX_DMA_BUF   256

typedef struct {
    UART_HandleTypeDef huart;
    DMA_HandleTypeDef  hdma_rx;
    bool               initialized;
    uint8_t            rx_dma_buf[UART_RX_DMA_BUF];
    volatile uint16_t  rx_len;       /* IDLE中断后有效字节数 */
    volatile bool      rx_ready;
} uart_ctx_t;

static uart_ctx_t s_uart[UART_INSTANCE_MAX];

static USART_TypeDef* uart_instance(uint32_t id) {
    switch (id) {
        case 0: return USART1;
        case 1: return USART2;
        case 2: return USART3;
        case 3: return UART4;
        case 4: return UART5;
        case 5: return USART6;
#ifdef UART7
        case 6: return UART7;
#endif
#ifdef UART8
        case 7: return UART8;
#endif
        default: return NULL;
    }
}

static void uart_clk_enable(uint32_t id) {
    switch (id) {
        case 0: __HAL_RCC_USART1_CLK_ENABLE(); break;
        case 1: __HAL_RCC_USART2_CLK_ENABLE(); break;
        case 2: __HAL_RCC_USART3_CLK_ENABLE(); break;
        case 3: __HAL_RCC_UART4_CLK_ENABLE();  break;
        case 4: __HAL_RCC_UART5_CLK_ENABLE();  break;
        case 5: __HAL_RCC_USART6_CLK_ENABLE(); break;
#ifdef __HAL_RCC_UART7_CLK_ENABLE
        case 6: __HAL_RCC_UART7_CLK_ENABLE();  break;
#endif
#ifdef __HAL_RCC_UART8_CLK_ENABLE
        case 7: __HAL_RCC_UART8_CLK_ENABLE();  break;
#endif
        default: break;
    }
}

hal_ret_t stm32_uart_init(hal_uart_handle_t* handle, uint32_t uart_id,
                           const hal_uart_config_t* config) {
    if (!handle || !config || uart_id >= UART_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;

    USART_TypeDef* inst = uart_instance(uart_id);
    if (!inst) return MAIX_HAL_INVALID_PARAM;

    uart_clk_enable(uart_id);

    uart_ctx_t* ctx = &s_uart[uart_id];
    memset(&ctx->huart, 0, sizeof(ctx->huart));

    ctx->huart.Instance          = inst;
    ctx->huart.Init.BaudRate     = config->baudrate ? config->baudrate : 115200;
    ctx->huart.Init.WordLength   = (config->wordlength == HAL_UART_WORDLENGTH_9B)
                                   ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
    ctx->huart.Init.StopBits     = (config->stopbits == HAL_UART_STOPBITS_2)
                                   ? UART_STOPBITS_2 : UART_STOPBITS_1;
    switch (config->parity) {
        case HAL_UART_PARITY_EVEN: ctx->huart.Init.Parity = UART_PARITY_EVEN; break;
        case HAL_UART_PARITY_ODD:  ctx->huart.Init.Parity = UART_PARITY_ODD;  break;
        default:                   ctx->huart.Init.Parity = UART_PARITY_NONE; break;
    }
    ctx->huart.Init.Mode         = UART_MODE_TX_RX;
    ctx->huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    ctx->huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&ctx->huart) != HAL_OK) return MAIX_HAL_ERROR;

    /* 使能IDLE中断 + 启动DMA循环接收 */
    __HAL_UART_ENABLE_IT(&ctx->huart, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&ctx->huart, ctx->rx_dma_buf, UART_RX_DMA_BUF);

    ctx->initialized = true;
    ctx->rx_ready    = false;
    ctx->rx_len      = 0;
    *handle = (hal_uart_handle_t)(uintptr_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_uart_deinit(hal_uart_handle_t handle) {
    uart_ctx_t* ctx = (uart_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized) return MAIX_HAL_INVALID_PARAM;
    HAL_UART_DMAStop(&ctx->huart);
    HAL_UART_DeInit(&ctx->huart);
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data,
                               size_t size, uint32_t timeout) {
    uart_ctx_t* ctx = (uart_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !tx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_UART_Transmit(&ctx->huart, (uint8_t*)tx_data,
                                            (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

hal_ret_t stm32_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data,
                              size_t size, uint32_t timeout) {
    uart_ctx_t* ctx = (uart_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !rx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_UART_Receive(&ctx->huart, rx_data,
                                           (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

/**
 * @brief 读取DMA不定长接收缓冲区（IDLE中断后调用）
 * @return 实际读取字节数，0表示无新数据
 */
uint16_t stm32_uart_read_dma(hal_uart_handle_t handle, uint8_t* buf, uint16_t max_len) {
    uart_ctx_t* ctx = (uart_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !buf || !ctx->rx_ready) return 0;

    uint16_t len = (ctx->rx_len > max_len) ? max_len : ctx->rx_len;
    memcpy(buf, ctx->rx_dma_buf, len);
    ctx->rx_ready = false;
    /* 重启DMA接收 */
    HAL_UART_Receive_DMA(&ctx->huart, ctx->rx_dma_buf, UART_RX_DMA_BUF);
    return len;
}

/**
 * @brief UART IDLE中断处理（在对应USARTx_IRQHandler中调用）
 */
void stm32_uart_idle_irq_handler(uint32_t uart_id) {
    if (uart_id >= UART_INSTANCE_MAX) return;
    uart_ctx_t* ctx = &s_uart[uart_id];
    if (!ctx->initialized) return;

    if (__HAL_UART_GET_FLAG(&ctx->huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&ctx->huart);
        HAL_UART_DMAStop(&ctx->huart);
        ctx->rx_len   = UART_RX_DMA_BUF
                        - (uint16_t)__HAL_DMA_GET_COUNTER(ctx->huart.hdmarx);
        ctx->rx_ready = true;
    }
}

#endif /* STM32 platforms */
