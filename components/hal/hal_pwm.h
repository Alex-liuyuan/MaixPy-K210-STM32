#ifndef __HAL_PWM_H__
#define __HAL_PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_common.h"

/**
 * @brief PWM硬件抽象层接口
 * @file hal_pwm.h
 */

/* PWM配置 */
typedef struct {
    uint32_t prescaler;
    uint32_t period;
    uint32_t pulse;
    uint32_t polarity;   /* 0=高电平有效, 1=低电平有效 */
} hal_pwm_config_t;

/**
 * @brief PWM接口结构体
 */
typedef struct {
    hal_ret_t (*init)(uint32_t timer_id, uint32_t channel, const hal_pwm_config_t* config);
    hal_ret_t (*deinit)(uint32_t timer_id, uint32_t channel);
    hal_ret_t (*start)(uint32_t timer_id, uint32_t channel);
    hal_ret_t (*stop)(uint32_t timer_id, uint32_t channel);
    hal_ret_t (*set_duty)(uint32_t timer_id, uint32_t channel, uint32_t duty_permille);
    hal_ret_t (*get_duty)(uint32_t timer_id, uint32_t channel, uint32_t* duty_permille);
} hal_pwm_ops_t;

/* HAL接口函数 */
hal_ret_t hal_pwm_init(uint32_t timer_id, uint32_t channel, const hal_pwm_config_t* config);
hal_ret_t hal_pwm_deinit(uint32_t timer_id, uint32_t channel);
hal_ret_t hal_pwm_start(uint32_t timer_id, uint32_t channel);
hal_ret_t hal_pwm_stop(uint32_t timer_id, uint32_t channel);
hal_ret_t hal_pwm_set_duty(uint32_t timer_id, uint32_t channel, uint32_t duty_permille);
hal_ret_t hal_pwm_get_duty(uint32_t timer_id, uint32_t channel, uint32_t* duty_permille);

/* 平台特定操作注册 */
hal_ret_t hal_pwm_register_ops(const hal_pwm_ops_t* ops);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_PWM_H__ */
