"""
MaixPy Nano RT-Thread 引脚复用管理模块
"""

from .err import ERR_NONE, ERR_INVAL, ERR_NODEV

_pin_map: dict = {}

# STM32F407 引脚能力表
_PIN_CAPS = {
    "PA0":  ["GPIO", "ADC1_CH0", "TIM2_CH1", "UART4_TX"],
    "PA1":  ["GPIO", "ADC1_CH1", "TIM2_CH2", "UART4_RX"],
    "PA2":  ["GPIO", "ADC1_CH2", "USART2_TX"],
    "PA3":  ["GPIO", "ADC1_CH3", "USART2_RX"],
    "PA5":  ["GPIO", "ADC1_CH5", "SPI1_SCK"],
    "PA6":  ["GPIO", "ADC1_CH6", "SPI1_MISO", "TIM3_CH1"],
    "PA7":  ["GPIO", "ADC1_CH7", "SPI1_MOSI", "TIM3_CH2"],
    "PA9":  ["GPIO", "USART1_TX", "TIM1_CH2"],
    "PA10": ["GPIO", "USART1_RX", "TIM1_CH3"],
    "PB6":  ["GPIO", "I2C1_SCL", "TIM4_CH1"],
    "PB7":  ["GPIO", "I2C1_SDA", "TIM4_CH2"],
}


def set_pin_function(pin: str, func: str) -> int:
    """设置引脚功能，返回错误码"""
    caps = _PIN_CAPS.get(pin)
    if caps is None:
        return ERR_NODEV
    if func not in caps:
        return ERR_INVAL
    _pin_map[pin] = func
    return ERR_NONE


def get_pin_function(pin: str) -> str:
    """获取引脚当前功能，未设置返回空字符串"""
    return _pin_map.get(pin, "")


def get_pins_info() -> dict:
    """获取所有已配置引脚的功能映射"""
    return dict(_pin_map)


def get_pin_capabilities(pin: str) -> list:
    """获取引脚支持的所有功能列表"""
    return list(_PIN_CAPS.get(pin, []))
