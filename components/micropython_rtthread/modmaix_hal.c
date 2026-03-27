/**
 * @file modmaix_hal.c
 * @brief MicroPython内建模块 _maix_hal — 桥接Python层到C HAL
 *
 * 提供 GPIO/SPI/I2C/UART/Camera/Display 顶层函数，
 * 以及 pwm/adc 子模块对象。
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"

#include "hal_common.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_pwm.h"
#include "hal_adc.h"
#include "hal_camera.h"
#include "hal_display.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* 辅助：handle ↔ mp_int 转换                                         */
/* ------------------------------------------------------------------ */
#define HANDLE_TO_INT(h)  ((mp_int_t)(uintptr_t)(h))
#define INT_TO_HANDLE(i)  ((void*)(uintptr_t)(i))

/* ------------------------------------------------------------------ */
/* GPIO                                                                */
/* ------------------------------------------------------------------ */
STATIC mp_obj_t mod_gpio_init(size_t n_args, const mp_obj_t *args) {
    uint32_t pin  = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t mode = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : HAL_GPIO_MODE_OUTPUT;
    uint32_t pull = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : HAL_GPIO_PULL_NONE;
    hal_gpio_config_t cfg = {
        .pin  = pin,
        .mode = (hal_gpio_mode_t)mode,
        .pull = (hal_gpio_pull_t)pull,
        .speed = HAL_GPIO_SPEED_HIGH,
        .alternate = 0,
    };
    return mp_obj_new_int(hal_gpio_init(pin, &cfg));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_gpio_init_obj, 1, 3, mod_gpio_init);

