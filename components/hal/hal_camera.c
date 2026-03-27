#include "hal_camera.h"

static const hal_camera_ops_t* g_camera_ops = NULL;

hal_ret_t hal_camera_register_ops(const hal_camera_ops_t* ops) {
    g_camera_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_camera_open(hal_camera_handle_t* handle, const hal_camera_config_t* config) {
    if (g_camera_ops && g_camera_ops->open) return g_camera_ops->open(handle, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_camera_close(hal_camera_handle_t handle) {
    if (g_camera_ops && g_camera_ops->close) return g_camera_ops->close(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_camera_start(hal_camera_handle_t handle) {
    if (g_camera_ops && g_camera_ops->start) return g_camera_ops->start(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_camera_stop(hal_camera_handle_t handle) {
    if (g_camera_ops && g_camera_ops->stop) return g_camera_ops->stop(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

bool hal_camera_frame_ready(hal_camera_handle_t handle) {
    if (g_camera_ops && g_camera_ops->frame_ready) return g_camera_ops->frame_ready(handle);
    return false;
}

hal_ret_t hal_camera_read_frame(hal_camera_handle_t handle, uint8_t* buffer, size_t size) {
    if (g_camera_ops && g_camera_ops->read_frame) return g_camera_ops->read_frame(handle, buffer, size);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_camera_get_size(hal_camera_handle_t handle, uint16_t* width, uint16_t* height) {
    if (g_camera_ops && g_camera_ops->get_size) return g_camera_ops->get_size(handle, width, height);
    return MAIX_HAL_NOT_SUPPORTED;
}
