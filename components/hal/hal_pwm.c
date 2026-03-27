#include "hal_pwm.h"

static const hal_pwm_ops_t* g_pwm_ops = NULL;

hal_ret_t hal_pwm_register_ops(const hal_pwm_ops_t* ops) {
    g_pwm_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_pwm_init(uint32_t timer_id, uint32_t channel, const hal_pwm_config_t* config) {
    if (g_pwm_ops && g_pwm_ops->init) return g_pwm_ops->init(timer_id, channel, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_pwm_deinit(uint32_t timer_id, uint32_t channel) {
    if (g_pwm_ops && g_pwm_ops->deinit) return g_pwm_ops->deinit(timer_id, channel);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_pwm_start(uint32_t timer_id, uint32_t channel) {
    if (g_pwm_ops && g_pwm_ops->start) return g_pwm_ops->start(timer_id, channel);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_pwm_stop(uint32_t timer_id, uint32_t channel) {
    if (g_pwm_ops && g_pwm_ops->stop) return g_pwm_ops->stop(timer_id, channel);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_pwm_set_duty(uint32_t timer_id, uint32_t channel, uint32_t duty_permille) {
    if (g_pwm_ops && g_pwm_ops->set_duty) return g_pwm_ops->set_duty(timer_id, channel, duty_permille);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_pwm_get_duty(uint32_t timer_id, uint32_t channel, uint32_t* duty_permille) {
    if (g_pwm_ops && g_pwm_ops->get_duty) return g_pwm_ops->get_duty(timer_id, channel, duty_permille);
    return MAIX_HAL_NOT_SUPPORTED;
}
