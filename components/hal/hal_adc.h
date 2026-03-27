#ifndef __HAL_ADC_H__
#define __HAL_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief ADC硬件抽象层接口
 * @file hal_adc.h
 */

/* ADC分辨率 */
typedef enum {
    HAL_ADC_RES_8BIT  = 8,
    HAL_ADC_RES_10BIT = 10,
    HAL_ADC_RES_12BIT = 12
} hal_adc_resolution_t;

/* ADC配置 */
typedef struct {
    hal_adc_resolution_t resolution;
    bool scan_mode;
    bool continuous;
} hal_adc_config_t;

/* ADC设备句柄 */
typedef void* hal_adc_handle_t;

/**
 * @brief ADC接口结构体
 */
typedef struct {
    hal_ret_t (*init)(hal_adc_handle_t* handle, uint32_t adc_id, const hal_adc_config_t* config);
    hal_ret_t (*deinit)(hal_adc_handle_t handle);
    hal_ret_t (*read)(hal_adc_handle_t handle, uint32_t channel, uint16_t* value);
    hal_ret_t (*read_voltage)(hal_adc_handle_t handle, uint32_t channel, float vref, float* voltage);
    hal_ret_t (*start_dma)(hal_adc_handle_t handle, uint32_t* channels, size_t count, uint16_t* buffer);
    hal_ret_t (*stop_dma)(hal_adc_handle_t handle);
} hal_adc_ops_t;

/* HAL接口函数 */
hal_ret_t hal_adc_init(hal_adc_handle_t* handle, uint32_t adc_id, const hal_adc_config_t* config);
hal_ret_t hal_adc_deinit(hal_adc_handle_t handle);
hal_ret_t hal_adc_read(hal_adc_handle_t handle, uint32_t channel, uint16_t* value);
hal_ret_t hal_adc_read_voltage(hal_adc_handle_t handle, uint32_t channel, float vref, float* voltage);
hal_ret_t hal_adc_start_dma(hal_adc_handle_t handle, uint32_t* channels, size_t count, uint16_t* buffer);
hal_ret_t hal_adc_stop_dma(hal_adc_handle_t handle);

/* 平台特定操作注册 */
hal_ret_t hal_adc_register_ops(const hal_adc_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_ADC_H__ */
