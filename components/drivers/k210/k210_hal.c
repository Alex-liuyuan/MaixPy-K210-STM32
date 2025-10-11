#include "k210_hal.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief K210平台硬件抽象层实现
 * @file k210_hal.c
 * @author MaixPy-K210-STM32 Team
 */

#ifdef CONFIG_PLATFORM_K210

// K210设备句柄结构体
typedef struct {
    uint32_t device_id;
    void* platform_handle;
    bool initialized;
} k210_device_handle_t;

// K210 GPIO实现
hal_ret_t k210_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (!config) {
        return HAL_INVALID_PARAM;
    }
    
    // 配置FPIOA
    fpioa_function_t function;
    if (pin < K210_GPIOHS_MAX) {
        function = FUNC_GPIOHS0 + pin;
        fpioa_set_function(config->pin, function);
        
        // 配置GPIOHS
        gpio_drive_mode_t drive_mode;
        switch (config->mode) {
            case HAL_GPIO_MODE_INPUT:
                drive_mode = (config->pull == HAL_GPIO_PULL_UP) ? GPIO_DM_INPUT_PULL_UP :
                           (config->pull == HAL_GPIO_PULL_DOWN) ? GPIO_DM_INPUT_PULL_DOWN :
                           GPIO_DM_INPUT;
                break;
            case HAL_GPIO_MODE_OUTPUT:
                drive_mode = GPIO_DM_OUTPUT;
                break;
            default:
                return HAL_NOT_SUPPORTED;
        }
        
        gpiohs_set_drive_mode(pin, drive_mode);
    } else {
        // 使用普通GPIO
        function = FUNC_GPIO0 + (pin - K210_GPIOHS_MAX);
        fpioa_set_function(config->pin, function);
        
        gpio_drive_mode_t drive_mode;
        switch (config->mode) {
            case HAL_GPIO_MODE_INPUT:
                drive_mode = (config->pull == HAL_GPIO_PULL_UP) ? GPIO_DM_INPUT_PULL_UP :
                           (config->pull == HAL_GPIO_PULL_DOWN) ? GPIO_DM_INPUT_PULL_DOWN :
                           GPIO_DM_INPUT;
                break;
            case HAL_GPIO_MODE_OUTPUT:
                drive_mode = GPIO_DM_OUTPUT;
                break;
            default:
                return HAL_NOT_SUPPORTED;
        }
        
        gpio_set_drive_mode(pin - K210_GPIOHS_MAX, drive_mode);
    }
    
    return HAL_OK;
}

hal_ret_t k210_gpio_deinit(uint32_t pin) {
    // K210 GPIO去初始化
    return HAL_OK;
}

hal_ret_t k210_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    gpio_pin_value_t value = (state == HAL_GPIO_PIN_SET) ? GPIO_PV_HIGH : GPIO_PV_LOW;
    
    if (pin < K210_GPIOHS_MAX) {
        gpiohs_set_pin(pin, value);
    } else {
        gpio_set_pin(pin - K210_GPIOHS_MAX, value);
    }
    
    return HAL_OK;
}

