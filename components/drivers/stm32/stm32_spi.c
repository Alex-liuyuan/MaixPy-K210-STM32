/**
 * @file stm32_spi.c
 * @brief STM32 SPI完整实现（同步 + DMA异步）
 */

#include "stm32_hal.h"
#include "hal_spi.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407)

#define SPI_INSTANCE_MAX 6

typedef struct {
    SPI_HandleTypeDef hspi;
    bool              initialized;
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
} spi_ctx_t;

static spi_ctx_t s_spi[SPI_INSTANCE_MAX];

/* SPI外设实例映射 */
static SPI_TypeDef* spi_instance(uint32_t id) {
    switch (id) {
        case 0: return SPI1;
        case 1: return SPI2;
        case 2: return SPI3;
#ifdef SPI4
        case 3: return SPI4;
#endif
#ifdef SPI5
        case 4: return SPI5;
#endif
#ifdef SPI6
        case 5: return SPI6;
#endif
        default: return NULL;
    }
}

/* 使能SPI时钟 */
static void spi_clk_enable(uint32_t id) {
    switch (id) {
        case 0: __HAL_RCC_SPI1_CLK_ENABLE(); break;
        case 1: __HAL_RCC_SPI2_CLK_ENABLE(); break;
        case 2: __HAL_RCC_SPI3_CLK_ENABLE(); break;
#if defined(SPI4) && defined(__HAL_RCC_SPI4_CLK_ENABLE)
        case 3: __HAL_RCC_SPI4_CLK_ENABLE(); break;
#endif
#if defined(SPI5) && defined(__HAL_RCC_SPI5_CLK_ENABLE)
        case 4: __HAL_RCC_SPI5_CLK_ENABLE(); break;
#endif
#if defined(SPI6) && defined(__HAL_RCC_SPI6_CLK_ENABLE)
        case 5: __HAL_RCC_SPI6_CLK_ENABLE(); break;
#endif
        default: break;
    }
}

hal_ret_t stm32_spi_init(hal_spi_handle_t* handle, uint32_t spi_id,
                         const hal_spi_config_t* config) {
    if (!handle || !config || spi_id >= SPI_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;

    SPI_TypeDef* inst = spi_instance(spi_id);
    if (!inst) return MAIX_HAL_INVALID_PARAM;

    spi_clk_enable(spi_id);

    spi_ctx_t* ctx = &s_spi[spi_id];
    memset(&ctx->hspi, 0, sizeof(ctx->hspi));

    ctx->hspi.Instance               = inst;
    ctx->hspi.Init.Mode              = (config->mode == HAL_SPI_MODE_MASTER)
                                       ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    ctx->hspi.Init.Direction         = SPI_DIRECTION_2LINES;
    ctx->hspi.Init.DataSize          = (config->datasize == HAL_SPI_DATASIZE_16BIT)
                                       ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    ctx->hspi.Init.CLKPolarity       = (config->cpol == HAL_SPI_CPOL_HIGH)
                                       ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    ctx->hspi.Init.CLKPhase          = (config->cpha == HAL_SPI_CPHA_2EDGE)
                                       ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    ctx->hspi.Init.NSS               = SPI_NSS_SOFT;
    ctx->hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; /* 默认，用户可调 */
    ctx->hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    ctx->hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
    ctx->hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(&ctx->hspi) != HAL_OK) return MAIX_HAL_ERROR;

    ctx->initialized = true;
    *handle = (hal_spi_handle_t)(uintptr_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_spi_deinit(hal_spi_handle_t handle) {
    spi_ctx_t* ctx = (spi_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized) return MAIX_HAL_INVALID_PARAM;
    HAL_SPI_DeInit(&ctx->hspi);
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data,
                              size_t size, uint32_t timeout) {
    spi_ctx_t* ctx = (spi_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !tx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_SPI_Transmit(&ctx->hspi, (uint8_t*)tx_data,
                                           (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

hal_ret_t stm32_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data,
                             size_t size, uint32_t timeout) {
    spi_ctx_t* ctx = (spi_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !rx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_SPI_Receive(&ctx->hspi, rx_data, (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

hal_ret_t stm32_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data,
                                     uint8_t* rx_data, size_t size, uint32_t timeout) {
    spi_ctx_t* ctx = (spi_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !tx_data || !rx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_SPI_TransmitReceive(&ctx->hspi, (uint8_t*)tx_data,
                                                  rx_data, (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

/* DMA异步发送（非阻塞） */
hal_ret_t stm32_spi_transmit_dma(hal_spi_handle_t handle, const uint8_t* tx_data,
                                  size_t size) {
    spi_ctx_t* ctx = (spi_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !tx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_SPI_Transmit_DMA(&ctx->hspi, (uint8_t*)tx_data,
                                               (uint16_t)size);
    return (r == HAL_OK) ? MAIX_HAL_OK : MAIX_HAL_ERROR;
}

#endif /* STM32 platforms */
