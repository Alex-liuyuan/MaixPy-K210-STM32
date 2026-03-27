#ifndef __HAL_AUDIO_H__
#define __HAL_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief Audio硬件抽象层接口
 * @file hal_audio.h
 */

/* 音频格式 */
typedef enum {
    HAL_AUDIO_FMT_PCM_S16 = 0,  /* 16-bit signed PCM */
    HAL_AUDIO_FMT_PCM_S32,
    HAL_AUDIO_FMT_PDM
} hal_audio_format_t;

/* 音频配置 */
typedef struct {
    uint32_t sample_rate;        /* 8000/16000/44100/48000 */
    uint8_t  channels;           /* 1=mono, 2=stereo */
    hal_audio_format_t format;
    uint16_t frame_size;         /* 每帧采样数（如 512） */
} hal_audio_config_t;

/* 音频设备句柄 */
typedef void* hal_audio_handle_t;

/**
 * @brief Audio接口结构体
 */
typedef struct {
    hal_ret_t (*open)(hal_audio_handle_t* handle, const hal_audio_config_t* config);
    hal_ret_t (*close)(hal_audio_handle_t handle);
    hal_ret_t (*start)(hal_audio_handle_t handle);
    hal_ret_t (*stop)(hal_audio_handle_t handle);
    hal_ret_t (*read)(hal_audio_handle_t handle, int16_t* buffer, size_t samples);
    hal_ret_t (*write)(hal_audio_handle_t handle, const int16_t* buffer, size_t samples);
    hal_ret_t (*set_volume)(hal_audio_handle_t handle, uint8_t volume);
    hal_ret_t (*get_volume)(hal_audio_handle_t handle, uint8_t* volume);
} hal_audio_ops_t;

/* HAL接口函数 */
hal_ret_t hal_audio_open(hal_audio_handle_t* handle, const hal_audio_config_t* config);
hal_ret_t hal_audio_close(hal_audio_handle_t handle);
hal_ret_t hal_audio_start(hal_audio_handle_t handle);
hal_ret_t hal_audio_stop(hal_audio_handle_t handle);
hal_ret_t hal_audio_read(hal_audio_handle_t handle, int16_t* buffer, size_t samples);
hal_ret_t hal_audio_write(hal_audio_handle_t handle, const int16_t* buffer, size_t samples);
hal_ret_t hal_audio_set_volume(hal_audio_handle_t handle, uint8_t volume);
hal_ret_t hal_audio_get_volume(hal_audio_handle_t handle, uint8_t* volume);

/* 平台特定操作注册 */
hal_ret_t hal_audio_register_ops(const hal_audio_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_AUDIO_H__ */
