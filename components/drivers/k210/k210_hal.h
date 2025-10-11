#ifndef __K210_HAL_H__
#define __K210_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief K210平台硬件抽象层
 * @file k210_hal.h
 * @author MaixPy-K210-STM32 Team
 */

// K210特定包含
#ifdef CONFIG_PLATFORM_K210
#include <platform.h>
#include <stdint.h>
#include <stdio.h>
#include <sysctl.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <gpio.h>
#include <spi.h>
#include <i2c.h>
#include <uart.h>
#include <pwm.h>
#include <timer.h>
#include <plic.h>
#include <sleep.h>
#endif

// K210 GPIO引脚定义
typedef enum {
    K210_GPIO_0 = 0,
    K210_GPIO_1,
    K210_GPIO_2,
    K210_GPIO_3,
    K210_GPIO_4,
    K210_GPIO_5,
    K210_GPIO_6,
    K210_GPIO_7,
    K210_GPIO_MAX = 8
} k210_gpio_pin_t;

// K210 GPIOHS引脚定义 (高速GPIO)
typedef enum {
    K210_GPIOHS_0 = 0,
    K210_GPIOHS_1,
    K210_GPIOHS_2,
    K210_GPIOHS_3,
    K210_GPIOHS_4,
    K210_GPIOHS_5,
    K210_GPIOHS_6,
    K210_GPIOHS_7,
    K210_GPIOHS_8,
    K210_GPIOHS_9,
    K210_GPIOHS_10,
    K210_GPIOHS_11,
    K210_GPIOHS_12,
    K210_GPIOHS_13,
    K210_GPIOHS_14,
    K210_GPIOHS_15,
    K210_GPIOHS_16,
    K210_GPIOHS_17,
    K210_GPIOHS_18,
    K210_GPIOHS_19,
    K210_GPIOHS_20,
    K210_GPIOHS_21,
    K210_GPIOHS_22,
    K210_GPIOHS_23,
    K210_GPIOHS_24,
    K210_GPIOHS_25,
    K210_GPIOHS_26,
    K210_GPIOHS_27,
    K210_GPIOHS_28,
    K210_GPIOHS_29,
    K210_GPIOHS_30,
    K210_GPIOHS_31,
    K210_GPIOHS_MAX = 32
} k210_gpiohs_pin_t;

// K210 FPIOA引脚定义
typedef enum {
    K210_FPIOA_PIN_0 = 0,
    K210_FPIOA_PIN_1,
    K210_FPIOA_PIN_2,
    K210_FPIOA_PIN_3,
    K210_FPIOA_PIN_4,
    K210_FPIOA_PIN_5,
    K210_FPIOA_PIN_6,
    K210_FPIOA_PIN_7,
    K210_FPIOA_PIN_8,
    K210_FPIOA_PIN_9,
    K210_FPIOA_PIN_10,
    K210_FPIOA_PIN_11,
    K210_FPIOA_PIN_12,
    K210_FPIOA_PIN_13,
    K210_FPIOA_PIN_14,
    K210_FPIOA_PIN_15,
    K210_FPIOA_PIN_16,
    K210_FPIOA_PIN_17,
    K210_FPIOA_PIN_18,
    K210_FPIOA_PIN_19,
    K210_FPIOA_PIN_20,
    K210_FPIOA_PIN_21,
    K210_FPIOA_PIN_22,
    K210_FPIOA_PIN_23,
    K210_FPIOA_PIN_24,
    K210_FPIOA_PIN_25,
    K210_FPIOA_PIN_26,
    K210_FPIOA_PIN_27,
    K210_FPIOA_PIN_28,
    K210_FPIOA_PIN_29,
    K210_FPIOA_PIN_30,
    K210_FPIOA_PIN_31,
    K210_FPIOA_PIN_32,
    K210_FPIOA_PIN_33,
    K210_FPIOA_PIN_34,
    K210_FPIOA_PIN_35,
    K210_FPIOA_PIN_36,
    K210_FPIOA_PIN_37,
    K210_FPIOA_PIN_38,
    K210_FPIOA_PIN_39,
    K210_FPIOA_PIN_40,
    K210_FPIOA_PIN_41,
    K210_FPIOA_PIN_42,
    K210_FPIOA_PIN_43,
    K210_FPIOA_PIN_44,
    K210_FPIOA_PIN_45,
    K210_FPIOA_PIN_46,
    K210_FPIOA_PIN_47,
    K210_FPIOA_PIN_MAX = 48
} k210_fpioa_pin_t;

