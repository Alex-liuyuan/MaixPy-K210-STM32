#include "hal_spi.h"

static const hal_spi_ops_t* g_spi_ops = NULL;

hal_ret_t hal_spi_register_ops(const hal_spi_ops_t* ops) {
    g_spi_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config) {
    if (g_spi_ops && g_spi_ops->init) return g_spi_ops->init(handle, spi_id, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_deinit(hal_spi_handle_t handle) {
    if (g_spi_ops && g_spi_ops->deinit) return g_spi_ops->deinit(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (g_spi_ops && g_spi_ops->transmit) return g_spi_ops->transmit(handle, tx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (g_spi_ops && g_spi_ops->receive) return g_spi_ops->receive(handle, rx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                  uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (g_spi_ops && g_spi_ops->transmit_receive) return g_spi_ops->transmit_receive(handle, tx_data, rx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_transmit_async(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size,
                                hal_spi_callback_t callback, void* user_data) {
    if (g_spi_ops && g_spi_ops->transmit_async) return g_spi_ops->transmit_async(handle, tx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_receive_async(hal_spi_handle_t handle, uint8_t* rx_data, size_t size,
                               hal_spi_callback_t callback, void* user_data) {
    if (g_spi_ops && g_spi_ops->receive_async) return g_spi_ops->receive_async(handle, rx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_transmit_receive_async(hal_spi_handle_t handle, const uint8_t* tx_data,
                                        uint8_t* rx_data, size_t size,
                                        hal_spi_callback_t callback, void* user_data) {
    if (g_spi_ops && g_spi_ops->transmit_receive_async) return g_spi_ops->transmit_receive_async(handle, tx_data, rx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

bool hal_spi_is_busy(hal_spi_handle_t handle) {
    if (g_spi_ops && g_spi_ops->is_busy) return g_spi_ops->is_busy(handle);
    return false;
}

hal_ret_t hal_spi_abort(hal_spi_handle_t handle) {
    if (g_spi_ops && g_spi_ops->abort) return g_spi_ops->abort(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_set_baudrate(hal_spi_handle_t handle, uint32_t baudrate) {
    if (g_spi_ops && g_spi_ops->set_baudrate) return g_spi_ops->set_baudrate(handle, baudrate);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_spi_get_baudrate(hal_spi_handle_t handle, uint32_t* baudrate) {
    if (g_spi_ops && g_spi_ops->get_baudrate) return g_spi_ops->get_baudrate(handle, baudrate);
    return MAIX_HAL_NOT_SUPPORTED;
}
