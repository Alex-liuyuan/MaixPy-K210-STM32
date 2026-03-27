#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief I2C硬件抽象层接口
 * @file hal_i2c.h
 * @author MaixPy Nano RT-Thread Team
 */

// I2C设备句柄
typedef void* hal_i2c_handle_t;

// I2C传输完成回调
typedef void (*hal_i2c_callback_t)(hal_i2c_handle_t handle, hal_ret_t result, void* user_data);

// I2C内存地址大小
typedef enum {
    HAL_I2C_MEMADD_SIZE_8BIT = 0,
    HAL_I2C_MEMADD_SIZE_16BIT
} hal_i2c_memaddr_size_t;

/**
 * @brief I2C接口结构体
 */
typedef struct {
    // 基础I2C操作
    hal_ret_t (*init)(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config);
    hal_ret_t (*deinit)(hal_i2c_handle_t handle);
    
    // 设备扫描
    hal_ret_t (*scan_device)(hal_i2c_handle_t handle, uint16_t device_addr, uint32_t timeout);
    
    // 主模式数据传输
    hal_ret_t (*master_transmit)(hal_i2c_handle_t handle, uint16_t device_addr, 
                                const uint8_t* tx_data, size_t size, uint32_t timeout);
    hal_ret_t (*master_receive)(hal_i2c_handle_t handle, uint16_t device_addr,
                               uint8_t* rx_data, size_t size, uint32_t timeout);
    
    // 内存读写操作
    hal_ret_t (*mem_write)(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr,
                          hal_i2c_memaddr_size_t mem_addr_size, const uint8_t* data, 
                          size_t size, uint32_t timeout);
    hal_ret_t (*mem_read)(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr,
                         hal_i2c_memaddr_size_t mem_addr_size, uint8_t* data,
                         size_t size, uint32_t timeout);
    
    // 异步传输
    hal_ret_t (*master_transmit_async)(hal_i2c_handle_t handle, uint16_t device_addr,
                                      const uint8_t* tx_data, size_t size,
                                      hal_i2c_callback_t callback, void* user_data);
    hal_ret_t (*master_receive_async)(hal_i2c_handle_t handle, uint16_t device_addr,
                                     uint8_t* rx_data, size_t size,
                                     hal_i2c_callback_t callback, void* user_data);
    
    // 状态查询
    bool (*is_busy)(hal_i2c_handle_t handle);
    hal_ret_t (*abort)(hal_i2c_handle_t handle);
    
    // 配置操作
    hal_ret_t (*set_clock_speed)(hal_i2c_handle_t handle, uint32_t clock_speed);
    hal_ret_t (*get_clock_speed)(hal_i2c_handle_t handle, uint32_t* clock_speed);
    
} hal_i2c_ops_t;

// I2C HAL接口函数
hal_ret_t hal_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config);
hal_ret_t hal_i2c_deinit(hal_i2c_handle_t handle);

// 设备扫描
hal_ret_t hal_i2c_scan_device(hal_i2c_handle_t handle, uint16_t device_addr, uint32_t timeout);

// 主模式数据传输
hal_ret_t hal_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr, 
                                 const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t hal_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                uint8_t* rx_data, size_t size, uint32_t timeout);

// 内存读写操作
hal_ret_t hal_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr,
                           hal_i2c_memaddr_size_t mem_addr_size, const uint8_t* data, 
                           size_t size, uint32_t timeout);
hal_ret_t hal_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr, uint16_t mem_addr,
                          hal_i2c_memaddr_size_t mem_addr_size, uint8_t* data,
                          size_t size, uint32_t timeout);

// 异步传输
hal_ret_t hal_i2c_master_transmit_async(hal_i2c_handle_t handle, uint16_t device_addr,
                                       const uint8_t* tx_data, size_t size,
                                       hal_i2c_callback_t callback, void* user_data);
hal_ret_t hal_i2c_master_receive_async(hal_i2c_handle_t handle, uint16_t device_addr,
                                      uint8_t* rx_data, size_t size,
                                      hal_i2c_callback_t callback, void* user_data);

// 状态和控制
bool hal_i2c_is_busy(hal_i2c_handle_t handle);
hal_ret_t hal_i2c_abort(hal_i2c_handle_t handle);
hal_ret_t hal_i2c_set_clock_speed(hal_i2c_handle_t handle, uint32_t clock_speed);
hal_ret_t hal_i2c_get_clock_speed(hal_i2c_handle_t handle, uint32_t* clock_speed);

// 平台特定I2C操作注册
hal_ret_t hal_i2c_register_ops(const hal_i2c_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_I2C_H__ */
