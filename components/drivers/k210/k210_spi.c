/**
 * @file k210_spi.c
 * @brief K210 SPI驱动实现
 *
 * K210有4个SPI控制器：SPI0/SPI1（通用）、SPI2（从机）、SPI3（Flash专用）。
 * 本驱动使用SPI0和SPI1。
 */

#include "../hal/hal_spi.h"
#include <string.h>
#include <stdlib.h>

#if defined(CONFIG_PLATFORM_K210)

#include "spi.h"
#include "fpioa.h"
#include "sysctl.h"
#include "dmac.h"

#define K210_SPI_MAX 2  /* SPI0, SPI1 */

typedef struct {
    spi_device_num_t dev;
    spi_chip_select_t cs;
    uint32_t baudrate;
    bool initialized;
} k210_spi_ctx_t;

static k210_spi_ctx_t s_spi_ctx[K210_SPI_MAX];

hal_ret_t k210_spi_init(hal_spi_handle_t* handle, uint32_t spi_id,
                         const hal_spi_config_t* config) {
    if (!handle || !config || spi_id >= K210_SPI_MAX) return MAIX_HAL_INVALID_PARAM;

    k210_spi_ctx_t* ctx = &s_spi_ctx[spi_id];
    ctx->dev = (spi_device_num_t)spi_id;
    ctx->cs = SPI_CHIP_SELECT_0;
    ctx->baudrate = config->baudrate;

    spi_init(ctx->dev, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_set_clk_rate(ctx->dev, config->baudrate);

    ctx->initialized = true;
    *handle = (hal_spi_handle_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t k210_spi_deinit(hal_spi_handle_t handle) {
    if (!handle) return MAIX_HAL_INVALID_PARAM;
    k210_spi_ctx_t* ctx = (k210_spi_ctx_t*)handle;
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t k210_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data,
                             size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !tx_data) return MAIX_HAL_INVALID_PARAM;
    k210_spi_ctx_t* ctx = (k210_spi_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    spi_send_data_standard(ctx->dev, ctx->cs, NULL, 0, tx_data, size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data,
                            size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !rx_data) return MAIX_HAL_INVALID_PARAM;
    k210_spi_ctx_t* ctx = (k210_spi_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    spi_receive_data_standard(ctx->dev, ctx->cs, NULL, 0, rx_data, size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_spi_transmit_receive(hal_spi_handle_t handle,
                                     const uint8_t* tx_data, uint8_t* rx_data,
                                     size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !tx_data || !rx_data) return MAIX_HAL_INVALID_PARAM;
    k210_spi_ctx_t* ctx = (k210_spi_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    spi_send_data_standard(ctx->dev, ctx->cs, tx_data, size, NULL, 0);
    spi_receive_data_standard(ctx->dev, ctx->cs, NULL, 0, rx_data, size);
    return MAIX_HAL_OK;
}

#endif /* CONFIG_PLATFORM_K210 */
