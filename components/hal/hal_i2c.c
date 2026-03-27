#include "hal_i2c.h"

static const hal_i2c_ops_t* g_i2c_ops = NULL;

hal_ret_t hal_i2c_register_ops(const hal_i2c_ops_t* ops) {
    g_i2c_ops = ops;
    return MAIX_HAL_OK;
}

hal_ret_t hal_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config) {
    if (g_i2c_ops && g_i2c_ops->init) return g_i2c_ops->init(handle, i2c_id, config);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_deinit(hal_i2c_handle_t handle) {
    if (g_i2c_ops && g_i2c_ops->deinit) return g_i2c_ops->deinit(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_scan_device(hal_i2c_handle_t handle, uint16_t device_addr, uint32_t timeout) {
    if (g_i2c_ops && g_i2c_ops->scan_device) return g_i2c_ops->scan_device(handle, device_addr, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr, const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (g_i2c_ops && g_i2c_ops->master_transmit) return g_i2c_ops->master_transmit(handle, device_addr, tx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr, uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (g_i2c_ops && g_i2c_ops->master_receive) return g_i2c_ops->master_receive(handle, device_addr, rx_data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size, const uint8_t* data, size_t size, uint32_t timeout) {
    if (g_i2c_ops && g_i2c_ops->mem_write) return g_i2c_ops->mem_write(handle, device_addr, mem_addr, mem_addr_size, data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size, uint8_t* data, size_t size, uint32_t timeout) {
    if (g_i2c_ops && g_i2c_ops->mem_read) return g_i2c_ops->mem_read(handle, device_addr, mem_addr, mem_addr_size, data, size, timeout);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_master_transmit_async(hal_i2c_handle_t handle, uint16_t device_addr, const uint8_t* tx_data, size_t size, hal_i2c_callback_t callback, void* user_data) {
    if (g_i2c_ops && g_i2c_ops->master_transmit_async) return g_i2c_ops->master_transmit_async(handle, device_addr, tx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_master_receive_async(hal_i2c_handle_t handle, uint16_t device_addr, uint8_t* rx_data, size_t size, hal_i2c_callback_t callback, void* user_data) {
    if (g_i2c_ops && g_i2c_ops->master_receive_async) return g_i2c_ops->master_receive_async(handle, device_addr, rx_data, size, callback, user_data);
    return MAIX_HAL_NOT_SUPPORTED;
}

bool hal_i2c_is_busy(hal_i2c_handle_t handle) {
    if (g_i2c_ops && g_i2c_ops->is_busy) return g_i2c_ops->is_busy(handle);
    return false;
}

hal_ret_t hal_i2c_abort(hal_i2c_handle_t handle) {
    if (g_i2c_ops && g_i2c_ops->abort) return g_i2c_ops->abort(handle);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_set_clock_speed(hal_i2c_handle_t handle, uint32_t clock_speed) {
    if (g_i2c_ops && g_i2c_ops->set_clock_speed) return g_i2c_ops->set_clock_speed(handle, clock_speed);
    return MAIX_HAL_NOT_SUPPORTED;
}

hal_ret_t hal_i2c_get_clock_speed(hal_i2c_handle_t handle, uint32_t* clock_speed) {
    if (g_i2c_ops && g_i2c_ops->get_clock_speed) return g_i2c_ops->get_clock_speed(handle, clock_speed);
    return MAIX_HAL_NOT_SUPPORTED;
}
