#ifndef __HAL_DISPLAY_H__
#define __HAL_DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief Display硬件抽象层接口
 * @file hal_display.h
 */

/* Display像素格式 */
typedef enum {
    HAL_DISPLAY_FMT_RGB565 = 0,
    HAL_DISPLAY_FMT_RGB888
} hal_display_format_t;

/* Display配置 */
typedef struct {
    uint16_t width;
    uint16_t height;
    hal_display_format_t format;
} hal_display_config_t;

/* Display设备句柄 */
typedef void* hal_display_handle_t;

/**
 * @brief Display接口结构体
 */
typedef struct {
    hal_ret_t (*open)(hal_display_handle_t* handle, const hal_display_config_t* config);
    hal_ret_t (*close)(hal_display_handle_t handle);
    hal_ret_t (*show)(hal_display_handle_t handle, const uint8_t* data, size_t size);
    hal_ret_t (*fill)(hal_display_handle_t handle, uint8_t r, uint8_t g, uint8_t b);
    hal_ret_t (*set_backlight)(hal_display_handle_t handle, uint8_t level);
    hal_ret_t (*set_rotation)(hal_display_handle_t handle, uint16_t rotation);
    hal_ret_t (*get_size)(hal_display_handle_t handle, uint16_t* width, uint16_t* height);
} hal_display_ops_t;

/* HAL接口函数 */
hal_ret_t hal_display_open(hal_display_handle_t* handle, const hal_display_config_t* config);
hal_ret_t hal_display_close(hal_display_handle_t handle);
hal_ret_t hal_display_show(hal_display_handle_t handle, const uint8_t* data, size_t size);
hal_ret_t hal_display_fill(hal_display_handle_t handle, uint8_t r, uint8_t g, uint8_t b);
hal_ret_t hal_display_set_backlight(hal_display_handle_t handle, uint8_t level);
hal_ret_t hal_display_set_rotation(hal_display_handle_t handle, uint16_t rotation);
hal_ret_t hal_display_get_size(hal_display_handle_t handle, uint16_t* width, uint16_t* height);

/* 平台特定操作注册 */
hal_ret_t hal_display_register_ops(const hal_display_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_DISPLAY_H__ */
