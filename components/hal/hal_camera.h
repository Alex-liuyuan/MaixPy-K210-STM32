#ifndef __HAL_CAMERA_H__
#define __HAL_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief Camera硬件抽象层接口
 * @file hal_camera.h
 */

/* Camera像素格式 */
typedef enum {
    HAL_CAMERA_FMT_RGB565 = 0,
    HAL_CAMERA_FMT_RGB888,
    HAL_CAMERA_FMT_YUV422,
    HAL_CAMERA_FMT_GRAY
} hal_camera_format_t;

/* Camera配置 */
typedef struct {
    uint16_t width;
    uint16_t height;
    hal_camera_format_t format;
} hal_camera_config_t;

/* Camera设备句柄 */
typedef void* hal_camera_handle_t;

/* 帧就绪回调 */
typedef void (*hal_camera_frame_cb_t)(hal_camera_handle_t handle, void* user_data);

/**
 * @brief Camera接口结构体
 */
typedef struct {
    hal_ret_t (*open)(hal_camera_handle_t* handle, const hal_camera_config_t* config);
    hal_ret_t (*close)(hal_camera_handle_t handle);
    hal_ret_t (*start)(hal_camera_handle_t handle);
    hal_ret_t (*stop)(hal_camera_handle_t handle);
    bool      (*frame_ready)(hal_camera_handle_t handle);
    hal_ret_t (*read_frame)(hal_camera_handle_t handle, uint8_t* buffer, size_t size);
    hal_ret_t (*get_size)(hal_camera_handle_t handle, uint16_t* width, uint16_t* height);
} hal_camera_ops_t;

/* HAL接口函数 */
hal_ret_t hal_camera_open(hal_camera_handle_t* handle, const hal_camera_config_t* config);
hal_ret_t hal_camera_close(hal_camera_handle_t handle);
hal_ret_t hal_camera_start(hal_camera_handle_t handle);
hal_ret_t hal_camera_stop(hal_camera_handle_t handle);
bool      hal_camera_frame_ready(hal_camera_handle_t handle);
hal_ret_t hal_camera_read_frame(hal_camera_handle_t handle, uint8_t* buffer, size_t size);
hal_ret_t hal_camera_get_size(hal_camera_handle_t handle, uint16_t* width, uint16_t* height);

/* 平台特定操作注册 */
hal_ret_t hal_camera_register_ops(const hal_camera_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_CAMERA_H__ */
