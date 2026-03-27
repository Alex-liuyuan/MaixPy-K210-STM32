/**
 * @file k210_i2c.c
 * @brief K210 I2C驱动实现
 *
 * K210有3个I2C控制器（I2C0/I2C1/I2C2），均支持主机模式。
 */

#include "../hal/hal_i2c.h"
#include <string.h>
#include <stdlib.h>

#if defined(CONFIG_PLATFORM_K210)

#include "i2c.h"
#include "fpioa.h"
#include "sysctl.h"

#define K210_I2C_MAX 3

typedef struct {
    i2c_device_number_t dev;
    uint32_t clock_speed;
    bool initialized;
} k210_i2c_ctx_t;

static k210_i2c_ctx_t s_i2c_ctx[K210_I2C_MAX];

hal_ret_t k210_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id,
                         const hal_i2c_config_t* config) {
    if (!handle || !config || i2c_id >= K210_I2C_MAX) return MAIX_HAL_INVALID_PARAM;

    k210_i2c_ctx_t* ctx = &s_i2c_ctx[i2c_id];
    ctx->dev = (i2c_device_number_t)i2c_id;
    ctx->clock_speed = config->clock_speed;

    i2c_init(ctx->dev, config->slave_address, 7, config->clock_speed);

    ctx->initialized = true;
    *handle = (hal_i2c_handle_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t k210_i2c_deinit(hal_i2c_handle_t handle) {
    if (!handle) return MAIX_HAL_INVALID_PARAM;
    k210_i2c_ctx_t* ctx = (k210_i2c_ctx_t*)handle;
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t k210_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr,
                                    const uint8_t* tx_data, size_t size,
                                    uint32_t timeout) {
    (void)timeout;
    if (!handle || !tx_data) return MAIX_HAL_INVALID_PARAM;
    k210_i2c_ctx_t* ctx = (k210_i2c_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    i2c_send_data(ctx->dev, device_addr, tx_data, size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                   uint8_t* rx_data, size_t size,
                                   uint32_t timeout) {
    (void)timeout;
    if (!handle || !rx_data) return MAIX_HAL_INVALID_PARAM;
    k210_i2c_ctx_t* ctx = (k210_i2c_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    i2c_recv_data(ctx->dev, device_addr, NULL, 0, rx_data, size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr,
                              uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                              const uint8_t* data, size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !data) return MAIX_HAL_INVALID_PARAM;
    k210_i2c_ctx_t* ctx = (k210_i2c_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    /* 构造发送缓冲：mem_addr + data */
    uint8_t addr_bytes = (mem_addr_size == HAL_I2C_MEMADDR_16BIT) ? 2 : 1;
    uint8_t buf[2 + 256]; /* 最大256字节数据 */
    if (size > 256) return MAIX_HAL_INVALID_PARAM;

    if (addr_bytes == 2) {
        buf[0] = (mem_addr >> 8) & 0xFF;
        buf[1] = mem_addr & 0xFF;
    } else {
        buf[0] = mem_addr & 0xFF;
    }
    memcpy(buf + addr_bytes, data, size);

    i2c_send_data(ctx->dev, device_addr, buf, addr_bytes + size);
    return MAIX_HAL_OK;
}

hal_ret_t k210_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr,
                             uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                             uint8_t* data, size_t size, uint32_t timeout) {
    (void)timeout;
    if (!handle || !data) return MAIX_HAL_INVALID_PARAM;
    k210_i2c_ctx_t* ctx = (k210_i2c_ctx_t*)handle;
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    uint8_t addr_buf[2];
    uint8_t addr_len;
    if (mem_addr_size == HAL_I2C_MEMADDR_16BIT) {
        addr_buf[0] = (mem_addr >> 8) & 0xFF;
        addr_buf[1] = mem_addr & 0xFF;
        addr_len = 2;
    } else {
        addr_buf[0] = mem_addr & 0xFF;
        addr_len = 1;
    }

    i2c_recv_data(ctx->dev, device_addr, addr_buf, addr_len, data, size);
    return MAIX_HAL_OK;
}

#endif /* CONFIG_PLATFORM_K210 */
