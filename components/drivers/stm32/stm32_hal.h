#ifndef __STM32_HAL_H__
#define __STM32_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_pwm.h"
#include "hal_adc.h"
#include "hal_display.h"
#include "hal_camera.h"

/**
 * @brief STM32平台硬件抽象层
 * @file stm32_hal.h
 * @author MaixPy Nano RT-Thread Team
 */

// STM32特定包含
#if defined(CONFIG_PLATFORM_STM32F407)
#include "stm32f4xx_hal.h"
#define STM32_FAMILY_F4
#else
#error "Current product branch only supports CONFIG_PLATFORM_STM32F407"
#endif

// STM32 GPIO端口定义
typedef enum {
    STM32_GPIO_PORT_A = 0,
    STM32_GPIO_PORT_B,
    STM32_GPIO_PORT_C,
    STM32_GPIO_PORT_D,
    STM32_GPIO_PORT_E,
    STM32_GPIO_PORT_F,
    STM32_GPIO_PORT_G,
    STM32_GPIO_PORT_H,
    STM32_GPIO_PORT_I,
    STM32_GPIO_PORT_J,
    STM32_GPIO_PORT_K,
    STM32_GPIO_PORT_MAX
} stm32_gpio_port_t;

// STM32 GPIO引脚定义
typedef enum {
    STM32_GPIO_PIN_0 = 0,
    STM32_GPIO_PIN_1,
    STM32_GPIO_PIN_2,
    STM32_GPIO_PIN_3,
    STM32_GPIO_PIN_4,
    STM32_GPIO_PIN_5,
    STM32_GPIO_PIN_6,
    STM32_GPIO_PIN_7,
    STM32_GPIO_PIN_8,
    STM32_GPIO_PIN_9,
    STM32_GPIO_PIN_10,
    STM32_GPIO_PIN_11,
    STM32_GPIO_PIN_12,
    STM32_GPIO_PIN_13,
    STM32_GPIO_PIN_14,
    STM32_GPIO_PIN_15,
    STM32_GPIO_PIN_MAX = 16
} stm32_gpio_pin_t;

// STM32 SPI设备定义
typedef enum {
    STM32_SPI_1 = 0,
    STM32_SPI_2,
    STM32_SPI_3,
    STM32_SPI_4,
    STM32_SPI_5,
    STM32_SPI_6,
    STM32_SPI_MAX
} stm32_spi_device_t;

// STM32 I2C设备定义
typedef enum {
    STM32_I2C_1 = 0,
    STM32_I2C_2,
    STM32_I2C_3,
    STM32_I2C_4,
    STM32_I2C_MAX
} stm32_i2c_device_t;

// STM32 UART设备定义
typedef enum {
    STM32_UART_1 = 0,
    STM32_UART_2,
    STM32_UART_3,
    STM32_UART_4,
    STM32_UART_5,
    STM32_UART_6,
    STM32_UART_7,
    STM32_UART_8,
    STM32_UART_MAX
} stm32_uart_device_t;

// STM32 Timer设备定义
typedef enum {
    STM32_TIM_1 = 0,
    STM32_TIM_2,
    STM32_TIM_3,
    STM32_TIM_4,
    STM32_TIM_5,
    STM32_TIM_6,
    STM32_TIM_7,
    STM32_TIM_8,
    STM32_TIM_9,
    STM32_TIM_10,
    STM32_TIM_11,
    STM32_TIM_12,
    STM32_TIM_13,
    STM32_TIM_14,
    STM32_TIM_MAX
} stm32_timer_device_t;

// STM32 ADC设备定义
typedef enum {
    STM32_ADC_1 = 0,
    STM32_ADC_2,
    STM32_ADC_3,
    STM32_ADC_MAX
} stm32_adc_device_t;

// STM32 DAC设备定义
typedef enum {
    STM32_DAC_1 = 0,
    STM32_DAC_2,
    STM32_DAC_MAX
} stm32_dac_device_t;

// STM32平台初始化
hal_ret_t stm32_hal_init(void);

