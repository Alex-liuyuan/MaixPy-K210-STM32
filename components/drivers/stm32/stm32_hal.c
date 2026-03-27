#include "stm32_hal.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_pwm.h"
#include "hal_adc.h"
#include "hal_camera.h"
#include "hal_display.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief STM32平台硬件抽象层实现
 * @file stm32_hal.c
 * @author MaixPy Nano RT-Thread Team
 */

#if defined(CONFIG_PLATFORM_STM32F407)

// STM32设备句柄结构体
typedef struct {
    uint32_t device_id;
    void* stm32_handle;
    bool initialized;
} stm32_device_handle_t;

// GPIO端口映射表
static GPIO_TypeDef* gpio_port_map[] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH
#ifdef GPIOI
    , GPIOI
#endif
};

static hal_ret_t stm32_gpio_enable_clock(uint32_t port_num) {
    switch (port_num) {
        case STM32_GPIO_PORT_A: __HAL_RCC_GPIOA_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_B: __HAL_RCC_GPIOB_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_C: __HAL_RCC_GPIOC_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_D: __HAL_RCC_GPIOD_CLK_ENABLE(); return MAIX_HAL_OK;
        case STM32_GPIO_PORT_E: __HAL_RCC_GPIOE_CLK_ENABLE(); return MAIX_HAL_OK;
#ifdef __HAL_RCC_GPIOF_CLK_ENABLE
        case STM32_GPIO_PORT_F: __HAL_RCC_GPIOF_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOG_CLK_ENABLE
        case STM32_GPIO_PORT_G: __HAL_RCC_GPIOG_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOH_CLK_ENABLE
        case STM32_GPIO_PORT_H: __HAL_RCC_GPIOH_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
#ifdef __HAL_RCC_GPIOI_CLK_ENABLE
        case STM32_GPIO_PORT_I: __HAL_RCC_GPIOI_CLK_ENABLE(); return MAIX_HAL_OK;
#endif
        default: return MAIX_HAL_INVALID_PARAM;
    }
}

static uint32_t stm32_gpio_mode_convert(hal_gpio_mode_t mode) {
    switch (mode) {
        case HAL_GPIO_MODE_INPUT:  return GPIO_MODE_INPUT;
        case HAL_GPIO_MODE_OUTPUT: return GPIO_MODE_OUTPUT_PP;
        case HAL_GPIO_MODE_AF:     return GPIO_MODE_AF_PP;
        case HAL_GPIO_MODE_ANALOG: return GPIO_MODE_ANALOG;
        default:                   return UINT32_MAX;
    }
}

static uint32_t stm32_gpio_pull_convert(hal_gpio_pull_t pull) {
    switch (pull) {
        case HAL_GPIO_PULL_NONE: return GPIO_NOPULL;
        case HAL_GPIO_PULL_UP:   return GPIO_PULLUP;
        case HAL_GPIO_PULL_DOWN: return GPIO_PULLDOWN;
        default:                 return UINT32_MAX;
    }
}

static uint32_t stm32_gpio_speed_convert(hal_gpio_speed_t speed) {
    switch (speed) {
        case HAL_GPIO_SPEED_LOW:       return GPIO_SPEED_FREQ_LOW;
        case HAL_GPIO_SPEED_MEDIUM:    return GPIO_SPEED_FREQ_MEDIUM;
        case HAL_GPIO_SPEED_HIGH:      return GPIO_SPEED_FREQ_HIGH;
        case HAL_GPIO_SPEED_VERY_HIGH: return GPIO_SPEED_FREQ_VERY_HIGH;
        default:                       return UINT32_MAX;
    }
}

static hal_ret_t stm32_i2c_mem_write_adapter(hal_i2c_handle_t handle, uint16_t device_addr,
                                             uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                             const uint8_t* data, size_t size, uint32_t timeout) {
    return stm32_i2c_mem_write(handle, device_addr, mem_addr, (uint16_t)mem_addr_size,
                               data, size, timeout);
}

static hal_ret_t stm32_i2c_mem_read_adapter(hal_i2c_handle_t handle, uint16_t device_addr,
                                            uint16_t mem_addr, hal_i2c_memaddr_size_t mem_addr_size,
                                            uint8_t* data, size_t size, uint32_t timeout) {
    return stm32_i2c_mem_read(handle, device_addr, mem_addr, (uint16_t)mem_addr_size,
                              data, size, timeout);
}

