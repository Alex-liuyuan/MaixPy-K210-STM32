/**
 * @file k210_gpio.c
 * @brief K210 GPIO驱动实现（基于FPIOA + GPIOHS）
 *
 * K210引脚编码：pin = (fpioa_io << 16) | gpiohs_num
 * FPIOA负责将物理引脚映射到GPIOHS功能，GPIOHS负责实际IO操作。
 */

#include "../hal/hal_gpio.h"
#include <stdio.h>

#if defined(CONFIG_PLATFORM_K210)

#include "fpioa.h"
#include "gpiohs.h"
#include "sysctl.h"

/* K210 GPIOHS最多32个 */
#define K210_GPIOHS_MAX 32

/* 从统一pin编码中提取FPIOA IO号和GPIOHS号 */
#define PIN_FPIOA_IO(pin)   (((pin) >> 16) & 0xFFFF)
#define PIN_GPIOHS(pin)     ((pin) & 0xFFFF)

hal_ret_t k210_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (!config) return MAIX_HAL_INVALID_PARAM;

    uint32_t fpioa_io = PIN_FPIOA_IO(pin);
    uint32_t gpiohs_num = PIN_GPIOHS(pin);

    if (gpiohs_num >= K210_GPIOHS_MAX) return MAIX_HAL_INVALID_PARAM;

    /* 将物理引脚映射到GPIOHS功能 */
    fpioa_set_function(fpioa_io, FUNC_GPIOHS0 + gpiohs_num);

    /* 设置方向 */
    switch (config->mode) {
        case HAL_GPIO_MODE_INPUT:
            gpiohs_set_drive_mode(gpiohs_num, GPIO_DM_INPUT);
            if (config->pull == HAL_GPIO_PULL_UP) {
                gpiohs_set_drive_mode(gpiohs_num, GPIO_DM_INPUT_PULL_UP);
            } else if (config->pull == HAL_GPIO_PULL_DOWN) {
                gpiohs_set_drive_mode(gpiohs_num, GPIO_DM_INPUT_PULL_DOWN);
            }
            break;
        case HAL_GPIO_MODE_OUTPUT:
            gpiohs_set_drive_mode(gpiohs_num, GPIO_DM_OUTPUT);
            break;
        default:
            return MAIX_HAL_INVALID_PARAM;
    }

    return MAIX_HAL_OK;
}

hal_ret_t k210_gpio_deinit(uint32_t pin) {
    uint32_t gpiohs_num = PIN_GPIOHS(pin);
    if (gpiohs_num >= K210_GPIOHS_MAX) return MAIX_HAL_INVALID_PARAM;
    /* 恢复为输入模式 */
    gpiohs_set_drive_mode(gpiohs_num, GPIO_DM_INPUT);
    return MAIX_HAL_OK;
}

hal_ret_t k210_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    uint32_t gpiohs_num = PIN_GPIOHS(pin);
    if (gpiohs_num >= K210_GPIOHS_MAX) return MAIX_HAL_INVALID_PARAM;
    gpiohs_set_pin(gpiohs_num, state == HAL_GPIO_PIN_SET ? GPIO_PV_HIGH : GPIO_PV_LOW);
    return MAIX_HAL_OK;
}

hal_gpio_state_t k210_gpio_read(uint32_t pin) {
    uint32_t gpiohs_num = PIN_GPIOHS(pin);
    if (gpiohs_num >= K210_GPIOHS_MAX) return HAL_GPIO_PIN_RESET;
    gpio_pin_value_t val = gpiohs_get_pin(gpiohs_num);
    return (val == GPIO_PV_HIGH) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

hal_ret_t k210_gpio_toggle(uint32_t pin) {
    hal_gpio_state_t current = k210_gpio_read(pin);
    hal_gpio_state_t next = (current == HAL_GPIO_PIN_SET)
                            ? HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET;
    return k210_gpio_write(pin, next);
}

/* 中断支持 */
static hal_gpio_irq_callback_t s_irq_callbacks[K210_GPIOHS_MAX] = {0};
static void* s_irq_user_data[K210_GPIOHS_MAX] = {0};

static int k210_gpiohs_irq_handler(void* ctx) {
    uint32_t gpiohs_num = (uint32_t)(uintptr_t)ctx;
    if (gpiohs_num < K210_GPIOHS_MAX && s_irq_callbacks[gpiohs_num]) {
        /* 重建pin编码（仅gpiohs部分，fpioa_io未知） */
        s_irq_callbacks[gpiohs_num](gpiohs_num, s_irq_user_data[gpiohs_num]);
    }
    return 0;
}

hal_ret_t k210_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode,
                                hal_gpio_irq_callback_t callback, void* user_data) {
    uint32_t gpiohs_num = PIN_GPIOHS(pin);
    if (gpiohs_num >= K210_GPIOHS_MAX || !callback) return MAIX_HAL_INVALID_PARAM;

    s_irq_callbacks[gpiohs_num] = callback;
    s_irq_user_data[gpiohs_num] = user_data;

    gpio_pin_edge_t edge;
    switch (mode) {
        case HAL_GPIO_IT_RISING:         edge = GPIO_PE_RISING;  break;
        case HAL_GPIO_IT_FALLING:        edge = GPIO_PE_FALLING; break;
        case HAL_GPIO_IT_RISING_FALLING: edge = GPIO_PE_BOTH;    break;
        default: return MAIX_HAL_INVALID_PARAM;
    }

    gpiohs_set_pin_edge(gpiohs_num, edge);
    gpiohs_irq_register(gpiohs_num, 1, k210_gpiohs_irq_handler,
                         (void*)(uintptr_t)gpiohs_num);
    return MAIX_HAL_OK;
}

hal_ret_t k210_gpio_disable_irq(uint32_t pin) {
    uint32_t gpiohs_num = PIN_GPIOHS(pin);
    if (gpiohs_num >= K210_GPIOHS_MAX) return MAIX_HAL_INVALID_PARAM;
    gpiohs_irq_unregister(gpiohs_num);
    s_irq_callbacks[gpiohs_num] = NULL;
    s_irq_user_data[gpiohs_num] = NULL;
    return MAIX_HAL_OK;
}

#endif /* CONFIG_PLATFORM_K210 */
