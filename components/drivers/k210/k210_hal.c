#include "../hal/hal_gpio.h"
#include "../hal/hal_spi.h"
#include "../hal/hal_i2c.h"
#include "../hal/hal_uart.h"
#include <stdio.h>

/**
 * K210 平台 HAL 适配层（初始 stub）
 * 目的：提供注册入口，将来替换为真实 K210 驱动实现
 */

#if defined(CONFIG_PLATFORM_K210)

// 简单的 stub 实现，返回不支持或占位符
static hal_ret_t k210_gpio_init(uint32_t pin, const hal_gpio_config_t* config) { (void)pin; (void)config; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_gpio_deinit(uint32_t pin) { (void)pin; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_gpio_write(uint32_t pin, hal_gpio_state_t state) { (void)pin; (void)state; return MAIX_HAL_NOT_SUPPORTED; }
static hal_gpio_state_t k210_gpio_read(uint32_t pin) { (void)pin; return HAL_GPIO_PIN_RESET; }
static hal_ret_t k210_gpio_toggle(uint32_t pin) { (void)pin; return MAIX_HAL_NOT_SUPPORTED; }

static hal_ret_t k210_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode, hal_gpio_irq_callback_t cb, void* ud) { (void)pin; (void)mode; (void)cb; (void)ud; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_gpio_disable_irq(uint32_t pin) { (void)pin; return MAIX_HAL_NOT_SUPPORTED; }

// SPI / I2C / UART 同样提供 stub
static hal_ret_t k210_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config) { (void)handle; (void)spi_id; (void)config; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_spi_deinit(hal_spi_handle_t handle) { (void)handle; return MAIX_HAL_NOT_SUPPORTED; }

static hal_ret_t k210_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config) { (void)handle; (void)i2c_id; (void)config; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_i2c_deinit(hal_i2c_handle_t handle) { (void)handle; return MAIX_HAL_NOT_SUPPORTED; }

static hal_ret_t k210_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config) { (void)handle; (void)uart_id; (void)config; return MAIX_HAL_NOT_SUPPORTED; }
static hal_ret_t k210_uart_deinit(hal_uart_handle_t handle) { (void)handle; return MAIX_HAL_NOT_SUPPORTED; }

// 注册函数
hal_ret_t k210_register_hal_ops(void) {
    static const hal_gpio_ops_t gpio_ops = {
        .init = k210_gpio_init,
        .deinit = k210_gpio_deinit,
        .write = k210_gpio_write,
        .read = k210_gpio_read,
        .toggle = k210_gpio_toggle,
        .enable_irq = k210_gpio_enable_irq,
        .disable_irq = k210_gpio_disable_irq,
    };

    static const hal_spi_ops_t spi_ops = {
        .init = k210_spi_init,
        .deinit = k210_spi_deinit,
    };

    static const hal_i2c_ops_t i2c_ops = {
        .init = k210_i2c_init,
        .deinit = k210_i2c_deinit,
    };

    static const hal_uart_ops_t uart_ops = {
        .init = k210_uart_init,
        .deinit = k210_uart_deinit,
    };

    hal_gpio_register_ops(&gpio_ops);
    hal_spi_register_ops(&spi_ops);
    hal_i2c_register_ops(&i2c_ops);
    hal_uart_register_ops(&uart_ops);

    return MAIX_HAL_OK;
}

#else

// 非 K210 平台编译占位
hal_ret_t k210_register_hal_ops(void) { (void)0; return MAIX_HAL_NOT_SUPPORTED; }

#endif
