#ifndef __HAL_COMMON_H__
#define __HAL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 硬件抽象层通用定义
 * @file hal_common.h
 * @author MaixPy-K210-STM32 Team
 */

// 平台定义
typedef enum {
    HAL_PLATFORM_UNKNOWN = 0,
    HAL_PLATFORM_K210,
    HAL_PLATFORM_STM32F407,
    HAL_PLATFORM_STM32F767,
    HAL_PLATFORM_STM32H743,
    HAL_PLATFORM_LINUX
} hal_platform_t;

// 错误码定义
typedef enum {
    MAIX_HAL_OK = 0,
    MAIX_HAL_ERROR = -1,
    MAIX_HAL_BUSY = -2,
    MAIX_HAL_TIMEOUT = -3,
    MAIX_HAL_INVALID_PARAM = -4,
    MAIX_HAL_NOT_SUPPORTED = -5,
    MAIX_HAL_NO_MEMORY = -6
} hal_ret_t;

// GPIO定义
typedef enum {
    HAL_GPIO_MODE_INPUT = 0,
    HAL_GPIO_MODE_OUTPUT,
    HAL_GPIO_MODE_AF,
    HAL_GPIO_MODE_ANALOG
} hal_gpio_mode_t;

typedef enum {
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN
} hal_gpio_pull_t;

typedef enum {
    HAL_GPIO_SPEED_LOW = 0,
    HAL_GPIO_SPEED_MEDIUM,
    HAL_GPIO_SPEED_HIGH,
    HAL_GPIO_SPEED_VERY_HIGH
} hal_gpio_speed_t;

typedef struct {
    uint32_t pin;
    hal_gpio_mode_t mode;
    hal_gpio_pull_t pull;
    hal_gpio_speed_t speed;
    uint32_t alternate;
} hal_gpio_config_t;

// SPI定义
typedef enum {
    HAL_SPI_MODE_MASTER = 0,
    HAL_SPI_MODE_SLAVE
} hal_spi_mode_t;

typedef enum {
    HAL_SPI_DATASIZE_8BIT = 0,
    HAL_SPI_DATASIZE_16BIT
} hal_spi_datasize_t;

typedef enum {
    HAL_SPI_CPOL_LOW = 0,
    HAL_SPI_CPOL_HIGH
} hal_spi_cpol_t;

typedef enum {
    HAL_SPI_CPHA_1EDGE = 0,
    HAL_SPI_CPHA_2EDGE
} hal_spi_cpha_t;

typedef struct {
    hal_spi_mode_t mode;
    uint32_t baudrate;
    hal_spi_datasize_t datasize;
    hal_spi_cpol_t cpol;
    hal_spi_cpha_t cpha;
} hal_spi_config_t;

// I2C定义
typedef enum {
    MAIX_HAL_I2C_MODE_MASTER = 0,
    MAIX_HAL_I2C_MODE_SLAVE
} hal_i2c_mode_t;

typedef struct {
    hal_i2c_mode_t mode;
    uint32_t clock_speed;
    uint32_t slave_address;
    bool address_10bit;
} hal_i2c_config_t;

// UART定义
typedef enum {
    HAL_UART_WORDLENGTH_8B = 0,
    HAL_UART_WORDLENGTH_9B
} hal_uart_wordlength_t;

typedef enum {
    HAL_UART_STOPBITS_1 = 0,
    HAL_UART_STOPBITS_2
} hal_uart_stopbits_t;

typedef enum {
    HAL_UART_PARITY_NONE = 0,
    HAL_UART_PARITY_EVEN,
    HAL_UART_PARITY_ODD
} hal_uart_parity_t;

typedef struct {
    uint32_t baudrate;
    hal_uart_wordlength_t wordlength;
    hal_uart_stopbits_t stopbits;
    hal_uart_parity_t parity;
} hal_uart_config_t;

// 时间定义
typedef struct {
    uint32_t seconds;
    uint32_t microseconds;
} hal_time_t;

// 内存管理
typedef struct {
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t size);
    void* (*calloc)(size_t num, size_t size);
} hal_memory_ops_t;

// 平台初始化
hal_ret_t hal_init(void);
hal_platform_t hal_get_platform(void);
const char* hal_get_platform_name(void);

// 系统控制
void hal_system_reset(void);
void hal_system_delay_ms(uint32_t ms);
void hal_system_delay_us(uint32_t us);
hal_time_t hal_system_get_time(void);
uint32_t hal_system_get_tick(void);

// 内存管理
void hal_memory_set_ops(const hal_memory_ops_t* ops);
void* hal_malloc(size_t size);
void hal_free(void* ptr);
void* hal_realloc(void* ptr, size_t size);
void* hal_calloc(size_t num, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_COMMON_H__ */
