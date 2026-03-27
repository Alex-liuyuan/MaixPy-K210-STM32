#include "hal_audio.h"

static const hal_audio_ops_t* g_audio_ops = NULL;

hal_ret_t hal_audio_register_ops(const hal_audio_ops_t* ops) {
    g_audio_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_audio_open(hal_audio_handle_t* handle, const hal_audio_config_t* config) {
    if (g_audio_ops && g_audio_ops->open) return g_audio_ops->open(handle, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_close(hal_audio_handle_t handle) {
    if (g_audio_ops && g_audio_ops->close) return g_audio_ops->close(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_start(hal_audio_handle_t handle) {
    if (g_audio_ops && g_audio_ops->start) return g_audio_ops->start(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_stop(hal_audio_handle_t handle) {
    if (g_audio_ops && g_audio_ops->stop) return g_audio_ops->stop(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_read(hal_audio_handle_t handle, int16_t* buffer, size_t samples) {
    if (g_audio_ops && g_audio_ops->read) return g_audio_ops->read(handle, buffer, samples);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_write(hal_audio_handle_t handle, const int16_t* buffer, size_t samples) {
    if (g_audio_ops && g_audio_ops->write) return g_audio_ops->write(handle, buffer, samples);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_set_volume(hal_audio_handle_t handle, uint8_t volume) {
    if (g_audio_ops && g_audio_ops->set_volume) return g_audio_ops->set_volume(handle, volume);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_audio_get_volume(hal_audio_handle_t handle, uint8_t* volume) {
    if (g_audio_ops && g_audio_ops->get_volume) return g_audio_ops->get_volume(handle, volume);
    return MAIX_HAL_NOT_SUPPORTED;
}
