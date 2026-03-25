"""
maix.pinmap 模块测试
"""
import pytest
import os
os.environ["MAIX_PLATFORM"] = "linux"

from maix import pinmap
from maix.err import ERR_NONE, ERR_INVAL, ERR_NODEV


def setup_function():
    """每个测试前清空 pinmap 状态"""
    pinmap._pin_map.clear()


def test_set_valid_function():
    ret = pinmap.set_pin_function("PA9", "USART1_TX")
    assert ret == ERR_NONE
    assert pinmap.get_pin_function("PA9") == "USART1_TX"


def test_set_invalid_function():
    ret = pinmap.set_pin_function("PA9", "SPI1_SCK")  # PA9 不支持 SPI1_SCK
    assert ret == ERR_INVAL


def test_set_unknown_pin():
    ret = pinmap.set_pin_function("PZ99", "GPIO")
    assert ret == ERR_NODEV


def test_get_unset_pin():
    assert pinmap.get_pin_function("PA0") == ""


def test_get_pins_info():
    pinmap.set_pin_function("PA9", "USART1_TX")
    pinmap.set_pin_function("PB6", "I2C1_SCL")
    info = pinmap.get_pins_info()
    assert info["PA9"] == "USART1_TX"
    assert info["PB6"] == "I2C1_SCL"


def test_get_pin_capabilities():
    caps = pinmap.get_pin_capabilities("PA9")
    assert "GPIO" in caps
    assert "USART1_TX" in caps


def test_get_capabilities_unknown_pin():
    caps = pinmap.get_pin_capabilities("PZ99")
    assert caps == []
def test_overwrite_function():
    pinmap.set_pin_function("PA0", "GPIO")
    pinmap.set_pin_function("PA0", "ADC1_CH0")
    assert pinmap.get_pin_function("PA0") == "ADC1_CH0"
