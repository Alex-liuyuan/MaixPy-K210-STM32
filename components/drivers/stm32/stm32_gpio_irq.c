/**
 * @file stm32_gpio_irq.c
 * @brief STM32 GPIO中断实现
 */

#include "stm32_hal.h"
#include "hal_gpio.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407)

#define GPIO_IRQ_MAX_PINS 16

typedef struct {
    hal_gpio_irq_callback_t callback;
    void*    user_data;
    uint32_t pin;
    bool     active;
} gpio_irq_entry_t;

static gpio_irq_entry_t s_irq_table[GPIO_IRQ_MAX_PINS];

/* EXTI线号 → IRQn 映射 */
static IRQn_Type exti_irqn(uint32_t line) {
    switch (line) {
        case 0:  return EXTI0_IRQn;
        case 1:  return EXTI1_IRQn;
        case 2:  return EXTI2_IRQn;
        case 3:  return EXTI3_IRQn;
        case 4:  return EXTI4_IRQn;
        case 5:  case 6:  case 7:  case 8:  case 9:  return EXTI9_5_IRQn;
        default: return EXTI15_10_IRQn;
    }
}

hal_ret_t stm32_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode,
                                hal_gpio_irq_callback_t callback, void* user_data) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num  = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= GPIO_IRQ_MAX_PINS || !callback) {
        return MAIX_HAL_INVALID_PARAM;
    }

    /* 配置EXTI */
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin  = (1u << pin_num);
    cfg.Pull = GPIO_NOPULL;
    switch (mode) {
        case HAL_GPIO_IT_RISING:         cfg.Mode = GPIO_MODE_IT_RISING;         break;
        case HAL_GPIO_IT_FALLING:        cfg.Mode = GPIO_MODE_IT_FALLING;        break;
        case HAL_GPIO_IT_RISING_FALLING: cfg.Mode = GPIO_MODE_IT_RISING_FALLING; break;
        default: return MAIX_HAL_INVALID_PARAM;
    }

    extern GPIO_TypeDef* gpio_port_map[];
    HAL_GPIO_Init(gpio_port_map[port_num], &cfg);

    /* 注册回调 */
    s_irq_table[pin_num].callback  = callback;
    s_irq_table[pin_num].user_data = user_data;
    s_irq_table[pin_num].pin       = pin;
    s_irq_table[pin_num].active    = true;

    IRQn_Type irqn = exti_irqn(pin_num);
    HAL_NVIC_SetPriority(irqn, 5, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return MAIX_HAL_OK;
}

hal_ret_t stm32_gpio_disable_irq(uint32_t pin) {
    uint32_t pin_num = pin & 0xFFFF;
    if (pin_num >= GPIO_IRQ_MAX_PINS) return MAIX_HAL_INVALID_PARAM;

    s_irq_table[pin_num].active   = false;
    s_irq_table[pin_num].callback = NULL;

    HAL_NVIC_DisableIRQ(exti_irqn(pin_num));
    return MAIX_HAL_OK;
}

/* 统一EXTI回调分发 */
static void dispatch_exti(uint32_t pin_num) {
    if (pin_num < GPIO_IRQ_MAX_PINS && s_irq_table[pin_num].active
        && s_irq_table[pin_num].callback) {
        s_irq_table[pin_num].callback(s_irq_table[pin_num].pin,
                                      s_irq_table[pin_num].user_data);
    }
}

/* STM32 HAL EXTI回调（弱符号覆盖） */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    for (uint32_t i = 0; i < GPIO_IRQ_MAX_PINS; i++) {
        if (GPIO_Pin & (1u << i)) {
            dispatch_exti(i);
        }
    }
}

/* IRQ处理函数 */
void EXTI0_IRQHandler(void)      { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);  }
void EXTI1_IRQHandler(void)      { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);  }
void EXTI2_IRQHandler(void)      { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);  }
void EXTI3_IRQHandler(void)      { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);  }
void EXTI4_IRQHandler(void)      { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);  }
void EXTI9_5_IRQHandler(void)    {
    for (uint16_t p = GPIO_PIN_5; p <= GPIO_PIN_9; p <<= 1)
        HAL_GPIO_EXTI_IRQHandler(p);
}
void EXTI15_10_IRQHandler(void)  {
    for (uint16_t p = GPIO_PIN_10; p <= GPIO_PIN_15; p <<= 1)
        HAL_GPIO_EXTI_IRQHandler(p);
}

#endif /* STM32 platforms */
