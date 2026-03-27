/**
 * @file stm32_lcd.c
 * @brief ST7789 LCD驱动（SPI接口，DC引脚区分命令/数据，DMA批量传输）
 */

#include "stm32_hal.h"
#include <string.h>

#if defined(CONFIG_PLATFORM_STM32F407)

/* 用户需根据硬件连接修改以下引脚定义 */
#ifndef LCD_SPI_ID
#define LCD_SPI_ID    0          /* SPI1 */
#endif
#ifndef LCD_DC_PIN
#define LCD_DC_PIN    ((1u<<16)|(5u))   /* PA5: port=A(0), pin=5 */
#endif
#ifndef LCD_RST_PIN
#define LCD_RST_PIN   ((1u<<16)|(6u))   /* PA6 */
#endif
#ifndef LCD_CS_PIN
#define LCD_CS_PIN    ((1u<<16)|(4u))   /* PA4 */
#endif

/* ST7789 命令 */
#define ST7789_NOP      0x00
#define ST7789_SWRESET  0x01
#define ST7789_SLPOUT   0x11
#define ST7789_NORON    0x13
#define ST7789_INVON    0x21
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_COLMOD   0x3A
#define ST7789_MADCTL   0x36

static SPI_HandleTypeDef s_hspi_lcd;
static uint16_t s_lcd_w = 240;
static uint16_t s_lcd_h = 240;

/* DC引脚控制 */
static inline void lcd_dc_cmd(void)  {
    uint32_t port = (LCD_DC_PIN >> 16) & 0xFFFF;
    uint32_t pin  = LCD_DC_PIN & 0xFFFF;
    extern GPIO_TypeDef* gpio_port_map[];
    HAL_GPIO_WritePin(gpio_port_map[port], (1u << pin), GPIO_PIN_RESET);
}
static inline void lcd_dc_data(void) {
    uint32_t port = (LCD_DC_PIN >> 16) & 0xFFFF;
    uint32_t pin  = LCD_DC_PIN & 0xFFFF;
    extern GPIO_TypeDef* gpio_port_map[];
    HAL_GPIO_WritePin(gpio_port_map[port], (1u << pin), GPIO_PIN_SET);
}
static inline void lcd_cs_low(void)  {
    uint32_t port = (LCD_CS_PIN >> 16) & 0xFFFF;
    uint32_t pin  = LCD_CS_PIN & 0xFFFF;
    extern GPIO_TypeDef* gpio_port_map[];
    HAL_GPIO_WritePin(gpio_port_map[port], (1u << pin), GPIO_PIN_RESET);
}
static inline void lcd_cs_high(void) {
    uint32_t port = (LCD_CS_PIN >> 16) & 0xFFFF;
    uint32_t pin  = LCD_CS_PIN & 0xFFFF;
    extern GPIO_TypeDef* gpio_port_map[];
    HAL_GPIO_WritePin(gpio_port_map[port], (1u << pin), GPIO_PIN_SET);
}

static void lcd_write_cmd(uint8_t cmd) {
    lcd_cs_low();
    lcd_dc_cmd();
    HAL_SPI_Transmit(&s_hspi_lcd, &cmd, 1, 10);
    lcd_cs_high();
}

static void lcd_write_data(const uint8_t* data, uint16_t len) {
    lcd_cs_low();
    lcd_dc_data();
    HAL_SPI_Transmit(&s_hspi_lcd, (uint8_t*)data, len, 100);
    lcd_cs_high();
}

static void lcd_write_data_byte(uint8_t d) {
    lcd_write_data(&d, 1);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    lcd_write_cmd(ST7789_CASET);
    buf[0] = x0 >> 8; buf[1] = x0 & 0xFF;
    buf[2] = x1 >> 8; buf[3] = x1 & 0xFF;
    lcd_write_data(buf, 4);

    lcd_write_cmd(ST7789_RASET);
    buf[0] = y0 >> 8; buf[1] = y0 & 0xFF;
    buf[2] = y1 >> 8; buf[3] = y1 & 0xFF;
    lcd_write_data(buf, 4);

    lcd_write_cmd(ST7789_RAMWR);
}

