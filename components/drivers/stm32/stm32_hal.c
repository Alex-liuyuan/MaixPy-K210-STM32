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
 * @author MaixPy-K210-STM32 Team
 */

#if defined(CONFIG_PLATFORM_STM32F407) || defined(CONFIG_PLATFORM_STM32F767) || defined(CONFIG_PLATFORM_STM32H743)

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

// STM32 GPIO实现
hal_ret_t stm32_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (!config) return HAL_INVALID_PARAM;
    
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;
    
    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return HAL_INVALID_PARAM;
    }
    
    // 使能GPIO时钟
    switch (port_num) {
        case STM32_GPIO_PORT_A: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case STM32_GPIO_PORT_B: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case STM32_GPIO_PORT_C: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case STM32_GPIO_PORT_D: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        default: return HAL_INVALID_PARAM;
    }
    
    // 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);
    
    switch (config->mode) {
        case HAL_GPIO_MODE_INPUT: GPIO_InitStruct.Mode = GPIO_MODE_INPUT; break;
        case HAL_GPIO_MODE_OUTPUT: GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; break;
        case HAL_GPIO_MODE_AF: GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; break;
        default: return HAL_INVALID_PARAM;
    }
    
    HAL_GPIO_Init(gpio_port_map[port_num], &GPIO_InitStruct);
    return HAL_OK;
}

hal_ret_t stm32_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;
    
    GPIO_PinState pin_state = (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(gpio_port_map[port_num], (1 << pin_num), pin_state);
    return HAL_OK;
}

// STM32平台初始化
hal_ret_t stm32_hal_init(void) {
    HAL_Init();
    return stm32_register_hal_ops();
}

// STM32平台操作注册
hal_ret_t stm32_register_hal_ops(void) {
    static const hal_gpio_ops_t gpio_ops = {
        .init = stm32_gpio_init,
        .write = stm32_gpio_write,
        // 其他操作函数...
    };
    hal_gpio_register_ops(&gpio_ops);
    
    return HAL_OK;
}

#endif /* STM32 platforms */