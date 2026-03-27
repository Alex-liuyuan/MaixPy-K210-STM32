/**
 * @file stm32_dcmi.c
 * @brief STM32 DCMI摄像头接口实现（DMA循环模式，帧完成标志轮询）
 */

#include "stm32_hal.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407)

static DCMI_HandleTypeDef  s_hdcmi;
static DMA_HandleTypeDef   s_hdma_dcmi;
static volatile bool       s_frame_ready = false;
static uint8_t*            s_frame_buf   = NULL;
static size_t              s_frame_size  = 0;

hal_ret_t stm32_dcmi_init(const stm32_dcmi_config_t* config) {
    if (!config) return MAIX_HAL_INVALID_PARAM;

    __HAL_RCC_DCMI_CLK_ENABLE();

    s_hdcmi.Instance              = DCMI;
    s_hdcmi.Init.SynchroMode      = (config->sync_mode == 1)
                                    ? DCMI_SYNCHRO_EMBEDDED : DCMI_SYNCHRO_HARDWARE;
    s_hdcmi.Init.PCKPolarity      = (config->pck_polarity == 1)
                                    ? DCMI_PCKPOLARITY_RISING : DCMI_PCKPOLARITY_FALLING;
    s_hdcmi.Init.VSPolarity       = (config->vsync_polarity == 1)
                                    ? DCMI_VSPOLARITY_HIGH : DCMI_VSPOLARITY_LOW;
    s_hdcmi.Init.HSPolarity       = (config->hsync_polarity == 1)
                                    ? DCMI_HSPOLARITY_HIGH : DCMI_HSPOLARITY_LOW;
    s_hdcmi.Init.CaptureRate      = DCMI_CR_ALL_FRAME;
    s_hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;

    if (HAL_DCMI_Init(&s_hdcmi) != HAL_OK) return MAIX_HAL_ERROR;

    /* DMA2 Stream1 Channel1 → DCMI */
    __HAL_RCC_DMA2_CLK_ENABLE();
    s_hdma_dcmi.Instance                 = DMA2_Stream1;
    s_hdma_dcmi.Init.Channel             = DMA_CHANNEL_1;
    s_hdma_dcmi.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    s_hdma_dcmi.Init.PeriphInc           = DMA_PINC_DISABLE;
    s_hdma_dcmi.Init.MemInc              = DMA_MINC_ENABLE;
    s_hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    s_hdma_dcmi.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    s_hdma_dcmi.Init.Mode                = DMA_CIRCULAR;
    s_hdma_dcmi.Init.Priority            = DMA_PRIORITY_HIGH;
    s_hdma_dcmi.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    s_hdma_dcmi.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    s_hdma_dcmi.Init.MemBurst            = DMA_MBURST_INC4;
    s_hdma_dcmi.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(&s_hdma_dcmi) != HAL_OK) return MAIX_HAL_ERROR;

    __HAL_LINKDMA(&s_hdcmi, DMA_Handle, s_hdma_dcmi);

    HAL_NVIC_SetPriority(DCMI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

    return MAIX_HAL_OK;
}

hal_ret_t stm32_dcmi_deinit(void) {
    HAL_DCMI_DeInit(&s_hdcmi);
    HAL_DMA_DeInit(&s_hdma_dcmi);
    return MAIX_HAL_OK;
}

hal_ret_t stm32_dcmi_start_capture(uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) return MAIX_HAL_INVALID_PARAM;
    s_frame_buf   = buffer;
    s_frame_size  = size;
    s_frame_ready = false;
    /* 连续捕获模式 */
    HAL_StatusTypeDef r = HAL_DCMI_Start_DMA(&s_hdcmi, DCMI_MODE_CONTINUOUS,
                                             (uint32_t)buffer, (uint32_t)(size / 4));
    return (r == HAL_OK) ? MAIX_HAL_OK : MAIX_HAL_ERROR;
}

hal_ret_t stm32_dcmi_stop_capture(void) {
    HAL_DCMI_Stop(&s_hdcmi);
    return MAIX_HAL_OK;
}

hal_ret_t stm32_dcmi_suspend_capture(void) {
    HAL_DCMI_Suspend(&s_hdcmi);
    return MAIX_HAL_OK;
}

hal_ret_t stm32_dcmi_resume_capture(void) {
    HAL_DCMI_Resume(&s_hdcmi);
    return MAIX_HAL_OK;
}

/** @brief 轮询是否有新帧（Python侧调用） */
bool stm32_dcmi_frame_ready(void) {
    return s_frame_ready;
}

void stm32_dcmi_clear_frame_flag(void) {
    s_frame_ready = false;
}

/* HAL帧完成回调 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef* hdcmi) {
    (void)hdcmi;
    s_frame_ready = true;
}

/* IRQ处理 */
void DCMI_IRQHandler(void)         { HAL_DCMI_IRQHandler(&s_hdcmi); }
void DMA2_Stream1_IRQHandler(void) { HAL_DMA_IRQHandler(&s_hdma_dcmi); }

#endif /* STM32 platforms */