// STM32 GPIO操作
hal_ret_t stm32_gpio_init(uint32_t pin, const hal_gpio_config_t* config);
hal_ret_t stm32_gpio_deinit(uint32_t pin);
hal_ret_t stm32_gpio_write(uint32_t pin, hal_gpio_state_t state);
hal_gpio_state_t stm32_gpio_read(uint32_t pin);
hal_ret_t stm32_gpio_toggle(uint32_t pin);

// STM32 SPI操作
hal_ret_t stm32_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config);
hal_ret_t stm32_spi_deinit(hal_spi_handle_t handle);
hal_ret_t stm32_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                    uint8_t* rx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_spi_transmit_dma(hal_spi_handle_t handle, const uint8_t* tx_data,
                                 size_t size);

// STM32 I2C操作
hal_ret_t stm32_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config);
hal_ret_t stm32_i2c_deinit(hal_i2c_handle_t handle);
hal_ret_t stm32_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr, 
                                   const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                  uint8_t* rx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_i2c_mem_write(hal_i2c_handle_t handle, uint16_t device_addr,
                              uint16_t mem_addr, uint16_t mem_addr_size,
                              const uint8_t* data, size_t size, uint32_t timeout);
hal_ret_t stm32_i2c_mem_read(hal_i2c_handle_t handle, uint16_t device_addr,
                             uint16_t mem_addr, uint16_t mem_addr_size,
                             uint8_t* data, size_t size, uint32_t timeout);

// STM32 UART操作
hal_ret_t stm32_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config);
hal_ret_t stm32_uart_deinit(hal_uart_handle_t handle);
hal_ret_t stm32_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout);
hal_ret_t stm32_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout);
uint16_t stm32_uart_read_dma(hal_uart_handle_t handle, uint8_t* buf, uint16_t max_len);
void stm32_uart_idle_irq_handler(uint32_t uart_id);

// STM32时钟系统
hal_ret_t stm32_rcc_set_system_clock(uint32_t frequency);
uint32_t stm32_rcc_get_system_clock(void);
hal_ret_t stm32_rcc_enable_peripheral_clock(uint32_t peripheral);
hal_ret_t stm32_rcc_disable_peripheral_clock(uint32_t peripheral);

// STM32电源管理
hal_ret_t stm32_pwr_enter_sleep_mode(void);
hal_ret_t stm32_pwr_enter_stop_mode(void);
hal_ret_t stm32_pwr_enter_standby_mode(void);
hal_ret_t stm32_pwr_set_voltage_scaling(uint32_t voltage_scale);

// STM32 ADC操作
typedef struct {
    uint32_t resolution;
    uint32_t data_align;
    uint32_t scan_mode;
    uint32_t continuous_mode;
    uint32_t trigger_mode;
} stm32_adc_config_t;

hal_ret_t stm32_adc_init(uint32_t adc_id, const stm32_adc_config_t* config);
hal_ret_t stm32_adc_deinit(uint32_t adc_id);
hal_ret_t stm32_adc_read_channel(uint32_t adc_id, uint32_t channel, uint16_t* value);
hal_ret_t stm32_adc_start_dma(uint32_t adc_id, uint16_t* buffer, size_t size);
hal_ret_t stm32_adc_stop_dma(uint32_t adc_id);

// STM32 DAC操作
typedef struct {
    uint32_t output_buffer;
    uint32_t trigger;
    uint32_t wave_generation;
} stm32_dac_config_t;

hal_ret_t stm32_dac_init(uint32_t dac_id, const stm32_dac_config_t* config);
hal_ret_t stm32_dac_deinit(uint32_t dac_id);
hal_ret_t stm32_dac_set_value(uint32_t dac_id, uint32_t channel, uint16_t value);
hal_ret_t stm32_dac_start_dma(uint32_t dac_id, uint32_t channel, uint16_t* buffer, size_t size);
hal_ret_t stm32_dac_stop_dma(uint32_t dac_id, uint32_t channel);

// STM32 PWM操作
typedef struct {
    uint32_t prescaler;
    uint32_t period;
    uint32_t pulse;
    uint32_t polarity;
} stm32_pwm_config_t;