// STM32 GPIO实现
hal_ret_t stm32_gpio_init(uint32_t pin, const hal_gpio_config_t* config) {
    if (!config) return MAIX_HAL_INVALID_PARAM;
    
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;
    
    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    if (port_num >= (sizeof(gpio_port_map) / sizeof(gpio_port_map[0]))) {
        return MAIX_HAL_INVALID_PARAM;
    }

    if (stm32_gpio_enable_clock(port_num) != MAIX_HAL_OK) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    // 配置GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = (1 << pin_num);

    GPIO_InitStruct.Mode = stm32_gpio_mode_convert(config->mode);
    GPIO_InitStruct.Pull = stm32_gpio_pull_convert(config->pull);
    GPIO_InitStruct.Speed = stm32_gpio_speed_convert(config->speed);
    GPIO_InitStruct.Alternate = config->alternate;

    if (GPIO_InitStruct.Mode == UINT32_MAX ||
        GPIO_InitStruct.Pull == UINT32_MAX ||
        GPIO_InitStruct.Speed == UINT32_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    
    HAL_GPIO_Init(gpio_port_map[port_num], &GPIO_InitStruct);
    return MAIX_HAL_OK;
}

hal_ret_t stm32_gpio_write(uint32_t pin, hal_gpio_state_t state) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    GPIO_PinState pin_state = (state == HAL_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(gpio_port_map[port_num], (1 << pin_num), pin_state);
    return MAIX_HAL_OK;
}

hal_gpio_state_t stm32_gpio_read(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return HAL_GPIO_PIN_RESET;
    }
    GPIO_PinState s = HAL_GPIO_ReadPin(gpio_port_map[port_num], (1 << pin_num));
    return (s == GPIO_PIN_SET) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
}

hal_ret_t stm32_gpio_toggle(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    HAL_GPIO_TogglePin(gpio_port_map[port_num], (1 << pin_num));
    return MAIX_HAL_OK;
}

hal_ret_t stm32_gpio_deinit(uint32_t pin) {
    uint32_t port_num = (pin >> 16) & 0xFFFF;
    uint32_t pin_num = pin & 0xFFFF;

    if (port_num >= STM32_GPIO_PORT_MAX || pin_num >= STM32_GPIO_PIN_MAX) {
        return MAIX_HAL_INVALID_PARAM;
    }
    HAL_GPIO_DeInit(gpio_port_map[port_num], (1 << pin_num));
    return MAIX_HAL_OK;
}

// STM32平台初始化
hal_ret_t stm32_hal_init(void) {
    HAL_Init();
    return stm32_register_hal_ops();
}

