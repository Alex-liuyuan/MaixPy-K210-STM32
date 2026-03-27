#include "hal_display.h"

static const hal_display_ops_t* g_display_ops = NULL;

hal_ret_t hal_display_register_ops(const hal_display_ops_t* ops) {
    g_display_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_display_open(hal_display_handle_t* handle, const hal_display_config_t* config) {
    if (g_display_ops && g_display_ops->open) return g_display_ops->open(handle, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_close(hal_display_handle_t handle) {
    if (g_display_ops && g_display_ops->close) return g_display_ops->close(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_show(hal_display_handle_t handle, const uint8_t* data, size_t size) {
    if (g_display_ops && g_display_ops->show) return g_display_ops->show(handle, data, size);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_fill(hal_display_handle_t handle, uint8_t r, uint8_t g, uint8_t b) {
    if (g_display_ops && g_display_ops->fill) return g_display_ops->fill(handle, r, g, b);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_set_backlight(hal_display_handle_t handle, uint8_t level) {
    if (g_display_ops && g_display_ops->set_backlight) return g_display_ops->set_backlight(handle, level);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_set_rotation(hal_display_handle_t handle, uint16_t rotation) {
    if (g_display_ops && g_display_ops->set_rotation) return g_display_ops->set_rotation(handle, rotation);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_display_get_size(hal_display_handle_t handle, uint16_t* width, uint16_t* height) {
    if (g_display_ops && g_display_ops->get_size) return g_display_ops->get_size(handle, width, height);
    return MAIX_HAL_NOT_SUPPORTED;
}
