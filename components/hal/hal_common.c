#include "hal_common.h"
#include "hal_gpio.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief HAL通用实现
 * @file hal_common.c
 * @author MaixPy-K210-STM32 Team
 */

// 全局变量
static hal_platform_t g_platform = HAL_PLATFORM_UNKNOWN;
static hal_memory_ops_t g_memory_ops = {0};
static bool g_hal_initialized = false;

// 平台检测
static hal_platform_t detect_platform(void) {
#ifdef CONFIG_PLATFORM_K210
    return HAL_PLATFORM_K210;
#elif defined(CONFIG_PLATFORM_STM32F407)
    return HAL_PLATFORM_STM32F407;
#elif defined(CONFIG_PLATFORM_STM32F767)
    return HAL_PLATFORM_STM32F767;
#elif defined(CONFIG_PLATFORM_STM32H743)
    return HAL_PLATFORM_STM32H743;
#elif defined(__linux__)
    return HAL_PLATFORM_LINUX;
#else
    return HAL_PLATFORM_UNKNOWN;
#endif
}

// HAL初始化
hal_ret_t hal_init(void) {
    if (g_hal_initialized) {
        return HAL_OK;
    }
    
    // 检测平台
    g_platform = detect_platform();
    if (g_platform == HAL_PLATFORM_UNKNOWN) {
        return HAL_NOT_SUPPORTED;
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
        case HAL_PLATFORM_K210:
            // K210特定初始化
            break;
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32特定初始化
            break;
        case HAL_PLATFORM_LINUX:
            // Linux模拟初始化
            break;
        default:
            return HAL_NOT_SUPPORTED;
    }
    
    g_hal_initialized = true;
    return HAL_OK;
}

// 获取平台信息
hal_platform_t hal_get_platform(void) {
    return g_platform;
}

const char* hal_get_platform_name(void) {
    switch (g_platform) {
        case HAL_PLATFORM_K210: return "K210";
        case HAL_PLATFORM_STM32F407: return "STM32F407";
        case HAL_PLATFORM_STM32F767: return "STM32F767";
        case HAL_PLATFORM_STM32H743: return "STM32H743";
        case HAL_PLATFORM_LINUX: return "Linux";
        default: return "Unknown";
    }
}

// 系统控制函数
void hal_system_reset(void) {
    switch (g_platform) {
        case HAL_PLATFORM_K210:
            // K210复位实现
            break;
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32复位实现
            // HAL_NVIC_SystemReset();
            break;
        case HAL_PLATFORM_LINUX:
            // Linux模拟复位
            break;
        default:
            break;
    }
}

void hal_system_delay_ms(uint32_t ms) {
    switch (g_platform) {
        case HAL_PLATFORM_K210:
            // K210延时实现
            // msleep(ms);
            break;
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32延时实现
            // HAL_Delay(ms);
            break;
        case HAL_PLATFORM_LINUX:
            // Linux延时实现
            // usleep(ms * 1000);
            break;
        default:
            break;
    }
}

void hal_system_delay_us(uint32_t us) {
    switch (g_platform) {
        case HAL_PLATFORM_K210:
            // K210微秒延时
            // usleep(us);
            break;
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32微秒延时
            // 需要实现高精度延时
            break;
        case HAL_PLATFORM_LINUX:
            // Linux微秒延时
            // usleep(us);
            break;
        default:
            break;
    }
}

hal_time_t hal_system_get_time(void) {
    hal_time_t time = {0, 0};
    
    switch (g_platform) {
        case HAL_PLATFORM_K210:
            // K210时间获取
            break;
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32时间获取
            break;
        case HAL_PLATFORM_LINUX:
            // Linux时间获取
            // struct timeval tv;
            // gettimeofday(&tv, NULL);
            // time.seconds = tv.tv_sec;
            // time.microseconds = tv.tv_usec;
            break;
        default:
            break;
    }
    
    return time;
}

uint32_t hal_system_get_tick(void) {
    switch (g_platform) {
        case HAL_PLATFORM_K210:
            // K210系统滴答
            return 0; // 需要实现
        case HAL_PLATFORM_STM32F407:
        case HAL_PLATFORM_STM32F767:
        case HAL_PLATFORM_STM32H743:
            // STM32系统滴答
            // return HAL_GetTick();
            return 0;
        case HAL_PLATFORM_LINUX:
            // Linux系统滴答
            return 0; // 需要实现
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