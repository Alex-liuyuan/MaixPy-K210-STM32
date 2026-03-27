#include "hal_adc.h"

static const hal_adc_ops_t* g_adc_ops = NULL;

hal_ret_t hal_adc_register_ops(const hal_adc_ops_t* ops) {
    g_adc_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_adc_init(hal_adc_handle_t* handle, uint32_t adc_id, const hal_adc_config_t* config) {
    if (g_adc_ops && g_adc_ops->init) return g_adc_ops->init(handle, adc_id, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_adc_deinit(hal_adc_handle_t handle) {
    if (g_adc_ops && g_adc_ops->deinit) return g_adc_ops->deinit(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_adc_read(hal_adc_handle_t handle, uint32_t channel, uint16_t* value) {
    if (g_adc_ops && g_adc_ops->read) return g_adc_ops->read(handle, channel, value);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_adc_read_voltage(hal_adc_handle_t handle, uint32_t channel, float vref, float* voltage) {
    if (g_adc_ops && g_adc_ops->read_voltage) return g_adc_ops->read_voltage(handle, channel, vref, voltage);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_adc_start_dma(hal_adc_handle_t handle, uint32_t* channels, size_t count, uint16_t* buffer) {
    if (g_adc_ops && g_adc_ops->start_dma) return g_adc_ops->start_dma(handle, channels, count, buffer);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_adc_stop_dma(hal_adc_handle_t handle) {
    if (g_adc_ops && g_adc_ops->stop_dma) return g_adc_ops->stop_dma(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}
