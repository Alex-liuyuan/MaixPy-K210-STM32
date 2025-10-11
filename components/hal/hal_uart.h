#ifndef __HAL_UART_H__
#define __HAL_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief UART硬件抽象层接口
 * @file hal_uart.h
 * @author MaixPy-K210-STM32 Team
 */

// UART设备句柄
typedef void* hal_uart_handle_t;

// UART传输完成回调
typedef void (*hal_uart_callback_t)(hal_uart_handle_t handle, hal_ret_t result, void* user_data);

// UART流控制
typedef enum {
    HAL_UART_HWCONTROL_NONE = 0,
    HAL_UART_HWCONTROL_RTS,
    HAL_UART_HWCONTROL_CTS,
    HAL_UART_HWCONTROL_RTS_CTS
} hal_uart_hwcontrol_t;

// UART扩展配置
typedef struct {
    hal_uart_config_t base;
    hal_uart_hwcontrol_t hw_control;
    bool over_sampling;  // 过采样: 8倍或16倍
    uint32_t rx_timeout; // 接收超时(ms)
} hal_uart_extended_config_t;

/**
 * @brief UART接口结构体
 */
typedef struct {
    // 基础UART操作
    hal_ret_t (*init)(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config);
    hal_ret_t (*deinit)(hal_uart_handle_t handle);
    
    // 同步传输
    hal_ret_t (*transmit)(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
    hal_ret_t (*receive)(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
    
    // 异步传输
    hal_ret_t (*transmit_async)(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size,
                               hal_uart_callback_t callback, void* user_data);
    hal_ret_t (*receive_async)(hal_uart_handle_t handle, uint8_t* rx_data, size_t size,
                              hal_uart_callback_t callback, void* user_data);
    
    // 单字符操作
    hal_ret_t (*putchar)(hal_uart_handle_t handle, uint8_t ch);
    hal_ret_t (*getchar)(hal_uart_handle_t handle, uint8_t* ch, uint32_t timeout);
    
    // 缓冲区操作
    size_t (*get_rx_count)(hal_uart_handle_t handle);
    size_t (*get_tx_count)(hal_uart_handle_t handle);
    hal_ret_t (*flush_rx)(hal_uart_handle_t handle);
    hal_ret_t (*flush_tx)(hal_uart_handle_t handle);
    
    // 状态查询
    bool (*is_busy)(hal_uart_handle_t handle);
    hal_ret_t (*abort)(hal_uart_handle_t handle);
    
    // 配置操作
    hal_ret_t (*set_baudrate)(hal_uart_handle_t handle, uint32_t baudrate);
    hal_ret_t (*get_baudrate)(hal_uart_handle_t handle, uint32_t* baudrate);
    hal_ret_t (*set_config)(hal_uart_handle_t handle, const hal_uart_config_t* config);
    
    // 高级功能
    hal_ret_t (*enable_rx_idle_detection)(hal_uart_handle_t handle, bool enable);
    hal_ret_t (*set_rx_timeout)(hal_uart_handle_t handle, uint32_t timeout_ms);
    
} hal_uart_ops_t;

// UART HAL接口函数
hal_ret_t hal_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config);
hal_ret_t hal_uart_deinit(hal_uart_handle_t handle);

// 同步传输
hal_ret_t hal_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t hal_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);

// 异步传输
hal_ret_t hal_uart_transmit_async(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size,
                                 hal_uart_callback_t callback, void* user_data);
hal_ret_t hal_uart_receive_async(hal_uart_handle_t handle, uint8_t* rx_data, size_t size,
                                hal_uart_callback_t callback, void* user_data);

// 单字符操作
hal_ret_t hal_uart_putchar(hal_uart_handle_t handle, uint8_t ch);
hal_ret_t hal_uart_getchar(hal_uart_handle_t handle, uint8_t* ch, uint32_t timeout);

// 字符串操作(便利函数)
hal_ret_t hal_uart_puts(hal_uart_handle_t handle, const char* str);
hal_ret_t hal_uart_printf(hal_uart_handle_t handle, const char* format, ...);

// 缓冲区操作
size_t hal_uart_get_rx_count(hal_uart_handle_t handle);
size_t hal_uart_get_tx_count(hal_uart_handle_t handle);
hal_ret_t hal_uart_flush_rx(hal_uart_handle_t handle);
hal_ret_t hal_uart_flush_tx(hal_uart_handle_t handle);

// 状态和控制
bool hal_uart_is_busy(hal_uart_handle_t handle);
hal_ret_t hal_uart_abort(hal_uart_handle_t handle);
hal_ret_t hal_uart_set_baudrate(hal_uart_handle_t handle, uint32_t baudrate);
hal_ret_t hal_uart_get_baudrate(hal_uart_handle_t handle, uint32_t* baudrate);
hal_ret_t hal_uart_set_config(hal_uart_handle_t handle, const hal_uart_config_t* config);

// 高级功能
hal_ret_t hal_uart_enable_rx_idle_detection(hal_uart_handle_t handle, bool enable);
hal_ret_t hal_uart_set_rx_timeout(hal_uart_handle_t handle, uint32_t timeout_ms);

// 平台特定UART操作注册
hal_ret_t hal_uart_register_ops(const hal_uart_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_UART_H__ */