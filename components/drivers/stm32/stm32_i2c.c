/**
 * @file stm32_i2c.c
 * @brief STM32 I2C完整实现（7bit/10bit，内存读写）
 */

#include "stm32_hal.h"
#include "hal_i2c.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407) || defined(CONFIG_PLATFORM_STM32F767) || defined(CONFIG_PLATFORM_STM32H743)

#define I2C_INSTANCE_MAX 4

typedef struct {
    I2C_HandleTypeDef hi2c;
    bool              initialized;
} i2c_ctx_t;

static i2c_ctx_t s_i2c[I2C_INSTANCE_MAX];

static I2C_TypeDef* i2c_instance(uint32_t id) {
    switch (id) {
        case 0: return I2C1;
        case 1: return I2C2;
        case 2: return I2C3;
#ifdef I2C4
        case 3: return I2C4;
#endif
        default: return NULL;
    }
}

static void i2c_clk_enable(uint32_t id) {
    switch (id) {
        case 0: __HAL_RCC_I2C1_CLK_ENABLE(); break;
        case 1: __HAL_RCC_I2C2_CLK_ENABLE(); break;
        case 2: __HAL_RCC_I2C3_CLK_ENABLE(); break;
#ifdef __HAL_RCC_I2C4_CLK_ENABLE
        case 3: __HAL_RCC_I2C4_CLK_ENABLE(); break;
#endif
        default: break;
    }
}

hal_ret_t stm32_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id,
                          const hal_i2c_config_t* config) {
    if (!handle || !config || i2c_id >= I2C_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;

    I2C_TypeDef* inst = i2c_instance(i2c_id);
    if (!inst) return MAIX_HAL_INVALID_PARAM;

    i2c_clk_enable(i2c_id);

    i2c_ctx_t* ctx = &s_i2c[i2c_id];
    memset(&ctx->hi2c, 0, sizeof(ctx->hi2c));

    ctx->hi2c.Instance             = inst;
    ctx->hi2c.Init.ClockSpeed      = config->clock_speed ? config->clock_speed : 100000;
    ctx->hi2c.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    ctx->hi2c.Init.OwnAddress1     = (config->mode == MAIX_HAL_I2C_MODE_SLAVE)
                                     ? (config->slave_address << 1) : 0;
    ctx->hi2c.Init.AddressingMode  = config->address_10bit
                                     ? I2C_ADDRESSINGMODE_10BIT
                                     : I2C_ADDRESSINGMODE_7BIT;
    ctx->hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    ctx->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    ctx->hi2c.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&ctx->hi2c) != HAL_OK) return MAIX_HAL_ERROR;

    ctx->initialized = true;
    *handle = (hal_i2c_handle_t)(uintptr_t)ctx;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_i2c_deinit(hal_i2c_handle_t handle) {
    i2c_ctx_t* ctx = (i2c_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized) return MAIX_HAL_INVALID_PARAM;
    HAL_I2C_DeInit(&ctx->hi2c);
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr,
                                    const uint8_t* tx_data, size_t size,
                                    uint32_t timeout) {
    i2c_ctx_t* ctx = (i2c_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !tx_data) return MAIX_HAL_INVALID_PARAM;
    /* STM32 HAL期望地址左移1位 */
    HAL_StatusTypeDef r = HAL_I2C_Master_Transmit(&ctx->hi2c, device_addr << 1,
                                                  (uint8_t*)tx_data,
                                                  (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

hal_ret_t stm32_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                   uint8_t* rx_data, size_t size, uint32_t timeout) {
    i2c_ctx_t* ctx = (i2c_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !rx_data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_I2C_Master_Receive(&ctx->hi2c, device_addr << 1,
                                                 rx_data, (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

/* 寄存器/内存读写（常用于传感器） */
hal_ret_t stm32_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr,
                               uint16_t mem_addr, uint16_t mem_addr_size,
                               const uint8_t* data, size_t size, uint32_t timeout) {
    i2c_ctx_t* ctx = (i2c_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_I2C_Mem_Write(&ctx->hi2c, device_addr << 1,
                                            mem_addr, mem_addr_size,
                                            (uint8_t*)data, (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

hal_ret_t stm32_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr,
                              uint16_t mem_addr, uint16_t mem_addr_size,
                              uint8_t* data, size_t size, uint32_t timeout) {
    i2c_ctx_t* ctx = (i2c_ctx_t*)(uintptr_t)handle;
    if (!ctx || !ctx->initialized || !data) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_I2C_Mem_Read(&ctx->hi2c, device_addr << 1,
                                           mem_addr, mem_addr_size,
                                           data, (uint16_t)size, timeout);
    return (r == HAL_OK) ? MAIX_HAL_OK : (r == HAL_TIMEOUT) ? MAIX_HAL_TIMEOUT : MAIX_HAL_ERROR;
}

#endif /* STM32 platforms */
