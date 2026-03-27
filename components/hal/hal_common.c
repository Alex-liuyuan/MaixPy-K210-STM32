#include "hal_common.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(CONFIG_PLATFORM_STM32F407)
#include "stm32_hal.h"
#elif defined(__linux__)
#include <sys/time.h>
#include <unistd.h>
#endif

/**
 * @brief HAL通用实现
 * @file hal_common.c
 * @author MaixPy Nano RT-Thread Team
 */

// 全局变量
static hal_platform_t g_platform = HAL_PLATFORM_UNKNOWN;
static hal_memory_ops_t g_memory_ops = {0};
static bool g_hal_initialized = false;

// 平台检测
static hal_platform_t detect_platform(void) {
#ifdef CONFIG_PLATFORM_STM32F407
    return HAL_PLATFORM_STM32F407;
#elif defined(CONFIG_PLATFORM_K210)
    return HAL_PLATFORM_K210;
#elif defined(__linux__)
    return HAL_PLATFORM_LINUX;
#else
    return HAL_PLATFORM_UNKNOWN;
#endif
}

// HAL初始化
hal_ret_t hal_init(void) {
    if (g_hal_initialized) {
        return MAIX_HAL_OK;
    }
    
    // 检测平台
    g_platform = detect_platform();
    if (g_platform == HAL_PLATFORM_UNKNOWN) {
        return MAIX_HAL_NOT_SUPPORTED;
    }
    
    // 设置默认内存操作
    if (g_memory_ops.malloc == NULL) {
        g_memory_ops.malloc = malloc;
        g_memory_ops.free = free;
        g_memory_ops.realloc = realloc;
        g_memory_ops.calloc = calloc;
    }
    
    // 平台特定初始化
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
            if (stm32_hal_init() != MAIX_HAL_OK) {
                return MAIX_HAL_ERROR;
            }
            break;
        case HAL_PLATFORM_K210:
            break;
        case HAL_PLATFORM_LINUX:
            break;
        default:
            return MAIX_HAL_NOT_SUPPORTED;
    }
    
    g_hal_initialized = true;
    return MAIX_HAL_OK;
}

// 获取平台信息
hal_platform_t hal_get_platform(void) {
    return g_platform;
}

const char* hal_get_platform_name(void) {
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407: return "STM32F407";
        case HAL_PLATFORM_LINUX: return "Linux";
        default: return "Unknown";
    }
}

// 系统控制函数
void hal_system_reset(void) {
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
#if defined(CONFIG_PLATFORM_STM32F407)
            HAL_NVIC_SystemReset();
#endif
            break;
        case HAL_PLATFORM_K210:
            break;
        case HAL_PLATFORM_LINUX:
            break;
        default:
            break;
    }
}

void hal_system_delay_ms(uint32_t ms) {
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
#if defined(CONFIG_PLATFORM_STM32F407)
            HAL_Delay(ms);
#endif
            break;
        case HAL_PLATFORM_K210:
            break;
        case HAL_PLATFORM_LINUX:
            usleep(ms * 1000);
            break;
        default:
            break;
    }
}

void hal_system_delay_us(uint32_t us) {
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
            if (us > 0) {
                HAL_Delay((us + 999) / 1000);
            }
            break;
        case HAL_PLATFORM_K210:
            break;
        case HAL_PLATFORM_LINUX:
            usleep(us);
            break;
        default:
            break;
    }
}

hal_time_t hal_system_get_time(void) {
    hal_time_t time = {0, 0};
    
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
            time.seconds = HAL_GetTick() / 1000U;
            time.microseconds = (HAL_GetTick() % 1000U) * 1000U;
            break;
        case HAL_PLATFORM_K210:
            break;
        case HAL_PLATFORM_LINUX:
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            time.seconds = (uint32_t)tv.tv_sec;
            time.microseconds = (uint32_t)tv.tv_usec;
            break;
        }
        default:
            break;
    }
    
    return time;
}

uint32_t hal_system_get_tick(void) {
    switch (g_platform) {
        case HAL_PLATFORM_STM32F407:
            return HAL_GetTick();
        case HAL_PLATFORM_K210:
            return 0;
        case HAL_PLATFORM_LINUX:
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (uint32_t)((tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL));
        }
        default:
            return 0;
    }
}

// 内存管理函数
void hal_memory_set_ops(const hal_memory_ops_t* ops) {
    if (ops) {
        g_memory_ops = *ops;
    }
}

void* hal_malloc(size_t size) {
    if (g_memory_ops.malloc) {
        return g_memory_ops.malloc(size);
    }
    return NULL;
}

void hal_free(void* ptr) {
    if (g_memory_ops.free) {
        g_memory_ops.free(ptr);
    }
}

void* hal_realloc(void* ptr, size_t size) {
    if (g_memory_ops.realloc) {
        return g_memory_ops.realloc(ptr, size);
    }
    return NULL;
}

void* hal_calloc(size_t num, size_t size) {
    if (g_memory_ops.calloc) {
        return g_memory_ops.calloc(num, size);
    }
    return NULL;
}