hal_ret_t stm32_lcd_init(uint16_t width, uint16_t height) {
    s_lcd_w = width;
    s_lcd_h = height;

    /* SPI初始化（高速，CPOL=0，CPHA=0） */
    __HAL_RCC_SPI1_CLK_ENABLE();
    s_hspi_lcd.Instance               = SPI1;
    s_hspi_lcd.Init.Mode              = SPI_MODE_MASTER;
    s_hspi_lcd.Init.Direction         = SPI_DIRECTION_2LINES;
    s_hspi_lcd.Init.DataSize          = SPI_DATASIZE_8BIT;
    s_hspi_lcd.Init.CLKPolarity       = SPI_POLARITY_LOW;
    s_hspi_lcd.Init.CLKPhase          = SPI_PHASE_1EDGE;
    s_hspi_lcd.Init.NSS               = SPI_NSS_SOFT;
    s_hspi_lcd.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    s_hspi_lcd.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    s_hspi_lcd.Init.TIMode            = SPI_TIMODE_DISABLE;
    s_hspi_lcd.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&s_hspi_lcd) != HAL_OK) return MAIX_HAL_ERROR;

    /* 硬件复位 */
    extern GPIO_TypeDef* gpio_port_map[];
    uint32_t rst_port = (LCD_RST_PIN >> 16) & 0xFFFF;
    uint32_t rst_pin  = LCD_RST_PIN & 0xFFFF;
    HAL_GPIO_WritePin(gpio_port_map[rst_port], (1u << rst_pin), GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(gpio_port_map[rst_port], (1u << rst_pin), GPIO_PIN_SET);
    HAL_Delay(120);

    /* ST7789初始化序列 */
    lcd_write_cmd(ST7789_SWRESET); HAL_Delay(150);
    lcd_write_cmd(ST7789_SLPOUT);  HAL_Delay(10);
    lcd_write_cmd(ST7789_COLMOD);  lcd_write_data_byte(0x55); /* RGB565 */
    lcd_write_cmd(ST7789_MADCTL);  lcd_write_data_byte(0x00);
    lcd_write_cmd(ST7789_INVON);
    lcd_write_cmd(ST7789_NORON);
    lcd_write_cmd(ST7789_DISPON);

    return MAIX_HAL_OK;
}

/**
 * @brief 显示RGB565帧缓冲（DMA批量传输）
 * @param buf  RGB565数据，大小 = width*height*2
 */
hal_ret_t stm32_lcd_show_frame(const uint16_t* buf) {
    if (!buf) return MAIX_HAL_INVALID_PARAM;
    lcd_set_window(0, 0, s_lcd_w - 1, s_lcd_h - 1);
    lcd_cs_low();
    lcd_dc_data();
    /* DMA发送整帧 */
    HAL_SPI_Transmit_DMA(&s_hspi_lcd, (uint8_t*)buf,
                         (uint16_t)(s_lcd_w * s_lcd_h * 2));
    /* 等待DMA完成（简单轮询，可改为回调） */
    while (HAL_SPI_GetState(&s_hspi_lcd) != HAL_SPI_STATE_READY) {}
    lcd_cs_high();
    return MAIX_HAL_OK;
}

hal_ret_t stm32_lcd_fill(uint16_t color) {
    lcd_set_window(0, 0, s_lcd_w - 1, s_lcd_h - 1);
    lcd_cs_low();
    lcd_dc_data();
    uint32_t total = (uint32_t)s_lcd_w * s_lcd_h;
    uint8_t  hi = color >> 8, lo = color & 0xFF;
    for (uint32_t i = 0; i < total; i++) {
        HAL_SPI_Transmit(&s_hspi_lcd, &hi, 1, 1);
        HAL_SPI_Transmit(&s_hspi_lcd, &lo, 1, 1);
    }
    lcd_cs_high();
    return MAIX_HAL_OK;
}

#endif /* STM32 platforms */
