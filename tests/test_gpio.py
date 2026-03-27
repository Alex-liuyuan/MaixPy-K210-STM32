"""
tests/test_gpio.py
GPIO功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_gpio_init_write_read(hal):
    pin = hal.gpio_pin(0, 5)  # PA5
    assert hal.gpio_init(pin, 1, 0) == 0
    assert hal.gpio_write(pin, 1) == 0
    assert hal.gpio_read(pin) == 1
    assert hal.gpio_write(pin, 0) == 0
    assert hal.gpio_read(pin) == 0


def test_gpio_toggle(hal):
    pin = hal.gpio_pin(1, 3)  # PB3
    hal.gpio_init(pin, 1, 0)
    hal.gpio_write(pin, 0)
    hal.gpio_toggle(pin)
    assert hal.gpio_read(pin) == 1
    hal.gpio_toggle(pin)
    assert hal.gpio_read(pin) == 0


def test_gpio_deinit(hal):
    pin = hal.gpio_pin(2, 13)  # PC13
    hal.gpio_init(pin, 1, 0)
    hal.gpio_write(pin, 1)
    hal.gpio_deinit(pin)
    # 释放后读取应返回默认值0
    assert hal.gpio_read(pin) == 0


def test_gpio_irq(hal):
    pin = hal.gpio_pin(0, 0)
    called = []
    def cb(p):
        called.append(p)
    assert hal.gpio_enable_irq(pin, 0, cb) == 0
    assert hal.gpio_disable_irq(pin) == 0


def test_gpio_class():
    """测试 sysu.GPIO 高层接口"""
    from sysu import GPIO
    led = GPIO(0x00000005, GPIO.MODE_OUTPUT)  # PA5
    led.on()
    assert led.value() == 1
    led.off()
    assert led.value() == 0
    led.toggle()
    assert led.value() == 1


def test_gpio_pin_encoding(hal):
    """验证引脚编码：port<<16 | pin_num"""
    assert hal.gpio_pin(0, 5)  == 0x00000005
    assert hal.gpio_pin(1, 13) == (1 << 16) | 13  # PB13
    assert hal.gpio_pin(2, 13) == (2 << 16) | 13
