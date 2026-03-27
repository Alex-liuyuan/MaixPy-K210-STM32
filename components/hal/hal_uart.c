#include "hal_uart.h"

static const hal_uart_ops_t* g_uart_ops = NULL;

hal_ret_t hal_uart_register_ops(const hal_uart_ops_t* ops) {
    g_uart_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config) {
    if (g_uart_ops && g_uart_ops->init) return g_uart_ops->init(handle, uart_id, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_deinit(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->deinit) return g_uart_ops->deinit(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (g_uart_ops && g_uart_ops->transmit) return g_uart_ops->transmit(handle, tx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (g_uart_ops && g_uart_ops->receive) return g_uart_ops->receive(handle, rx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_transmit_async(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size,
                                 hal_uart_callback_t callback, void* user_data) {
    if (g_uart_ops && g_uart_ops->transmit_async) return g_uart_ops->transmit_async(handle, tx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_receive_async(hal_uart_handle_t handle, uint8_t* rx_data, size_t size,
                                hal_uart_callback_t callback, void* user_data) {
    if (g_uart_ops && g_uart_ops->receive_async) return g_uart_ops->receive_async(handle, rx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_putchar(hal_uart_handle_t handle, uint8_t ch) {
    if (g_uart_ops && g_uart_ops->putchar) return g_uart_ops->putchar(handle, ch);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_getchar(hal_uart_handle_t handle, uint8_t* ch, uint32_t timeout) {
    if (g_uart_ops && g_uart_ops->getchar) return g_uart_ops->getchar(handle, ch, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

size_t hal_uart_get_rx_count(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->get_rx_count) return g_uart_ops->get_rx_count(handle);
    return 0;
}

size_t hal_uart_get_tx_count(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->get_tx_count) return g_uart_ops->get_tx_count(handle);
    return 0;
}

hal_ret_t hal_uart_flush_rx(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->flush_rx) return g_uart_ops->flush_rx(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_flush_tx(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->flush_tx) return g_uart_ops->flush_tx(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

bool hal_uart_is_busy(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->is_busy) return g_uart_ops->is_busy(handle);
    return false;
}

hal_ret_t hal_uart_abort(hal_uart_handle_t handle) {
    if (g_uart_ops && g_uart_ops->abort) return g_uart_ops->abort(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_set_baudrate(hal_uart_handle_t handle, uint32_t baudrate) {
    if (g_uart_ops && g_uart_ops->set_baudrate) return g_uart_ops->set_baudrate(handle, baudrate);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_get_baudrate(hal_uart_handle_t handle, uint32_t* baudrate) {
    if (g_uart_ops && g_uart_ops->get_baudrate) return g_uart_ops->get_baudrate(handle, baudrate);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_set_config(hal_uart_handle_t handle, const hal_uart_config_t* config) {
    if (g_uart_ops && g_uart_ops->set_config) return g_uart_ops->set_config(handle, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_enable_rx_idle_detection(hal_uart_handle_t handle, bool enable) {
    if (g_uart_ops && g_uart_ops->enable_rx_idle_detection) return g_uart_ops->enable_rx_idle_detection(handle, enable);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_uart_set_rx_timeout(hal_uart_handle_t handle, uint32_t timeout_ms) {
    if (g_uart_ops && g_uart_ops->set_rx_timeout) return g_uart_ops->set_rx_timeout(handle, timeout_ms);
    return MAIX_HAL_NOT_SUPPORTED;
}
