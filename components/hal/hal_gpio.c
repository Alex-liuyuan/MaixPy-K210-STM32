#include "hal_gpio.h"

static const hal_gpio_ops_t* g_gpio_ops = NULL;

hal_ret_t hal_gpio_register_ops(const hal_gpio_ops_t* ops) {
    g_gpio_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (g_gpio_ops && g_gpio_ops->init) return g_gpio_ops->init(pin, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_deinit(uint32_t pin) {
    if (g_gpio_ops && g_gpio_ops->deinit) return g_gpio_ops->deinit(pin);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    if (g_gpio_ops && g_gpio_ops->write) return g_gpio_ops->write(pin, state);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_gpio_state_t hal_gpio_read(uint32_t pin) {
    if (g_gpio_ops && g_gpio_ops->read) return g_gpio_ops->read(pin);
    return HAL_GPIO_PIN_RESET;
}

hal_ret_t hal_gpio_toggle(uint32_t pin) {
    if (g_gpio_ops && g_gpio_ops->toggle) return g_gpio_ops->toggle(pin);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_enable_irq(uint32_t pin, hal_gpio_it_mode_t mode, 
                             hal_gpio_irq_callback_t callback, void* user_data) {
    if (g_gpio_ops && g_gpio_ops->enable_irq) return g_gpio_ops->enable_irq(pin, mode, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_disable_irq(uint32_t pin) {
    if (g_gpio_ops && g_gpio_ops->disable_irq) return g_gpio_ops->disable_irq(pin);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_set_drive_strength(uint32_t pin, uint32_t strength) {
    if (g_gpio_ops && g_gpio_ops->set_drive_strength) return g_gpio_ops->set_drive_strength(pin, strength);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_get_port_value(uint32_t port, uint32_t* value) {
    if (g_gpio_ops && g_gpio_ops->get_port_value) return g_gpio_ops->get_port_value(port, value);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_gpio_set_port_value(uint32_t port, uint32_t value, uint32_t mask) {
    if (g_gpio_ops && g_gpio_ops->set_port_value) return g_gpio_ops->set_port_value(port, value, mask);
    return MAIX_HAL_NOT_SUPPORTED;
}