// STM32平台操作注册
hal_ret_t stm32_register_hal_ops(void) {
    static const hal_gpio_ops_t gpio_ops = {
        .init   = stm32_gpio_init,
        .deinit = stm32_gpio_deinit,
        .write  = stm32_gpio_write,
        .read   = stm32_gpio_read,
        .toggle = stm32_gpio_toggle,
    };
    static const hal_spi_ops_t spi_ops = {
        .init = stm32_spi_init,
        .deinit = stm32_spi_deinit,
        .transmit = stm32_spi_transmit,
        .receive = stm32_spi_receive,
        .transmit_receive = stm32_spi_transmit_receive,
    };
    static const hal_i2c_ops_t i2c_ops = {
        .init = stm32_i2c_init,
        .deinit = stm32_i2c_deinit,
        .master_transmit = stm32_i2c_master_transmit,
        .master_receive = stm32_i2c_master_receive,
        .mem_write = stm32_i2c_mem_write_adapter,
        .mem_read = stm32_i2c_mem_read_adapter,
    };
    static const hal_uart_ops_t uart_ops = {
        .init = stm32_uart_init,
        .deinit = stm32_uart_deinit,
        .transmit = stm32_uart_transmit,
        .receive = stm32_uart_receive,
    };

    /* PWM ops — 适配 hal_pwm_ops_t 签名 */
    static hal_ret_t pwm_init_adapter(uint32_t timer_id, uint32_t channel,
                                       const hal_pwm_config_t* config) {
        stm32_pwm_config_t cfg = {
            .prescaler = config->prescaler,
            .period    = config->period,
            .pulse     = config->pulse,
            .polarity  = config->polarity,
        };
        return stm32_pwm_init(timer_id, channel, &cfg);
    }
    static const hal_pwm_ops_t pwm_ops = {
        .init     = pwm_init_adapter,
        .deinit   = stm32_pwm_deinit,
        .start    = stm32_pwm_start,
        .stop     = stm32_pwm_stop,
        .set_duty = stm32_pwm_set_duty_cycle,
    };

    /* ---- ADC 适配器 ---- */
    /* handle 存储 adc_id（编码为指针） */
    static hal_ret_t adc_init_adapter(hal_adc_handle_t* handle, uint32_t adc_id,
                                       const hal_adc_config_t* config) {
        stm32_adc_config_t cfg = {0};
        cfg.resolution      = ADC_RESOLUTION_12B;
        cfg.data_align       = 0;
        cfg.scan_mode        = config->scan_mode ? 1 : 0;
        cfg.continuous_mode  = config->continuous ? 1 : 0;
        cfg.trigger_mode     = 0;
        hal_ret_t ret = stm32_adc_init(adc_id, &cfg);
        if (ret == MAIX_HAL_OK) {
            *handle = (hal_adc_handle_t)(uintptr_t)(adc_id + 1); /* 非NULL */
        }
        return ret;
    }
    static hal_ret_t adc_deinit_adapter(hal_adc_handle_t handle) {
        uint32_t adc_id = (uint32_t)(uintptr_t)handle - 1;
        return stm32_adc_deinit(adc_id);
    }
    static hal_ret_t adc_read_adapter(hal_adc_handle_t handle, uint32_t channel,
                                       uint16_t* value) {
        uint32_t adc_id = (uint32_t)(uintptr_t)handle - 1;
        return stm32_adc_read_channel(adc_id, channel, value);
    }
    static hal_ret_t adc_read_voltage_adapter(hal_adc_handle_t handle, uint32_t channel,
                                               float vref, float* voltage) {
        uint16_t raw = 0;
        hal_ret_t ret = adc_read_adapter(handle, channel, &raw);
        if (ret == MAIX_HAL_OK) {
            *voltage = (float)raw * vref / 4095.0f;
        }
        return ret;
    }
    static hal_ret_t adc_start_dma_adapter(hal_adc_handle_t handle, uint32_t* channels,
                                            size_t count, uint16_t* buffer) {
        uint32_t adc_id = (uint32_t)(uintptr_t)handle - 1;
        (void)buffer; /* STM32驱动使用内部缓冲 */
        return stm32_adc_start_scan_dma(adc_id, channels, (uint32_t)count);
    }
    static hal_ret_t adc_stop_dma_adapter(hal_adc_handle_t handle) {
        uint32_t adc_id = (uint32_t)(uintptr_t)handle - 1;
        return stm32_adc_stop_dma(adc_id);
    }
    static const hal_adc_ops_t adc_ops = {
        .init         = adc_init_adapter,
        .deinit       = adc_deinit_adapter,
        .read         = adc_read_adapter,
        .read_voltage = adc_read_voltage_adapter,
        .start_dma    = adc_start_dma_adapter,
        .stop_dma     = adc_stop_dma_adapter,
    };

    /* ---- Camera (DCMI) 适配器 ---- */
    typedef struct {
        uint16_t width;
        uint16_t height;
        uint8_t* frame_buf;
        size_t   frame_size;
        bool     started;
    } stm32_camera_ctx_t;
    static stm32_camera_ctx_t s_cam_ctx;

    static hal_ret_t cam_open_adapter(hal_camera_handle_t* handle,
                                       const hal_camera_config_t* config) {
        stm32_dcmi_config_t dcmi_cfg = {0};
        dcmi_cfg.pck_polarity = 1;  /* rising edge */
        hal_ret_t ret = stm32_dcmi_init(&dcmi_cfg);
        if (ret != MAIX_HAL_OK) return ret;
        s_cam_ctx.width  = config->width;
        s_cam_ctx.height = config->height;
        /* RGB565: 2 bytes/pixel */
        s_cam_ctx.frame_size = (size_t)config->width * config->height * 2;
        s_cam_ctx.frame_buf  = (uint8_t*)hal_malloc(s_cam_ctx.frame_size);
        if (!s_cam_ctx.frame_buf) { stm32_dcmi_deinit(); return MAIX_HAL_NO_MEMORY; }
        s_cam_ctx.started = false;
        *handle = (hal_camera_handle_t)&s_cam_ctx;
        return MAIX_HAL_OK;
    }
    static hal_ret_t cam_close_adapter(hal_camera_handle_t handle) {
        (void)handle;
        if (s_cam_ctx.started) stm32_dcmi_stop_capture();
        stm32_dcmi_deinit();
        if (s_cam_ctx.frame_buf) { hal_free(s_cam_ctx.frame_buf); s_cam_ctx.frame_buf = NULL; }
        s_cam_ctx.started = false;
        return MAIX_HAL_OK;
    }
    static hal_ret_t cam_start_adapter(hal_camera_handle_t handle) {
        (void)handle;
        hal_ret_t ret = stm32_dcmi_start_capture(s_cam_ctx.frame_buf, s_cam_ctx.frame_size);
        if (ret == MAIX_HAL_OK) s_cam_ctx.started = true;
        return ret;
    }
    static hal_ret_t cam_stop_adapter(hal_camera_handle_t handle) {
        (void)handle;
        s_cam_ctx.started = false;
        return stm32_dcmi_stop_capture();
    }
    static bool cam_frame_ready_adapter(hal_camera_handle_t handle) {
        (void)handle;
        return stm32_dcmi_frame_ready();
    }
    static hal_ret_t cam_read_frame_adapter(hal_camera_handle_t handle,
                                             uint8_t* buffer, size_t size) {
        (void)handle;
        if (!buffer || !s_cam_ctx.frame_buf) return MAIX_HAL_INVALID_PARAM;
        size_t copy_size = (size < s_cam_ctx.frame_size) ? size : s_cam_ctx.frame_size;
        memcpy(buffer, s_cam_ctx.frame_buf, copy_size);
        stm32_dcmi_clear_frame_flag();
        return MAIX_HAL_OK;
    }
    static hal_ret_t cam_get_size_adapter(hal_camera_handle_t handle,
                                           uint16_t* width, uint16_t* height) {
        (void)handle;
        if (width)  *width  = s_cam_ctx.width;
        if (height) *height = s_cam_ctx.height;
        return MAIX_HAL_OK;
    }
    static const hal_camera_ops_t camera_ops = {
        .open        = cam_open_adapter,
        .close       = cam_close_adapter,
        .start       = cam_start_adapter,
        .stop        = cam_stop_adapter,
        .frame_ready = cam_frame_ready_adapter,
        .read_frame  = cam_read_frame_adapter,
        .get_size    = cam_get_size_adapter,
    };

    /* ---- Display (LCD ST7789) 适配器 ---- */
    typedef struct {
        uint16_t width;
        uint16_t height;
        bool     opened;
    } stm32_display_ctx_t;
    static stm32_display_ctx_t s_disp_ctx;

    static hal_ret_t disp_open_adapter(hal_display_handle_t* handle,
                                        const hal_display_config_t* config) {
        hal_ret_t ret = stm32_lcd_init(config->width, config->height);
        if (ret != MAIX_HAL_OK) return ret;
        s_disp_ctx.width  = config->width;
        s_disp_ctx.height = config->height;
        s_disp_ctx.opened = true;
        *handle = (hal_display_handle_t)&s_disp_ctx;
        return MAIX_HAL_OK;
    }
    static hal_ret_t disp_close_adapter(hal_display_handle_t handle) {
        (void)handle;
        s_disp_ctx.opened = false;
        return MAIX_HAL_OK;
    }
    static hal_ret_t disp_show_adapter(hal_display_handle_t handle,
                                        const uint8_t* data, size_t size) {
        (void)handle;
        (void)size;
        /* data 为 RGB565 帧缓冲 */
        return stm32_lcd_show_frame((const uint16_t*)data);
    }
    static hal_ret_t disp_fill_adapter(hal_display_handle_t handle,
                                        uint8_t r, uint8_t g, uint8_t b) {
        (void)handle;
        /* RGB888 → RGB565 */
        uint16_t color = ((uint16_t)(r >> 3) << 11) |
                         ((uint16_t)(g >> 2) << 5)  |
                         ((uint16_t)(b >> 3));
        return stm32_lcd_fill(color);
    }
    static hal_ret_t disp_set_backlight_adapter(hal_display_handle_t handle, uint8_t level) {
        (void)handle; (void)level;
        return MAIX_HAL_NOT_SUPPORTED;
    }
    static hal_ret_t disp_set_rotation_adapter(hal_display_handle_t handle, uint16_t rotation) {
        (void)handle; (void)rotation;
        return MAIX_HAL_NOT_SUPPORTED;
    }
    static hal_ret_t disp_get_size_adapter(hal_display_handle_t handle,
                                            uint16_t* width, uint16_t* height) {
        (void)handle;
        if (width)  *width  = s_disp_ctx.width;
        if (height) *height = s_disp_ctx.height;
        return MAIX_HAL_OK;
    }
    static const hal_display_ops_t display_ops = {
        .open          = disp_open_adapter,
        .close         = disp_close_adapter,
        .show          = disp_show_adapter,
        .fill          = disp_fill_adapter,
        .set_backlight = disp_set_backlight_adapter,
        .set_rotation  = disp_set_rotation_adapter,
        .get_size      = disp_get_size_adapter,
    };

    hal_gpio_register_ops(&gpio_ops);
    hal_spi_register_ops(&spi_ops);
    hal_i2c_register_ops(&i2c_ops);
    hal_uart_register_ops(&uart_ops);
    hal_pwm_register_ops(&pwm_ops);
    hal_adc_register_ops(&adc_ops);
    hal_camera_register_ops(&camera_ops);
    hal_display_register_ops(&display_ops);

    return MAIX_HAL_OK;
}

#endif /* STM32 platforms */
