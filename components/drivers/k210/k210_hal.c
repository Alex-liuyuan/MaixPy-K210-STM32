/**
 * @file k210_hal.c
 * @brief K210 平台 HAL 适配层 — 注册真实驱动到 HAL ops
 */

#include "../hal/hal_gpio.h"
#include "../hal/hal_spi.h"
#include "../hal/hal_i2c.h"
#include "../hal/hal_uart.h"
#include <stdio.h>

#if defined(CONFIG_PLATFORM_K210)

/* 外部驱动函数声明（k210_gpio.c） */
extern hal_ret_t k210_gpio_init(uint32_t pin, const hal_gpio_config_t* config);
extern hal_ret_t k210_gpio_deinit(uint32_t pin);
extern hal_ret_t k210_gpio_write(uint32_t pin, hal_gpio_state_t state);
extern hal_gpio_state_t k210_gpio_read(uint32_t pin);
extern hal_ret_t k210_gpio_toggle(uint32_t pin);
extern hal_ret_t k210_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode,
                                       hal_gpio_irq_callback_t callback, void* user_data);
extern hal_ret_t k210_gpio_disable_irq(uint32_t pin);

/* 外部驱动函数声明（k210_spi.c） */
extern hal_ret_t k210_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config);
extern hal_ret_t k210_spi_deinit(hal_spi_handle_t handle);
extern hal_ret_t k210_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
extern hal_ret_t k210_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
extern hal_ret_t k210_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data,
                                            uint8_t* rx_data, size_t size, uint32_t timeout);

/* 外部驱动函数声明（k210_i2c.c） */
extern hal_ret_t k210_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config);
extern hal_ret_t k210_i2c_deinit(hal_i2c_handle_t handle);
extern hal_ret_t k210_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr,
                                           const uint8_t* tx_data, size_t size, uint32_t timeout);
extern hal_ret_t k210_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                          uint8_t* rx_data, size_t size, uint32_t timeout);
extern hal_ret_t k210_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr,
                                     uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                     const uint8_t* data, size_t size, uint32_t timeout);
extern hal_ret_t k210_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr,
                                    uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                    uint8_t* data, size_t size, uint32_t timeout);

/* 外部驱动函数声明（k210_uart.c） */
extern hal_ret_t k210_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config);
extern hal_ret_t k210_uart_deinit(hal_uart_handle_t handle);
extern hal_ret_t k210_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
extern hal_ret_t k210_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);

/* 注册函数 */
hal_ret_t k210_register_hal_ops(void) {
    static const hal_gpio_ops_t gpio_ops = {
        .init        = k210_gpio_init,
        .deinit      = k210_gpio_deinit,
        .write       = k210_gpio_write,
        .read        = k210_gpio_read,
        .toggle      = k210_gpio_toggle,
        .enable_irq  = k210_gpio_enable_irq,
        .disable_irq = k210_gpio_disable_irq,
    };

    static const hal_spi_ops_t spi_ops = {
        .init             = k210_spi_init,
        .deinit           = k210_spi_deinit,
        .transmit         = k210_spi_transmit,
        .receive          = k210_spi_receive,
        .transmit_receive = k210_spi_transmit_receive,
    };

    static const hal_i2c_ops_t i2c_ops = {
        .init             = k210_i2c_init,
        .deinit           = k210_i2c_deinit,
        .master_transmit  = k210_i2c_master_transmit,
        .master_receive   = k210_i2c_master_receive,
        .mem_write        = k210_i2c_mem_write,
        .mem_read         = k210_i2c_mem_read,
    };

    static const hal_uart_ops_t uart_ops = {
        .init     = k210_uart_init,
        .deinit   = k210_uart_deinit,
        .transmit = k210_uart_transmit,
        .receive  = k210_uart_receive,
    };

    hal_gpio_register_ops(&gpio_ops);
    hal_spi_register_ops(&spi_ops);
    hal_i2c_register_ops(&i2c_ops);
    hal_uart_register_ops(&uart_ops);

    return MAIX_HAL_OK;
}

#else

/* 非 K210 平台编译占位 */
hal_ret_t k210_register_hal_ops(void) { (void)0; return MAIX_HAL_NOT_SUPPORTED; }

#endif
