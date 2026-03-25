/**
 * @file stm32_pwm.c
 * @brief STM32 PWM实现（TIM通用定时器）
 */

#include "stm32_hal.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407) || defined(CONFIG_PLATFORM_STM32F767) || defined(CONFIG_PLATFORM_STM32H743)

#define PWM_TIM_MAX 14

typedef struct {
    TIM_HandleTypeDef htim;
    bool              initialized;
} pwm_ctx_t;

static pwm_ctx_t s_pwm[PWM_TIM_MAX];

static TIM_TypeDef* tim_instance(uint32_t id) {
    switch (id) {
        case 0:  return TIM1;
        case 1:  return TIM2;
        case 2:  return TIM3;
        case 3:  return TIM4;
        case 4:  return TIM5;
        case 7:  return TIM8;
#ifdef TIM9
        case 8:  return TIM9;
#endif
#ifdef TIM10
        case 9:  return TIM10;
#endif
#ifdef TIM11
        case 10: return TIM11;
#endif
#ifdef TIM12
        case 11: return TIM12;
#endif
#ifdef TIM13
        case 12: return TIM13;
#endif
#ifdef TIM14
        case 13: return TIM14;
#endif
        default: return NULL;
    }
}

static void tim_clk_enable(uint32_t id) {
    switch (id) {
        case 0:  __HAL_RCC_TIM1_CLK_ENABLE();  break;
        case 1:  __HAL_RCC_TIM2_CLK_ENABLE();  break;
        case 2:  __HAL_RCC_TIM3_CLK_ENABLE();  break;
        case 3:  __HAL_RCC_TIM4_CLK_ENABLE();  break;
        case 4:  __HAL_RCC_TIM5_CLK_ENABLE();  break;
        case 7:  __HAL_RCC_TIM8_CLK_ENABLE();  break;
#ifdef __HAL_RCC_TIM9_CLK_ENABLE
        case 8:  __HAL_RCC_TIM9_CLK_ENABLE();  break;
#endif
#ifdef __HAL_RCC_TIM10_CLK_ENABLE
        case 9:  __HAL_RCC_TIM10_CLK_ENABLE(); break;
#endif
#ifdef __HAL_RCC_TIM11_CLK_ENABLE
        case 10: __HAL_RCC_TIM11_CLK_ENABLE(); break;
#endif
#ifdef __HAL_RCC_TIM12_CLK_ENABLE
        case 11: __HAL_RCC_TIM12_CLK_ENABLE(); break;
#endif
#ifdef __HAL_RCC_TIM13_CLK_ENABLE
        case 12: __HAL_RCC_TIM13_CLK_ENABLE(); break;
#endif
#ifdef __HAL_RCC_TIM14_CLK_ENABLE
        case 13: __HAL_RCC_TIM14_CLK_ENABLE(); break;
#endif
        default: break;
    }
}

/* channel_id: 0-3 → TIM_CHANNEL_1..4 */
static uint32_t hal_channel(uint32_t ch) {
    switch (ch) {
        case 0: return TIM_CHANNEL_1;
        case 1: return TIM_CHANNEL_2;
        case 2: return TIM_CHANNEL_3;
        case 3: return TIM_CHANNEL_4;
        default: return TIM_CHANNEL_1;
    }
}

hal_ret_t stm32_pwm_init(uint32_t timer_id, uint32_t channel,
                          const stm32_pwm_config_t* config) {
    if (!config || timer_id >= PWM_TIM_MAX) return MAIX_HAL_INVALID_PARAM;

    TIM_TypeDef* inst = tim_instance(timer_id);
    if (!inst) return MAIX_HAL_INVALID_PARAM;

    tim_clk_enable(timer_id);

    pwm_ctx_t* ctx = &s_pwm[timer_id];
    ctx->htim.Instance               = inst;
    ctx->htim.Init.Prescaler         = config->prescaler;
    ctx->htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    ctx->htim.Init.Period            = config->period;
    ctx->htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    ctx->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&ctx->htim) != HAL_OK) return MAIX_HAL_ERROR;

    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = config->pulse;
    oc.OCPolarity   = (config->polarity == 1) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&ctx->htim, &oc, hal_channel(channel)) != HAL_OK) {
        return MAIX_HAL_ERROR;
    }

    ctx->initialized = true;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_pwm_deinit(uint32_t timer_id, uint32_t channel) {
    if (timer_id >= PWM_TIM_MAX) return MAIX_HAL_INVALID_PARAM;
    pwm_ctx_t* ctx = &s_pwm[timer_id];
    HAL_TIM_PWM_Stop(&ctx->htim, hal_channel(channel));
    HAL_TIM_PWM_DeInit(&ctx->htim);
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_pwm_start(uint32_t timer_id, uint32_t channel) {
    if (timer_id >= PWM_TIM_MAX) return MAIX_HAL_INVALID_PARAM;
    HAL_StatusTypeDef r = HAL_TIM_PWM_Start(&s_pwm[timer_id].htim, hal_channel(channel));
    return (r == HAL_OK) ? MAIX_HAL_OK : MAIX_HAL_ERROR;
}

hal_ret_t stm32_pwm_stop(uint32_t timer_id, uint32_t channel) {
    if (timer_id >= PWM_TIM_MAX) return MAIX_HAL_INVALID_PARAM;
    HAL_TIM_PWM_Stop(&s_pwm[timer_id].htim, hal_channel(channel));
    return MAIX_HAL_OK;
}

/**
 * @brief 设置占空比（0-1000，对应0.0%-100.0%）
 */
hal_ret_t stm32_pwm_set_duty_cycle(uint32_t timer_id, uint32_t channel,
                                    uint32_t duty_cycle) {
    if (timer_id >= PWM_TIM_MAX) return MAIX_HAL_INVALID_PARAM;
    pwm_ctx_t* ctx = &s_pwm[timer_id];
    uint32_t period = ctx->htim.Init.Period;
    uint32_t pulse  = (period * duty_cycle) / 1000;
    __HAL_TIM_SET_COMPARE(&ctx->htim, hal_channel(channel), pulse);
    return MAIX_HAL_OK;
}

#endif /* STM32 platforms */
