#include "stm32_hal.h"
#include "hal_gpio.h"
#include "hal_spi.h" 
#include "hal_i2c.h"
#include "hal_uart.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief STM32平台硬件抽象层实现
 * @file stm32_hal.c
 * @author MaixPy Nano RT-Thread Team
 */

#if defined(CONFIG_PLATFORM_STM32F407)

// STM32设备句柄结构体
typedef struct {
    uint32_t device_id;
    void* stm32_handle;
    bool initialized;
} stm32_device_handle_t;

// GPIO端口映射表
static GPIO_TypeDef* gpio_port_map[] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH
#ifdef GPIOI
    , GPIOI
#endif
};

static hal_ret_t stm32_gpio_enable_clock(uint32_t port_num) {
    switch (port_num) {
        case STM32_GPIO_PORT_A: __HAL_RCC_GPIOA_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_B: __HAL_RCC_GPIOB_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_C: __HAL_RCC_GPIOC_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_D: __HAL_RCC_GPIOD_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_E: __HAL_RCC_GPIOE_CLK_ENABLE(); return MAIX_HAL_OK;
#ifdef __HAL_RCC_GPIOF_CLK_ENABLE
        case STM32_GPIO_PORT_F: __HAL_RCC_GPIOF_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOG_CLK_ENABLE
        case STM32_GPIO_PORT_G: __HAL_RCC_GPIOG_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOH_CLK_ENABLE
        case STM32_GPIO_PORT_H: __HAL_RCC_GPIOH_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOI_CLK_ENABLE
        case STM32_GPIO_PORT_I: __HAL_RCC_GPIOI_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
        default: return MAIX_HAL_INVALID_PARAM;
    }
}

static uint32_t stm32_gpio_mode_convert(hal_gpio_mode_t mode) {
    switch (mode) {
        case HAL_GPIO_MODE_INPUT:  return GPIO_MODE_INPUT;
        case HAL_GPIO_MODE_OUTPUT: return GPIO_MODE_OUTPUT_PP;
        case HAL_GPIO_MODE_AF:     return GPIO_MODE_AF_PP;
        case HAL_GPIO_MODE_ANALOG: return GPIO_MODE_ANALOG;
        default:                   return UINT32_MAX;
    }
}

static uint32_t stm32_gpio_pull_convert(hal_gpio_pull_t pull) {
    switch (pull) {
        case HAL_GPIO_PULL_NONE: return GPIO_NOPULL;
        case HAL_GPIO_PULL_UP:   return GPIO_PULLUP;
        case HAL_GPIO_PULL_DOWN: return GPIO_PULLDOWN;
        default:                 return UINT32_MAX;
    }
}

static uint32_t stm32_gpio_speed_convert(hal_gpio_speed_t speed) {
    switch (speed) {
        case HAL_GPIO_SPEED_LOW:       return GPIO_SPEED_FREQ_LOW;
        case HAL_GPIO_SPEED_MEDIUM:    return GPIO_SPEED_FREQ_MEDIUM;
        case HAL_GPIO_SPEED_HIGH:      return GPIO_SPEED_FREQ_HIGH;
        case HAL_GPIO_SPEED_VERY_HIGH: return GPIO_SPEED_FREQ_VERY_HIGH;
        default:                       return UINT32_MAX;
    }
}

static hal_ret_t stm32_i2c_mem_write_adapter(hal_i2c_handle_t handle, uint16_t device_addr,
                                             uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                             const uint8_t* data, size_t size, uint32_t timeout) {
    return stm32_i2c_mem_write(handle, device_addr, mem_addr, (uint16_t)mem_addr_size,
                               data, size, timeout);
}

static hal_ret_t stm32_i2c_mem_read_adapter(hal_i2c_handle_t handle, uint16_t device_addr,
                                            uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                            uint8_t* data, size_t size, uint32_t timeout) {
    return stm32_i2c_mem_read(handle, device_addr, mem_addr, (uint16_t)mem_addr_size,
                              data, size, timeout);
}

// STM32 GPIO实现
hal_ret_t stm32_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (!config) return MAIX_HAL_INVALID_PARAM;
    
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;
    
    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    if (port_num >= (sizeof(gpio_port_map) / sizeof(gpio_port_map[0]))) {
        return MAIX_HAL_INVALID_PARAM;
    }

    if (stm32_gpio_enable_clock(port_num) != MAIX_HAL_OK) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    // 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);

    GPIO_InitStruct.Mode = stm32_gpio_mode_convert(config->mode);
    GPIO_InitStruct.Pull = stm32_gpio_pull_convert(config->pull);
    GPIO_InitStruct.Speed = stm32_gpio_speed_convert(config->speed);
    GPIO_InitStruct.Alternate = config->alternate;

    if (GPIO_InitStruct.Mode == UINT32_MAX ||
        GPIO_InitStruct.Pull == UINT32_MAX ||
        GPIO_InitStruct.Speed == UINT32_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    HAL_GPIO_Init(gpio_port_map[port_num], &GPIO_InitStruct);
    return MAIX_HAL_OK;
}

hal_ret_t stm32_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    GPIO_PinState pin_state = (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(gpio_port_map[port_num], (1 << pin_num), pin_state);
    return MAIX_HAL_OK;
}

hal_gpio_state_t stm32_gpio_read(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return HAL_GPIO_PIN_RESET;
    }
    GPIO_PinState s = HAL_GPIO_ReadPin(gpio_port_map[port_num], (1 << pin_num));
    return (s == GPIO_PIN_SET) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

hal_ret_t stm32_gpio_toggle(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    HAL_GPIO_TogglePin(gpio_port_map[port_num], (1 << pin_num));
    return MAIX_HAL_OK;
}

hal_ret_t stm32_gpio_deinit(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    HAL_GPIO_DeInit(gpio_port_map[port_num], (1 << pin_num));
    return MAIX_HAL_OK;
}

// STM32平台初始化
hal_ret_t stm32_hal_init(void) {
    HAL_Init();
    return stm32_register_hal_ops();
}

// STM32平台操作注册
hal_ret_t stm32_register_hal_ops(void) {
    static const hal_gpio_ops_t gpio_ops = {
        .init   = stm32_gpio_init,
        .deinit = stm32_gpio_deinit,
        .write  = stm32_gpio_write,
        .read   = stm32_gpio_read,
        .toggle = stm32_gpio_toggle,
    };
    static const hal_spi_ops_t spi_ops = {
        .init = stm32_spi_init,
        .deinit = stm32_spi_deinit,
        .transmit = stm32_spi_transmit,
        .receive = stm32_spi_receive,
        .transmit_receive = stm32_spi_transmit_receive,
    };
    static const hal_i2c_ops_t i2c_ops = {
        .init = stm32_i2c_init,
        .deinit = stm32_i2c_deinit,
        .master_transmit = stm32_i2c_master_transmit,
        .master_receive = stm32_i2c_master_receive,
        .mem_write = stm32_i2c_mem_write_adapter,
        .mem_read = stm32_i2c_mem_read_adapter,
    };
    static const hal_uart_ops_t uart_ops = {
        .init = stm32_uart_init,
        .deinit = stm32_uart_deinit,
        .transmit = stm32_uart_transmit,
        .receive = stm32_uart_receive,
    };

    hal_gpio_register_ops(&gpio_ops);
    hal_spi_register_ops(&spi_ops);
    hal_i2c_register_ops(&i2c_ops);
    hal_uart_register_ops(&uart_ops);

    return MAIX_HAL_OK;
}

#endif /* STM32 platforms */
