#ifndef __HAL_SPI_H__
#define __HAL_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief SPI硬件抽象层接口
 * @file hal_spi.h  
 * @author MaixPy-K210-STM32 Team
 */

// SPI设备句柄
typedef void* hal_spi_handle_t;

// SPI传输完成回调
typedef void (*hal_spi_callback_t)(hal_spi_handle_t handle, hal_ret_t result, void* user_data);

/**
 * @brief SPI接口结构体
 */
typedef struct {
    // 基础SPI操作
    hal_ret_t (*init)(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config);
    hal_ret_t (*deinit)(hal_spi_handle_t handle);
    
    // 同步传输
    hal_ret_t (*transmit)(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
    hal_ret_t (*receive)(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
    hal_ret_t (*transmit_receive)(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                 uint8_t* rx_data, size_t size, uint32_t timeout);
    
    // 异步传输
    hal_ret_t (*transmit_async)(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size,
                               hal_spi_callback_t callback, void* user_data);
    hal_ret_t (*receive_async)(hal_spi_handle_t handle, uint8_t* rx_data, size_t size,
                              hal_spi_callback_t callback, void* user_data);
    hal_ret_t (*transmit_receive_async)(hal_spi_handle_t handle, const uint8_t* tx_data,
                                       uint8_t* rx_data, size_t size,
                                       hal_spi_callback_t callback, void* user_data);
    
    // 状态查询
    bool (*is_busy)(hal_spi_handle_t handle);
    hal_ret_t (*abort)(hal_spi_handle_t handle);
    
    // 配置操作
    hal_ret_t (*set_baudrate)(hal_spi_handle_t handle, uint32_t baudrate);
    hal_ret_t (*get_baudrate)(hal_spi_handle_t handle, uint32_t* baudrate);
    
} hal_spi_ops_t;

// SPI HAL接口函数
hal_ret_t hal_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config);
hal_ret_t hal_spi_deinit(hal_spi_handle_t handle);

// 同步传输函数
hal_ret_t hal_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t hal_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
hal_ret_t hal_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                  uint8_t* rx_data, size_t size, uint32_t timeout);

// 异步传输函数
hal_ret_t hal_spi_transmit_async(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size,
                                hal_spi_callback_t callback, void* user_data);
hal_ret_t hal_spi_receive_async(hal_spi_handle_t handle, uint8_t* rx_data, size_t size,
                               hal_spi_callback_t callback, void* user_data);
hal_ret_t hal_spi_transmit_receive_async(hal_spi_handle_t handle, const uint8_t* tx_data,
                                        uint8_t* rx_data, size_t size,
                                        hal_spi_callback_t callback, void* user_data);

// 状态和控制函数
bool hal_spi_is_busy(hal_spi_handle_t handle);
hal_ret_t hal_spi_abort(hal_spi_handle_t handle);
hal_ret_t hal_spi_set_baudrate(hal_spi_handle_t handle, uint32_t baudrate);
hal_ret_t hal_spi_get_baudrate(hal_spi_handle_t handle, uint32_t* baudrate);

// 平台特定SPI操作注册
hal_ret_t hal_spi_register_ops(const hal_spi_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_SPI_H__ */