STATIC mp_obj_t mod_gpio_deinit(mp_obj_t pin_obj) {
    return mp_obj_new_int(hal_gpio_deinit((uint32_t)mp_obj_get_int(pin_obj)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_gpio_deinit_obj, mod_gpio_deinit);

STATIC mp_obj_t mod_gpio_write(mp_obj_t pin_obj, mp_obj_t state_obj) {
    return mp_obj_new_int(hal_gpio_write((uint32_t)mp_obj_get_int(pin_obj),
                                         mp_obj_is_true(state_obj)
                                             ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_gpio_write_obj, mod_gpio_write);

STATIC mp_obj_t mod_gpio_read(mp_obj_t pin_obj) {
    return mp_obj_new_int(hal_gpio_read((uint32_t)mp_obj_get_int(pin_obj)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_gpio_read_obj, mod_gpio_read);

STATIC mp_obj_t mod_gpio_toggle(mp_obj_t pin_obj) {
    return mp_obj_new_int(hal_gpio_toggle((uint32_t)mp_obj_get_int(pin_obj)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_gpio_toggle_obj, mod_gpio_toggle);

/* ------------------------------------------------------------------ */
/* SPI                                                                 */
/* ------------------------------------------------------------------ */
STATIC mp_obj_t mod_spi_init(size_t n_args, const mp_obj_t *args) {
    uint32_t spi_id   = (uint32_t)mp_obj_get_int(args[0]);
    hal_spi_config_t cfg = {
        .mode     = (n_args > 1) ? (hal_spi_mode_t)mp_obj_get_int(args[1]) : HAL_SPI_MODE_MASTER,
        .baudrate = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 1000000,
        .datasize = (n_args > 3) ? (hal_spi_datasize_t)mp_obj_get_int(args[3]) : HAL_SPI_DATASIZE_8BIT,
        .cpol     = (n_args > 4) ? (hal_spi_cpol_t)mp_obj_get_int(args[4]) : HAL_SPI_CPOL_LOW,
        .cpha     = (n_args > 5) ? (hal_spi_cpha_t)mp_obj_get_int(args[5]) : HAL_SPI_CPHA_1EDGE,
    };
    hal_spi_handle_t handle = NULL;
    hal_ret_t ret = hal_spi_init(&handle, spi_id, &cfg);
    if (ret != MAIX_HAL_OK) {
        mp_raise_OSError(-ret);
    }
    return mp_obj_new_int(HANDLE_TO_INT(handle));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_spi_init_obj, 1, 6, mod_spi_init);

STATIC mp_obj_t mod_spi_deinit(mp_obj_t handle_obj) {
    return mp_obj_new_int(hal_spi_deinit(INT_TO_HANDLE(mp_obj_get_int(handle_obj))));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_spi_deinit_obj, mod_spi_deinit);

STATIC mp_obj_t mod_spi_write(size_t n_args, const mp_obj_t *args) {
    hal_spi_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[1], &buf, MP_BUFFER_READ);
    uint32_t timeout = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 100;
    return mp_obj_new_int(hal_spi_transmit(handle, buf.buf, buf.len, timeout));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_spi_write_obj, 2, 3, mod_spi_write);

STATIC mp_obj_t mod_spi_read(size_t n_args, const mp_obj_t *args) {
    hal_spi_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    size_t size = (size_t)mp_obj_get_int(args[1]);
    uint32_t timeout = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 100;
    uint8_t* buf = m_new(uint8_t, size);
    hal_ret_t ret = hal_spi_receive(handle, buf, size, timeout);
    if (ret != MAIX_HAL_OK) {
        m_del(uint8_t, buf, size);
        mp_raise_OSError(-ret);
    }
    mp_obj_t result = mp_obj_new_bytes(buf, size);
    m_del(uint8_t, buf, size);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_spi_read_obj, 2, 3, mod_spi_read);

STATIC mp_obj_t mod_spi_transfer(size_t n_args, const mp_obj_t *args) {
    hal_spi_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    mp_buffer_info_t tx_buf;
    mp_get_buffer_raise(args[1], &tx_buf, MP_BUFFER_READ);
    uint32_t timeout = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 100;
    uint8_t* rx = m_new(uint8_t, tx_buf.len);
    hal_ret_t ret = hal_spi_transmit_receive(handle, tx_buf.buf, rx, tx_buf.len, timeout);
    if (ret != MAIX_HAL_OK) {
        m_del(uint8_t, rx, tx_buf.len);
        mp_raise_OSError(-ret);
    }
    mp_obj_t result = mp_obj_new_bytes(rx, tx_buf.len);
    m_del(uint8_t, rx, tx_buf.len);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_spi_transfer_obj, 2, 3, mod_spi_transfer);

/* ------------------------------------------------------------------ */
/* I2C                                                                 */
/* ------------------------------------------------------------------ */
STATIC mp_obj_t mod_i2c_init(size_t n_args, const mp_obj_t *args) {
    uint32_t i2c_id = (uint32_t)mp_obj_get_int(args[0]);
    hal_i2c_config_t cfg = {
        .mode          = MAIX_HAL_I2C_MODE_MASTER,
        .clock_speed   = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 100000,
        .slave_address = 0,
        .address_10bit = (n_args > 2) ? mp_obj_is_true(args[2]) : false,
    };
    hal_i2c_handle_t handle = NULL;
    hal_ret_t ret = hal_i2c_init(&handle, i2c_id, &cfg);
    if (ret != MAIX_HAL_OK) mp_raise_OSError(-ret);
    return mp_obj_new_int(HANDLE_TO_INT(handle));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_i2c_init_obj, 1, 3, mod_i2c_init);

STATIC mp_obj_t mod_i2c_deinit(mp_obj_t h) {
    return mp_obj_new_int(hal_i2c_deinit(INT_TO_HANDLE(mp_obj_get_int(h))));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_i2c_deinit_obj, mod_i2c_deinit);

STATIC mp_obj_t mod_i2c_write(size_t n_args, const mp_obj_t *args) {
    hal_i2c_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    uint16_t addr = (uint16_t)mp_obj_get_int(args[1]);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[2], &buf, MP_BUFFER_READ);
    uint32_t timeout = (n_args > 3) ? (uint32_t)mp_obj_get_int(args[3]) : 100;
    return mp_obj_new_int(hal_i2c_master_transmit(handle, addr, buf.buf, buf.len, timeout));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_i2c_write_obj, 3, 4, mod_i2c_write);

STATIC mp_obj_t mod_i2c_read(size_t n_args, const mp_obj_t *args) {
    hal_i2c_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    uint16_t addr = (uint16_t)mp_obj_get_int(args[1]);
    size_t size = (size_t)mp_obj_get_int(args[2]);
    uint32_t timeout = (n_args > 3) ? (uint32_t)mp_obj_get_int(args[3]) : 100;
    uint8_t* buf = m_new(uint8_t, size);
    hal_ret_t ret = hal_i2c_master_receive(handle, addr, buf, size, timeout);
    if (ret != MAIX_HAL_OK) { m_del(uint8_t, buf, size); mp_raise_OSError(-ret); }
    mp_obj_t result = mp_obj_new_bytes(buf, size);
    m_del(uint8_t, buf, size);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_i2c_read_obj, 3, 4, mod_i2c_read);

STATIC mp_obj_t mod_i2c_mem_write(size_t n_args, const mp_obj_t *args) {
    hal_i2c_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    uint16_t dev_addr = (uint16_t)mp_obj_get_int(args[1]);
    uint16_t mem_addr = (uint16_t)mp_obj_get_int(args[2]);
    bool mem_16bit = mp_obj_is_true(args[3]);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[4], &buf, MP_BUFFER_READ);
    uint32_t timeout = (n_args > 5) ? (uint32_t)mp_obj_get_int(args[5]) : 100;
    hal_i2c_memaddr_size_t addr_size = mem_16bit
        ? HAL_I2C_MEMADD_SIZE_16BIT : HAL_I2C_MEMADD_SIZE_8BIT;
    return mp_obj_new_int(hal_i2c_mem_write(handle, dev_addr, mem_addr, addr_size,
                                             buf.buf, buf.len, timeout));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_i2c_mem_write_obj, 5, 6, mod_i2c_mem_write);

STATIC mp_obj_t mod_i2c_mem_read(size_t n_args, const mp_obj_t *args) {
    hal_i2c_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    uint16_t dev_addr = (uint16_t)mp_obj_get_int(args[1]);
    uint16_t mem_addr = (uint16_t)mp_obj_get_int(args[2]);
    bool mem_16bit = mp_obj_is_true(args[3]);
    size_t size = (size_t)mp_obj_get_int(args[4]);
    uint32_t timeout = (n_args > 5) ? (uint32_t)mp_obj_get_int(args[5]) : 100;
    hal_i2c_memaddr_size_t addr_size = mem_16bit
        ? HAL_I2C_MEMADD_SIZE_16BIT : HAL_I2C_MEMADD_SIZE_8BIT;
    uint8_t* buf = m_new(uint8_t, size);
    hal_ret_t ret = hal_i2c_mem_read(handle, dev_addr, mem_addr, addr_size,
                                      buf, size, timeout);
    if (ret != MAIX_HAL_OK) { m_del(uint8_t, buf, size); mp_raise_OSError(-ret); }
    mp_obj_t result = mp_obj_new_bytes(buf, size);
    m_del(uint8_t, buf, size);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_i2c_mem_read_obj, 5, 6, mod_i2c_mem_read);

/* ------------------------------------------------------------------ */
/* UART                                                                */
/* ------------------------------------------------------------------ */
STATIC mp_obj_t mod_uart_init(size_t n_args, const mp_obj_t *args) {
    uint32_t uart_id = (uint32_t)mp_obj_get_int(args[0]);
    hal_uart_config_t cfg = {
        .baudrate   = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 115200,
        .wordlength = (n_args > 2) ? (hal_uart_wordlength_t)mp_obj_get_int(args[2]) : HAL_UART_WORDLENGTH_8B,
        .stopbits   = (n_args > 3) ? (hal_uart_stopbits_t)mp_obj_get_int(args[3]) : HAL_UART_STOPBITS_1,
        .parity     = (n_args > 4) ? (hal_uart_parity_t)mp_obj_get_int(args[4]) : HAL_UART_PARITY_NONE,
    };
    hal_uart_handle_t handle = NULL;
    hal_ret_t ret = hal_uart_init(&handle, uart_id, &cfg);
    if (ret != MAIX_HAL_OK) mp_raise_OSError(-ret);
    return mp_obj_new_int(HANDLE_TO_INT(handle));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_uart_init_obj, 1, 5, mod_uart_init);

STATIC mp_obj_t mod_uart_deinit(mp_obj_t h) {
    return mp_obj_new_int(hal_uart_deinit(INT_TO_HANDLE(mp_obj_get_int(h))));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_uart_deinit_obj, mod_uart_deinit);

STATIC mp_obj_t mod_uart_write(size_t n_args, const mp_obj_t *args) {
    hal_uart_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[1], &buf, MP_BUFFER_READ);
    uint32_t timeout = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 1000;
    return mp_obj_new_int(hal_uart_transmit(handle, buf.buf, buf.len, timeout));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_uart_write_obj, 2, 3, mod_uart_write);

STATIC mp_obj_t mod_uart_read(size_t n_args, const mp_obj_t *args) {
    hal_uart_handle_t handle = INT_TO_HANDLE(mp_obj_get_int(args[0]));
    size_t size = (size_t)mp_obj_get_int(args[1]);
    uint32_t timeout = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 1000;
    uint8_t* buf = m_new(uint8_t, size);
    hal_ret_t ret = hal_uart_receive(handle, buf, size, timeout);
    if (ret != MAIX_HAL_OK) { m_del(uint8_t, buf, size); mp_raise_OSError(-ret); }
    mp_obj_t result = mp_obj_new_bytes(buf, size);
    m_del(uint8_t, buf, size);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_uart_read_obj, 2, 3, mod_uart_read);

/* ------------------------------------------------------------------ */
/* Camera                                                              */
/* ------------------------------------------------------------------ */
static hal_camera_handle_t s_mpy_cam_handle = NULL;

STATIC mp_obj_t mod_camera_open(size_t n_args, const mp_obj_t *args) {
    hal_camera_config_t cfg = {
        .width  = (n_args > 0) ? (uint16_t)mp_obj_get_int(args[0]) : 320,
        .height = (n_args > 1) ? (uint16_t)mp_obj_get_int(args[1]) : 240,
        .format = HAL_CAMERA_FMT_RGB565,
    };
    if (n_args > 2) {
        /* format 参数（字符串或整数） */
        if (mp_obj_is_int(args[2])) {
            cfg.format = (hal_camera_format_t)mp_obj_get_int(args[2]);
        }
    }
    hal_ret_t ret = hal_camera_open(&s_mpy_cam_handle, &cfg);
    if (ret != MAIX_HAL_OK) return mp_obj_new_int(ret);
    ret = hal_camera_start(s_mpy_cam_handle);
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_camera_open_obj, 0, 3, mod_camera_open);

STATIC mp_obj_t mod_camera_close(void) {
    if (s_mpy_cam_handle) {
        hal_camera_stop(s_mpy_cam_handle);
        hal_camera_close(s_mpy_cam_handle);
        s_mpy_cam_handle = NULL;
    }
    return mp_obj_new_int(MAIX_HAL_OK);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_camera_close_obj, mod_camera_close);

STATIC mp_obj_t mod_camera_frame_ready(void) {
    if (!s_mpy_cam_handle) return mp_const_false;
    return mp_obj_new_bool(hal_camera_frame_ready(s_mpy_cam_handle));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_camera_frame_ready_obj, mod_camera_frame_ready);

STATIC mp_obj_t mod_camera_read_frame(void) {
    if (!s_mpy_cam_handle) return mp_const_none;
    uint16_t w = 0, h = 0;
    hal_camera_get_size(s_mpy_cam_handle, &w, &h);
    size_t size = (size_t)w * h * 2; /* RGB565 */
    uint8_t* buf = m_new(uint8_t, size);
    hal_ret_t ret = hal_camera_read_frame(s_mpy_cam_handle, buf, size);
    if (ret != MAIX_HAL_OK) { m_del(uint8_t, buf, size); return mp_const_none; }
    mp_obj_t result = mp_obj_new_bytes(buf, size);
    m_del(uint8_t, buf, size);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_camera_read_frame_obj, mod_camera_read_frame);

/* ------------------------------------------------------------------ */
/* Display                                                             */
/* ------------------------------------------------------------------ */
static hal_display_handle_t s_mpy_disp_handle = NULL;

STATIC mp_obj_t mod_display_open(size_t n_args, const mp_obj_t *args) {
    hal_display_config_t cfg = {
        .width  = (n_args > 0) ? (uint16_t)mp_obj_get_int(args[0]) : 240,
        .height = (n_args > 1) ? (uint16_t)mp_obj_get_int(args[1]) : 240,
        .format = HAL_DISPLAY_FMT_RGB565,
    };
    return mp_obj_new_int(hal_display_open(&s_mpy_disp_handle, &cfg));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_display_open_obj, 0, 2, mod_display_open);

STATIC mp_obj_t mod_display_close(void) {
    if (s_mpy_disp_handle) {
        hal_display_close(s_mpy_disp_handle);
        s_mpy_disp_handle = NULL;
    }
    return mp_obj_new_int(MAIX_HAL_OK);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_display_close_obj, mod_display_close);

STATIC mp_obj_t mod_display_show(mp_obj_t data_obj) {
    if (!s_mpy_disp_handle) return mp_obj_new_int(MAIX_HAL_ERROR);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(data_obj, &buf, MP_BUFFER_READ);
    return mp_obj_new_int(hal_display_show(s_mpy_disp_handle, buf.buf, buf.len));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_display_show_obj, mod_display_show);

STATIC mp_obj_t mod_display_fill(size_t n_args, const mp_obj_t *args) {
    if (!s_mpy_disp_handle) return mp_obj_new_int(MAIX_HAL_ERROR);
    uint8_t r = (n_args > 0) ? (uint8_t)mp_obj_get_int(args[0]) : 0;
    uint8_t g = (n_args > 1) ? (uint8_t)mp_obj_get_int(args[1]) : 0;
    uint8_t b = (n_args > 2) ? (uint8_t)mp_obj_get_int(args[2]) : 0;
    return mp_obj_new_int(hal_display_fill(s_mpy_disp_handle, r, g, b));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_display_fill_obj, 0, 3, mod_display_fill);

/* ================================================================== */
/* PWM 子模块 (_maix_hal.pwm)                                         */
/* ================================================================== */
STATIC mp_obj_t mod_pwm_init(size_t n_args, const mp_obj_t *args) {
    uint32_t timer_id = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t channel  = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    hal_pwm_config_t cfg = {
        .prescaler = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : 83,
        .period    = (n_args > 3) ? (uint32_t)mp_obj_get_int(args[3]) : 999,
        .pulse     = (n_args > 4) ? (uint32_t)mp_obj_get_int(args[4]) : 500,
        .polarity  = (n_args > 5) ? (uint32_t)mp_obj_get_int(args[5]) : 0,
    };
    return mp_obj_new_int(hal_pwm_init(timer_id, channel, &cfg));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_pwm_init_obj, 1, 6, mod_pwm_init);

STATIC mp_obj_t mod_pwm_deinit(size_t n_args, const mp_obj_t *args) {
    uint32_t timer_id = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t channel  = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    return mp_obj_new_int(hal_pwm_deinit(timer_id, channel));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_pwm_deinit_obj, 1, 2, mod_pwm_deinit);

STATIC mp_obj_t mod_pwm_start(size_t n_args, const mp_obj_t *args) {
    uint32_t timer_id = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t channel  = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    return mp_obj_new_int(hal_pwm_start(timer_id, channel));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_pwm_start_obj, 1, 2, mod_pwm_start);

STATIC mp_obj_t mod_pwm_stop(size_t n_args, const mp_obj_t *args) {
    uint32_t timer_id = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t channel  = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    return mp_obj_new_int(hal_pwm_stop(timer_id, channel));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_pwm_stop_obj, 1, 2, mod_pwm_stop);

STATIC mp_obj_t mod_pwm_set_duty(mp_obj_t timer_obj, mp_obj_t ch_obj, mp_obj_t duty_obj) {
    return mp_obj_new_int(hal_pwm_set_duty((uint32_t)mp_obj_get_int(timer_obj),
                                            (uint32_t)mp_obj_get_int(ch_obj),
                                            (uint32_t)mp_obj_get_int(duty_obj)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mod_pwm_set_duty_obj, mod_pwm_set_duty);

STATIC const mp_rom_map_elem_t pwm_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pwm) },
    { MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&mod_pwm_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&mod_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),    MP_ROM_PTR(&mod_pwm_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),     MP_ROM_PTR(&mod_pwm_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_duty), MP_ROM_PTR(&mod_pwm_set_duty_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pwm_module_globals, pwm_module_globals_table);

STATIC const mp_obj_module_t mp_module_maix_hal_pwm = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pwm_module_globals,
};

/* ================================================================== */
/* ADC 子模块 (_maix_hal.adc)                                         */
/* ================================================================== */
static hal_adc_handle_t s_adc_handles[3] = { NULL, NULL, NULL };

STATIC mp_obj_t mod_adc_init(size_t n_args, const mp_obj_t *args) {
    uint32_t adc_id = (n_args > 0) ? (uint32_t)mp_obj_get_int(args[0]) : 0;
    if (adc_id >= 3) mp_raise_ValueError(MP_ERROR_TEXT("adc_id 0-2"));
    hal_adc_config_t cfg = {
        .resolution = HAL_ADC_RES_12BIT,
        .scan_mode  = (n_args > 1) ? mp_obj_is_true(args[1]) : false,
        .continuous = (n_args > 2) ? mp_obj_is_true(args[2]) : false,
    };
    return mp_obj_new_int(hal_adc_init(&s_adc_handles[adc_id], adc_id, &cfg));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_adc_init_obj, 0, 3, mod_adc_init);

STATIC mp_obj_t mod_adc_deinit(size_t n_args, const mp_obj_t *args) {
    uint32_t adc_id = (n_args > 0) ? (uint32_t)mp_obj_get_int(args[0]) : 0;
    if (adc_id >= 3 || !s_adc_handles[adc_id]) return mp_obj_new_int(MAIX_HAL_INVALID_PARAM);
    hal_ret_t ret = hal_adc_deinit(s_adc_handles[adc_id]);
    s_adc_handles[adc_id] = NULL;
    return mp_obj_new_int(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_adc_deinit_obj, 0, 1, mod_adc_deinit);

STATIC mp_obj_t mod_adc_read(size_t n_args, const mp_obj_t *args) {
    uint32_t adc_id  = (n_args > 0) ? (uint32_t)mp_obj_get_int(args[0]) : 0;
    uint32_t channel = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    if (adc_id >= 3 || !s_adc_handles[adc_id]) mp_raise_OSError(MAIX_HAL_ERROR);
    uint16_t value = 0;
    hal_ret_t ret = hal_adc_read(s_adc_handles[adc_id], channel, &value);
    if (ret != MAIX_HAL_OK) mp_raise_OSError(-ret);
    return mp_obj_new_int(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_adc_read_obj, 0, 2, mod_adc_read);

STATIC mp_obj_t mod_adc_read_voltage(size_t n_args, const mp_obj_t *args) {
    uint32_t adc_id  = (n_args > 0) ? (uint32_t)mp_obj_get_int(args[0]) : 0;
    uint32_t channel = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : 0;
    float vref = (n_args > 2) ? (float)mp_obj_get_float(args[2]) : 3.3f;
    if (adc_id >= 3 || !s_adc_handles[adc_id]) mp_raise_OSError(MAIX_HAL_ERROR);
    float voltage = 0.0f;
    hal_ret_t ret = hal_adc_read_voltage(s_adc_handles[adc_id], channel, vref, &voltage);
    if (ret != MAIX_HAL_OK) mp_raise_OSError(-ret);
    return mp_obj_new_float(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_adc_read_voltage_obj, 0, 3, mod_adc_read_voltage);

STATIC const mp_rom_map_elem_t adc_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),      MP_ROM_QSTR(MP_QSTR_adc) },
    { MP_ROM_QSTR(MP_QSTR_init),          MP_ROM_PTR(&mod_adc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),        MP_ROM_PTR(&mod_adc_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),          MP_ROM_PTR(&mod_adc_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_voltage),  MP_ROM_PTR(&mod_adc_read_voltage_obj) },
};
STATIC MP_DEFINE_CONST_DICT(adc_module_globals, adc_module_globals_table);

STATIC const mp_obj_module_t mp_module_maix_hal_adc = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&adc_module_globals,
};

/* ================================================================== */
/* 顶层模块 _maix_hal                                                 */
/* ================================================================== */
STATIC const mp_rom_map_elem_t maix_hal_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),          MP_ROM_QSTR(MP_QSTR__maix_hal) },
    /* GPIO */
    { MP_ROM_QSTR(MP_QSTR_gpio_init),         MP_ROM_PTR(&mod_gpio_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_deinit),       MP_ROM_PTR(&mod_gpio_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_write),        MP_ROM_PTR(&mod_gpio_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_read),         MP_ROM_PTR(&mod_gpio_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_toggle),       MP_ROM_PTR(&mod_gpio_toggle_obj) },
    /* SPI */
    { MP_ROM_QSTR(MP_QSTR_spi_init),          MP_ROM_PTR(&mod_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_deinit),        MP_ROM_PTR(&mod_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_write),         MP_ROM_PTR(&mod_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_read),          MP_ROM_PTR(&mod_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_transfer),      MP_ROM_PTR(&mod_spi_transfer_obj) },
    /* I2C */
    { MP_ROM_QSTR(MP_QSTR_i2c_init),          MP_ROM_PTR(&mod_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_deinit),        MP_ROM_PTR(&mod_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_write),         MP_ROM_PTR(&mod_i2c_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_read),          MP_ROM_PTR(&mod_i2c_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_mem_write),     MP_ROM_PTR(&mod_i2c_mem_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_mem_read),      MP_ROM_PTR(&mod_i2c_mem_read_obj) },
    /* UART */
    { MP_ROM_QSTR(MP_QSTR_uart_init),         MP_ROM_PTR(&mod_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart_deinit),       MP_ROM_PTR(&mod_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart_write),        MP_ROM_PTR(&mod_uart_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart_read),         MP_ROM_PTR(&mod_uart_read_obj) },
    /* Camera */
    { MP_ROM_QSTR(MP_QSTR_camera_open),       MP_ROM_PTR(&mod_camera_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_close),      MP_ROM_PTR(&mod_camera_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_frame_ready),MP_ROM_PTR(&mod_camera_frame_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_read_frame), MP_ROM_PTR(&mod_camera_read_frame_obj) },
    /* Display */
    { MP_ROM_QSTR(MP_QSTR_display_open),      MP_ROM_PTR(&mod_display_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_close),     MP_ROM_PTR(&mod_display_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_show),      MP_ROM_PTR(&mod_display_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_fill),      MP_ROM_PTR(&mod_display_fill_obj) },
    /* 子模块 */
    { MP_ROM_QSTR(MP_QSTR_pwm),               MP_ROM_PTR(&mp_module_maix_hal_pwm) },
    { MP_ROM_QSTR(MP_QSTR_adc),               MP_ROM_PTR(&mp_module_maix_hal_adc) },
};
STATIC MP_DEFINE_CONST_DICT(maix_hal_module_globals, maix_hal_module_globals_table);

const mp_obj_module_t mp_module_maix_hal = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&maix_hal_module_globals,
};