hal_gpio_state_t k210_gpio_read(uint32_t pin) {
    gpio_pin_value_t value;
    
    if (pin < K210_GPIOHS_MAX) {
        value = gpiohs_get_pin(pin);
    } else {
        value = gpio_get_pin(pin - K210_GPIOHS_MAX);
    }
    
    return (value == GPIO_PV_HIGH) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

hal_ret_t k210_gpio_toggle(uint32_t pin) {
    hal_gpio_state_t current_state = k210_gpio_read(pin);
    hal_gpio_state_t new_state = (current_state == HAL_GPIO_PIN_SET) ? 
                                HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET;
    return k210_gpio_write(pin, new_state);
}

// K210 SPI实现
hal_ret_t k210_spi_init(hal_spi_handle_t* handle, uint32_t spi_id, const hal_spi_config_t* config) {
    if (!handle || !config || spi_id >= K210_SPI_DEVICE_MAX) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = malloc(sizeof(k210_device_handle_t));
    if (!dev_handle) {
        return HAL_NO_MEMORY;
    }
    
    dev_handle->device_id = spi_id;
    dev_handle->initialized = true;
    
    // 配置SPI
    spi_work_mode_t work_mode = (config->mode == HAL_SPI_MODE_MASTER) ? 
                               SPI_WORK_MODE_0 : SPI_WORK_MODE_1;
    spi_frame_format_t frame_format = SPI_FF_STANDARD;
    spi_instruction_length_t inst_len = SPI_AITM_STANDARD;
    spi_address_length_t addr_len = SPI_AAITM_STANDARD;
    spi_wait_cycles_t wait_cycles = SPI_AITM_STANDARD;
    spi_instruction_address_trans_mode_t trans_mode = SPI_AITM_STANDARD;
    
    size_t data_bit_length = (config->datasize == HAL_SPI_DATASIZE_8BIT) ? 8 : 16;
    
    spi_init(spi_id, work_mode, frame_format, data_bit_length, 0);
    spi_set_clk_rate(spi_id, config->baudrate);
    
    *handle = dev_handle;
    return HAL_OK;
}

hal_ret_t k210_spi_deinit(hal_spi_handle_t handle) {
    if (!handle) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    free(dev_handle);
    
    return HAL_OK;
}

hal_ret_t k210_spi_transmit(hal_spi_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (!handle || !tx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    spi_init_non_standard(dev_handle->device_id, 0, NULL, 0, NULL, 
                         size, (uint8_t*)tx_data);
    spi_send_data_normal_dma(DMAC_CHANNEL0, dev_handle->device_id, 
                            SPI_CHIP_SELECT_0, tx_data, size, SPI_TRANS_CHAR);
    
    return HAL_OK;
}

hal_ret_t k210_spi_receive(hal_spi_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (!handle || !rx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    spi_receive_data_standard_dma(DMAC_CHANNEL0, dev_handle->device_id,
                                 SPI_CHIP_SELECT_0, NULL, 0, rx_data, size);
    
    return HAL_OK;
}

hal_ret_t k210_spi_transmit_receive(hal_spi_handle_t handle, const uint8_t* tx_data, 
                                   uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (!handle || !tx_data || !rx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    spi_send_data_standard_dma(DMAC_CHANNEL0, dev_handle->device_id,
                              SPI_CHIP_SELECT_0, tx_data, size);
    spi_receive_data_standard_dma(DMAC_CHANNEL1, dev_handle->device_id,
                                 SPI_CHIP_SELECT_0, NULL, 0, rx_data, size);
    
    return HAL_OK;
}

// K210 I2C实现
hal_ret_t k210_i2c_init(hal_i2c_handle_t* handle, uint32_t i2c_id, const hal_i2c_config_t* config) {
    if (!handle || !config || i2c_id >= K210_I2C_DEVICE_MAX) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = malloc(sizeof(k210_device_handle_t));
    if (!dev_handle) {
        return HAL_NO_MEMORY;
    }
    
    dev_handle->device_id = i2c_id;
    dev_handle->initialized = true;
    
    // 初始化I2C
    i2c_init(i2c_id, config->slave_address, 7, config->clock_speed);
    
    *handle = dev_handle;
    return HAL_OK;
}

hal_ret_t k210_i2c_deinit(hal_i2c_handle_t handle) {
    if (!handle) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    free(dev_handle);
    
    return HAL_OK;
}

hal_ret_t k210_i2c_master_transmit(hal_i2c_handle_t handle, uint16_t device_addr, 
                                  const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (!handle || !tx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    int ret = i2c_send_data(dev_handle->device_id, device_addr, tx_data, size);
    return (ret == 0) ? HAL_OK : HAL_ERROR;
}

hal_ret_t k210_i2c_master_receive(hal_i2c_handle_t handle, uint16_t device_addr,
                                 uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (!handle || !rx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    int ret = i2c_recv_data(dev_handle->device_id, device_addr, NULL, 0, rx_data, size);
    return (ret == 0) ? HAL_OK : HAL_ERROR;
}

// K210 UART实现
hal_ret_t k210_uart_init(hal_uart_handle_t* handle, uint32_t uart_id, const hal_uart_config_t* config) {
    if (!handle || !config || uart_id >= K210_UART_DEVICE_MAX) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = malloc(sizeof(k210_device_handle_t));
    if (!dev_handle) {
        return HAL_NO_MEMORY;
    }
    
    dev_handle->device_id = uart_id + 1; // K210 UART从1开始
    dev_handle->initialized = true;
    
    // 初始化UART
    uart_init(dev_handle->device_id);
    uart_configure(dev_handle->device_id, config->baudrate, 
                  (config->wordlength == HAL_UART_WORDLENGTH_8B) ? 8 : 9);
    
    *handle = dev_handle;
    return HAL_OK;
}

hal_ret_t k210_uart_deinit(hal_uart_handle_t handle) {
    if (!handle) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    free(dev_handle);
    
    return HAL_OK;
}

hal_ret_t k210_uart_transmit(hal_uart_handle_t handle, const uint8_t* tx_data, size_t size, uint32_t timeout) {
    if (!handle || !tx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    for (size_t i = 0; i < size; i++) {
        uart_send_data(dev_handle->device_id, tx_data[i]);
    }
    
    return HAL_OK;
}

hal_ret_t k210_uart_receive(hal_uart_handle_t handle, uint8_t* rx_data, size_t size, uint32_t timeout) {
    if (!handle || !rx_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    k210_device_handle_t* dev_handle = (k210_device_handle_t*)handle;
    
    for (size_t i = 0; i < size; i++) {
        // 等待数据可用
        uint32_t start_time = get_time();
        while (!uart_receive_data(dev_handle->device_id, &rx_data[i])) {
            if ((get_time() - start_time) > timeout) {
                return HAL_TIMEOUT;
            }
        }
    }
    
    return HAL_OK;
}

// K210平台初始化
hal_ret_t k210_hal_init(void) {
    // 初始化系统时钟
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    
    // 初始化FPIOA
    fpioa_init();
    
    // 注册HAL操作
    return k210_register_hal_ops();
}

// K210系统控制
hal_ret_t k210_sysctl_set_cpu_frequency(uint32_t frequency) {
    sysctl_cpu_set_freq(frequency);
    return HAL_OK;
}

uint32_t k210_sysctl_get_cpu_frequency(void) {
    return sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
}

// K210电源管理
hal_ret_t k210_power_set_mode(uint32_t mode) {
    // K210电源模式设置
    return HAL_OK;
}

hal_ret_t k210_power_deep_sleep(uint32_t sleep_time_ms) {
    msleep(sleep_time_ms);
    return HAL_OK;
}

// K210 DVP实现
hal_ret_t k210_dvp_init(const k210_dvp_config_t* config) {
    if (!config) {
        return HAL_INVALID_PARAM;
    }
    
    // 初始化DVP
    // 这里需要根据具体的摄像头模块进行配置
    
    return HAL_OK;
}

hal_ret_t k210_dvp_deinit(void) {
    // DVP去初始化
    return HAL_OK;
}

hal_ret_t k210_dvp_start_capture(void) {
    // 开始图像捕获
    return HAL_OK;
}

hal_ret_t k210_dvp_stop_capture(void) {
    // 停止图像捕获
    return HAL_OK;
}

hal_ret_t k210_dvp_get_image(uint8_t** image_data, size_t* size) {
    if (!image_data || !size) {
        return HAL_INVALID_PARAM;
    }
    
    // 获取图像数据
    // 实际实现需要根据DVP缓冲区管理
    
    return HAL_OK;
}

// K210 KPU实现
hal_ret_t k210_kpu_init(void) {
    // 初始化KPU
    return HAL_OK;
}

hal_ret_t k210_kpu_deinit(void) {
    // KPU去初始化
    return HAL_OK;
}

hal_ret_t k210_kpu_load_model(k210_kpu_model_t* model, const uint8_t* model_data, size_t size) {
    if (!model || !model_data || size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    // 加载KPU模型
    // 实际实现需要调用KPU SDK
    
    return HAL_OK;
}

hal_ret_t k210_kpu_unload_model(k210_kpu_model_t model) {
    if (!model) {
        return HAL_INVALID_PARAM;
    }
    
    // 卸载KPU模型
    
    return HAL_OK;
}

hal_ret_t k210_kpu_run_inference(k210_kpu_model_t model, const uint8_t* input_data, 
                                uint8_t* output_data, size_t output_size) {
    if (!model || !input_data || !output_data || output_size == 0) {
        return HAL_INVALID_PARAM;
    }
    
    // 运行KPU推理
    // 实际实现需要调用KPU SDK
    
    return HAL_OK;
}

#endif /* CONFIG_PLATFORM_K210 */

// K210平台操作注册
hal_ret_t k210_register_hal_ops(void) {
#ifdef CONFIG_PLATFORM_K210
    // 注册GPIO操作
    static const hal_gpio_ops_t gpio_ops = {
        .init = k210_gpio_init,
        .deinit = k210_gpio_deinit,
        .write = k210_gpio_write,
        .read = k210_gpio_read,
        .toggle = k210_gpio_toggle,
        .enable_irq = NULL, // 需要实现
        .disable_irq = NULL, // 需要实现
        .set_drive_strength = NULL, // 需要实现
        .get_port_value = NULL, // 需要实现
        .set_port_value = NULL, // 需要实现
    };
    hal_gpio_register_ops(&gpio_ops);
    
    // 注册SPI操作
    static const hal_spi_ops_t spi_ops = {
        .init = k210_spi_init,
        .deinit = k210_spi_deinit,
        .transmit = k210_spi_transmit,
        .receive = k210_spi_receive,
        .transmit_receive = k210_spi_transmit_receive,
        .transmit_async = NULL, // 需要实现
        .receive_async = NULL, // 需要实现
        .transmit_receive_async = NULL, // 需要实现
        .is_busy = NULL, // 需要实现
        .abort = NULL, // 需要实现
        .set_baudrate = NULL, // 需要实现
        .get_baudrate = NULL, // 需要实现
    };
    hal_spi_register_ops(&spi_ops);
    
    // 注册I2C操作
    static const hal_i2c_ops_t i2c_ops = {
        .init = k210_i2c_init,
        .deinit = k210_i2c_deinit,
        .scan_device = NULL, // 需要实现
        .master_transmit = k210_i2c_master_transmit,
        .master_receive = k210_i2c_master_receive,
        .mem_write = NULL, // 需要实现
        .mem_read = NULL, // 需要实现
        .master_transmit_async = NULL, // 需要实现
        .master_receive_async = NULL, // 需要实现
        .is_busy = NULL, // 需要实现
        .abort = NULL, // 需要实现
        .set_clock_speed = NULL, // 需要实现
        .get_clock_speed = NULL, // 需要实现
    };
    hal_i2c_register_ops(&i2c_ops);
    
    // 注册UART操作
    static const hal_uart_ops_t uart_ops = {
        .init = k210_uart_init,
        .deinit = k210_uart_deinit,
        .transmit = k210_uart_transmit,
        .receive = k210_uart_receive,
        .transmit_async = NULL, // 需要实现
        .receive_async = NULL, // 需要实现
        .putchar = NULL, // 需要实现
        .getchar = NULL, // 需要实现
        .get_rx_count = NULL, // 需要实现
        .get_tx_count = NULL, // 需要实现
        .flush_rx = NULL, // 需要实现
        .flush_tx = NULL, // 需要实现
        .is_busy = NULL, // 需要实现
        .abort = NULL, // 需要实现
        .set_baudrate = NULL, // 需要实现
        .get_baudrate = NULL, // 需要实现
        .set_config = NULL, // 需要实现
        .enable_rx_idle_detection = NULL, // 需要实现
        .set_rx_timeout = NULL, // 需要实现
    };
    hal_uart_register_ops(&uart_ops);
    
    return HAL_OK;
#else
    return HAL_NOT_SUPPORTED;
#endif
}