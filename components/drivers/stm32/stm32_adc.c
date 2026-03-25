/**
 * @file stm32_adc.c
 * @brief STM32 ADC实现（单次转换 + DMA多通道扫描）
 */

#include "stm32_hal.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407) || defined(CONFIG_PLATFORM_STM32F767) || defined(CONFIG_PLATFORM_STM32H743)

#define ADC_INSTANCE_MAX 3

typedef struct {
    ADC_HandleTypeDef hadc;
    bool              initialized;
    uint16_t          dma_buf[16];   /* 最多16通道DMA缓冲 */
    volatile bool     dma_done;
} adc_ctx_t;

static adc_ctx_t s_adc[ADC_INSTANCE_MAX];

static ADC_TypeDef* adc_instance(uint32_t id) {
    switch (id) {
        case 0: return ADC1;
        case 1: return ADC2;
        case 2: return ADC3;
        default: return NULL;
    }
}

static void adc_clk_enable(uint32_t id) {
    switch (id) {
        case 0: __HAL_RCC_ADC1_CLK_ENABLE(); break;
        case 1: __HAL_RCC_ADC2_CLK_ENABLE(); break;
        case 2: __HAL_RCC_ADC3_CLK_ENABLE(); break;
        default: break;
    }
}

hal_ret_t stm32_adc_init(uint32_t adc_id, const stm32_adc_config_t* config) {
    if (!config || adc_id >= ADC_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;

    ADC_TypeDef* inst = adc_instance(adc_id);
    if (!inst) return MAIX_HAL_INVALID_PARAM;

    adc_clk_enable(adc_id);

    adc_ctx_t* ctx = &s_adc[adc_id];
    memset(&ctx->hadc, 0, sizeof(ctx->hadc));

    ctx->hadc.Instance                   = inst;
    ctx->hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    ctx->hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    ctx->hadc.Init.ScanConvMode          = config->scan_mode
                                           ? ENABLE : DISABLE;
    ctx->hadc.Init.ContinuousConvMode    = config->continuous_mode
                                           ? ENABLE : DISABLE;
    ctx->hadc.Init.DiscontinuousConvMode = DISABLE;
    ctx->hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    ctx->hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    ctx->hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    ctx->hadc.Init.NbrOfConversion       = 1;
    ctx->hadc.Init.DMAContinuousRequests = DISABLE;
    ctx->hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&ctx->hadc) != HAL_OK) return MAIX_HAL_ERROR;

    ctx->initialized = true;
    ctx->dma_done    = false;
    return MAIX_HAL_OK;
}

hal_ret_t stm32_adc_deinit(uint32_t adc_id) {
    if (adc_id >= ADC_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;
    adc_ctx_t* ctx = &s_adc[adc_id];
    HAL_ADC_DeInit(&ctx->hadc);
    ctx->initialized = false;
    return MAIX_HAL_OK;
}

/**
 * @brief 单通道单次读取
 * @param channel  ADC_CHANNEL_0 .. ADC_CHANNEL_18
 * @param value    输出12bit原始值（0-4095）
 */
hal_ret_t stm32_adc_read_channel(uint32_t adc_id, uint32_t channel,
                                  uint16_t* value) {
    if (adc_id >= ADC_INSTANCE_MAX || !value) return MAIX_HAL_INVALID_PARAM;
    adc_ctx_t* ctx = &s_adc[adc_id];
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = channel;
    cfg.Rank         = 1;
    cfg.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    if (HAL_ADC_ConfigChannel(&ctx->hadc, &cfg) != HAL_OK) return MAIX_HAL_ERROR;

    HAL_ADC_Start(&ctx->hadc);
    if (HAL_ADC_PollForConversion(&ctx->hadc, 10) != HAL_OK) {
        HAL_ADC_Stop(&ctx->hadc);
        return MAIX_HAL_TIMEOUT;
    }
    *value = (uint16_t)HAL_ADC_GetValue(&ctx->hadc);
    HAL_ADC_Stop(&ctx->hadc);
    return MAIX_HAL_OK;
}

/**
 * @brief 启动DMA多通道扫描（结果存入内部缓冲，通过stm32_adc_get_dma_result读取）
 * @param channels  通道数组（ADC_CHANNEL_x）
 * @param num       通道数量（最多16）
 */
hal_ret_t stm32_adc_start_scan_dma(uint32_t adc_id, const uint32_t* channels,
                                    uint32_t num) {
    if (adc_id >= ADC_INSTANCE_MAX || !channels || num == 0 || num > 16) {
        return MAIX_HAL_INVALID_PARAM;
    }
    adc_ctx_t* ctx = &s_adc[adc_id];
    if (!ctx->initialized) return MAIX_HAL_ERROR;

    /* 重新配置为扫描+DMA模式 */
    ctx->hadc.Init.ScanConvMode          = ENABLE;
    ctx->hadc.Init.ContinuousConvMode    = ENABLE;
    ctx->hadc.Init.NbrOfConversion       = (uint32_t)num;
    ctx->hadc.Init.DMAContinuousRequests = ENABLE;
    HAL_ADC_Init(&ctx->hadc);

    for (uint32_t i = 0; i < num; i++) {
        ADC_ChannelConfTypeDef cfg = {0};
        cfg.Channel      = channels[i];
        cfg.Rank         = i + 1;
        cfg.SamplingTime = ADC_SAMPLETIME_56CYCLES;
        HAL_ADC_ConfigChannel(&ctx->hadc, &cfg);
    }

    ctx->dma_done = false;
    HAL_StatusTypeDef r = HAL_ADC_Start_DMA(&ctx->hadc,
                                            (uint32_t*)ctx->dma_buf, num);
    return (r == HAL_OK) ? MAIX_HAL_OK : MAIX_HAL_ERROR;
}

hal_ret_t stm32_adc_stop_dma(uint32_t adc_id) {
    if (adc_id >= ADC_INSTANCE_MAX) return MAIX_HAL_INVALID_PARAM;
    HAL_ADC_Stop_DMA(&s_adc[adc_id].hadc);
    return MAIX_HAL_OK;
}

uint16_t stm32_adc_get_dma_result(uint32_t adc_id, uint32_t index) {
    if (adc_id >= ADC_INSTANCE_MAX || index >= 16) return 0;
    return s_adc[adc_id].dma_buf[index];
}

/* HAL ADC转换完成回调 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    for (uint32_t i = 0; i < ADC_INSTANCE_MAX; i++) {
        if (s_adc[i].hadc.Instance == hadc->Instance) {
            s_adc[i].dma_done = true;
            break;
        }
    }
}

#endif /* STM32 platforms */