hal_ret_t stm32_pwm_init(uint32_t timer_id, uint32_t channel, const stm32_pwm_config_t* config);
hal_ret_t stm32_pwm_deinit(uint32_t timer_id, uint32_t channel);
hal_ret_t stm32_pwm_start(uint32_t timer_id, uint32_t channel);
hal_ret_t stm32_pwm_stop(uint32_t timer_id, uint32_t channel);
hal_ret_t stm32_pwm_set_duty_cycle(uint32_t timer_id, uint32_t channel, uint32_t duty_cycle);

// STM32 DMA操作
typedef struct {
    uint32_t direction;
    uint32_t peripheral_inc;
    uint32_t memory_inc;
    uint32_t peripheral_data_alignment;
    uint32_t memory_data_alignment;
    uint32_t mode;
    uint32_t priority;
} stm32_dma_config_t;

typedef void (*stm32_dma_callback_t)(uint32_t dma_id, uint32_t stream_id, void* user_data);

hal_ret_t stm32_dma_init(uint32_t dma_id, uint32_t stream_id, const stm32_dma_config_t* config);
hal_ret_t stm32_dma_deinit(uint32_t dma_id, uint32_t stream_id);
hal_ret_t stm32_dma_start(uint32_t dma_id, uint32_t stream_id, uint32_t src_addr, 
                         uint32_t dst_addr, uint32_t data_length);
hal_ret_t stm32_dma_stop(uint32_t dma_id, uint32_t stream_id);
hal_ret_t stm32_dma_register_callback(uint32_t dma_id, uint32_t stream_id, 
                                     stm32_dma_callback_t callback, void* user_data);

// STM32 Camera Interface (DCMI)
typedef struct {
    uint32_t sync_mode;
    uint32_t pck_polarity;
    uint32_t vsync_polarity;
    uint32_t hsync_polarity;
    uint32_t capture_rate;
    uint32_t extended_data_mode;
} stm32_dcmi_config_t;

hal_ret_t stm32_dcmi_init(const stm32_dcmi_config_t* config);
hal_ret_t stm32_dcmi_deinit(void);
hal_ret_t stm32_dcmi_start_capture(uint8_t* buffer, size_t size);
hal_ret_t stm32_dcmi_stop_capture(void);
hal_ret_t stm32_dcmi_suspend_capture(void);
hal_ret_t stm32_dcmi_resume_capture(void);
bool stm32_dcmi_frame_ready(void);
void stm32_dcmi_clear_frame_flag(void);

// STM32 LCD (ST7789)
hal_ret_t stm32_lcd_init(uint16_t width, uint16_t height);
hal_ret_t stm32_lcd_show_frame(const uint16_t* buf);
hal_ret_t stm32_lcd_fill(uint16_t color);

// STM32 ADC DMA扫描（签名与stm32_adc_start_dma不同）
hal_ret_t stm32_adc_start_scan_dma(uint32_t adc_id, const uint32_t* channels, uint32_t num);

// STM32 CRC计算
hal_ret_t stm32_crc_init(void);
hal_ret_t stm32_crc_deinit(void);
uint32_t stm32_crc_calculate(const uint8_t* data, size_t size);
uint32_t stm32_crc_accumulate(const uint8_t* data, size_t size);
void stm32_crc_reset(void);

// STM32 RNG随机数生成
hal_ret_t stm32_rng_init(void);
hal_ret_t stm32_rng_deinit(void);
hal_ret_t stm32_rng_generate(uint32_t* random_number);
hal_ret_t stm32_rng_generate_block(uint32_t* random_numbers, size_t count);

// STM32 Flash操作
hal_ret_t stm32_flash_unlock(void);
hal_ret_t stm32_flash_lock(void);
hal_ret_t stm32_flash_erase_sector(uint32_t sector);
hal_ret_t stm32_flash_write(uint32_t address, const uint8_t* data, size_t size);
hal_ret_t stm32_flash_read(uint32_t address, uint8_t* data, size_t size);

// STM32平台操作注册
hal_ret_t stm32_register_hal_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32_HAL_H__ */
