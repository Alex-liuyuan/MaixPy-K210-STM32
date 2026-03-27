#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief GPIO硬件抽象层接口
 * @file hal_gpio.h
 * @author MaixPy Nano RT-Thread Team
 */

// GPIO引脚状态
typedef enum {
    HAL_GPIO_PIN_RESET = 0,
    HAL_GPIO_PIN_SET = 1
} hal_gpio_state_t;

// GPIO中断触发方式
typedef enum {
    HAL_GPIO_IT_RISING = 0,
    HAL_GPIO_IT_FALLING,
    HAL_GPIO_IT_RISING_FALLING
} hal_gpio_it_mode_t;

// GPIO中断回调函数类型
typedef void (*hal_gpio_irq_callback_t)(uint32_t pin, void* user_data);

/**
 * @brief GPIO接口结构体
 */
typedef struct {
    // 基础GPIO操作
    hal_ret_t (*init)(uint32_t pin, const hal_gpio_config_t* config);
    hal_ret_t (*deinit)(uint32_t pin);
    hal_ret_t (*write)(uint32_t pin, hal_gpio_state_t state);
    hal_gpio_state_t (*read)(uint32_t pin);
    hal_ret_t (*toggle)(uint32_t pin);
    
    // 中断操作
    hal_ret_t (*enable_irq)(uint32_t pin, hal_gpio_it_mode_t mode, 
                           hal_gpio_irq_callback_t callback, void* user_data);
    hal_ret_t (*disable_irq)(uint32_t pin);
    
    // 高级功能
    hal_ret_t (*set_drive_strength)(uint32_t pin, uint32_t strength);
    hal_ret_t (*get_port_value)(uint32_t port, uint32_t* value);
    hal_ret_t (*set_port_value)(uint32_t port, uint32_t value, uint32_t mask);
    
} hal_gpio_ops_t;

// GPIO HAL接口函数
hal_ret_t hal_gpio_init(uint32_t pin, const hal_gpio_config_t* config);
hal_ret_t hal_gpio_deinit(uint32_t pin);
hal_ret_t hal_gpio_write(uint32_t pin, hal_gpio_state_t state);
hal_gpio_state_t hal_gpio_read(uint32_t pin);
hal_ret_t hal_gpio_toggle(uint32_t pin);

// GPIO中断操作
hal_ret_t hal_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode, 
                             hal_gpio_irq_callback_t callback, void* user_data);
hal_ret_t hal_gpio_disable_irq(uint32_t pin);

// 高级GPIO操作
hal_ret_t hal_gpio_set_drive_strength(uint32_t pin, uint32_t strength);
hal_ret_t hal_gpio_get_port_value(uint32_t port, uint32_t* value);
hal_ret_t hal_gpio_set_port_value(uint32_t port, uint32_t value, uint32_t mask);

// 平台特定GPIO操作注册
hal_ret_t hal_gpio_register_ops(const hal_gpio_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H__ */