// K210 SPI设备定义
typedef enum {
    K210_SPI_DEVICE_0 = 0,
    K210_SPI_DEVICE_1,
    K210_SPI_DEVICE_2,
    K210_SPI_DEVICE_3,
    K210_SPI_DEVICE_MAX = 4
} k210_spi_device_t;

// K210 I2C设备定义
typedef enum {
    K210_I2C_DEVICE_0 = 0,
    K210_I2C_DEVICE_1,
    K210_I2C_DEVICE_2,
    K210_I2C_DEVICE_MAX = 3
} k210_i2c_device_t;

// K210 UART设备定义
typedef enum {
    K210_UART_DEVICE_1 = 0,
    K210_UART_DEVICE_2,
    K210_UART_DEVICE_3,
    K210_UART_DEVICE_MAX = 3
} k210_uart_device_t;

// K210 PWM设备定义
typedef enum {
    K210_PWM_DEVICE_0 = 0,
    K210_PWM_DEVICE_1,
    K210_PWM_DEVICE_2,
    K210_PWM_DEVICE_MAX = 3
} k210_pwm_device_t;

// K210 Timer设备定义
typedef enum {
    K210_TIMER_DEVICE_0 = 0,
    K210_TIMER_DEVICE_1,
    K210_TIMER_DEVICE_2,
    K210_TIMER_DEVICE_MAX = 3
} k210_timer_device_t;

// K210平台初始化
hal_ret_t k210_hal_init(void);

// K210 GPIO操作
hal_ret_t k210_gpio_init(uint32_t pin, const hal_gpio_config_t* config);
hal_ret_t k210_gpio_deinit(uint32_t pin);
hal_ret_t k210_gpio_write(uint32_t pin, hal_gpio_state_t state);
hal_gpio_state_t k210_gpio_read(uint32_t pin);
hal_ret_t k210_gpio_toggle(uint32_t pin);

// K210 SPI操作
hal_ret_t k210_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config);
hal_ret_t k210_spi_deinit(hal_spi_handle_t handle);
hal_ret_t k210_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t k210_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
hal_ret_t k210_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                   uint8_t* rx_data, size_t size, uint32_t timeout);

// K210 I2C操作
hal_ret_t k210_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config);
hal_ret_t k210_i2c_deinit(hal_i2c_handle_t handle);
hal_ret_t k210_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr, 
                                  const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t k210_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                 uint8_t* rx_data, size_t size, uint32_t timeout);

// K210 UART操作
hal_ret_t k210_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config);
hal_ret_t k210_uart_deinit(hal_uart_handle_t handle);
hal_ret_t k210_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t k210_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);

// K210时钟系统
hal_ret_t k210_sysctl_set_cpu_frequency(uint32_t frequency);
uint32_t k210_sysctl_get_cpu_frequency(void);

// K210电源管理
hal_ret_t k210_power_set_mode(uint32_t mode);
hal_ret_t k210_power_deep_sleep(uint32_t sleep_time_ms);

// K210 DVP(摄像头)接口
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t framerate;
} k210_dvp_config_t;

hal_ret_t k210_dvp_init(const k210_dvp_config_t* config);
hal_ret_t k210_dvp_deinit(void);
hal_ret_t k210_dvp_start_capture(void);
hal_ret_t k210_dvp_stop_capture(void);
hal_ret_t k210_dvp_get_image(uint8_t** image_data, size_t* size);

// K210 KPU(AI加速器)接口
typedef void* k210_kpu_model_t;

hal_ret_t k210_kpu_init(void);
hal_ret_t k210_kpu_deinit(void);
hal_ret_t k210_kpu_load_model(k210_kpu_model_t* model, const uint8_t* model_data, size_t size);
hal_ret_t k210_kpu_unload_model(k210_kpu_model_t model);
hal_ret_t k210_kpu_run_inference(k210_kpu_model_t model, const uint8_t* input_data, 
                                uint8_t* output_data, size_t output_size);

// K210平台操作注册
hal_ret_t k210_register_hal_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* __K210_HAL_H__ */