"""
tests/test_adc.py
ADC功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_adc_init_hal(hal):
    """测试HAL层ADC初始化"""
    assert hal.adc.init(0, False, False) == 0


def test_adc_read_hal(hal):
    """测试HAL层ADC读取"""
    hal.adc.init(0)
    val = hal.adc.read(0, 0)
    assert isinstance(val, int)
    assert 0 <= val < 4096


def test_adc_read_voltage_hal(hal):
    """测试HAL层ADC电压读取"""
    hal.adc.init(0)
    vol = hal.adc.read_voltage(0, 0, 3.3)
    assert isinstance(vol, float)
    assert 0.0 <= vol <= 3.3


def test_adc_dma_hal(hal):
    """测试HAL层ADC DMA扫描"""
    hal.adc.init(0)
    assert hal.adc.start_scan_dma(0, [0, 1, 2]) == 0
    for i in range(3):
        val = hal.adc.get_dma_result(0, i)
        assert isinstance(val, int)
        assert 0 <= val < 4096


def test_adc_deinit_hal(hal):
    """测试HAL层ADC释放"""
    hal.adc.init(0)
    assert hal.adc.deinit(0) == 0


# ---- 高层 sysu.adc.ADC 接口测试 ----

def test_adc_class_init():
    """测试ADC类初始化"""
    from sysu.adc import ADC, RES_BIT_12
    a = ADC(adc_id=1, resolution=RES_BIT_12, vref=3.3)
    assert a.adc_id == 0  # 1-based -> 0-based
    assert a.resolution == 12
    assert a.vref == 3.3
    assert a._max_val == 4095


def test_adc_class_resolution():
    """测试不同分辨率"""
    from sysu.adc import ADC, RES_BIT_8, RES_BIT_10, RES_BIT_12
    a8 = ADC(adc_id=1, resolution=RES_BIT_8)
    assert a8._max_val == 255
    a10 = ADC(adc_id=1, resolution=RES_BIT_10)
    assert a10._max_val == 1023
    a12 = ADC(adc_id=1, resolution=RES_BIT_12)
    assert a12._max_val == 4095


def test_adc_class_read():
    """测试ADC read()"""
    from sysu.adc import ADC, CH0, CH1
    a = ADC(adc_id=1)
    val0 = a.read(CH0)
    assert isinstance(val0, int)
    assert 0 <= val0 <= 4095
    val1 = a.read(CH1)
    assert isinstance(val1, int)


def test_adc_class_read_vol():
    """测试ADC read_vol()"""
    from sysu.adc import ADC, CH0
    a = ADC(adc_id=1, vref=3.3)
    vol = a.read_vol(CH0)
    assert isinstance(vol, float)
    assert 0.0 <= vol <= 3.3


def test_adc_class_read_voltage_alias():
    """测试 read_voltage() 是 read_vol() 的别名"""
    from sysu.adc import ADC, CH0
    a = ADC(adc_id=1, vref=3.3)
    assert a.read_voltage(CH0) == a.read_vol(CH0)


def test_adc_class_read_multi():
    """测试多通道读取"""
    from sysu.adc import ADC, CH0, CH1, CH2
    a = ADC(adc_id=1)
    results = a.read_multi([CH0, CH1, CH2])
    assert len(results) == 3
    for val in results:
        assert isinstance(val, int)
        assert 0 <= val <= 4095


def test_adc_class_close():
    """测试ADC close()"""
    from sysu.adc import ADC
    a = ADC(adc_id=1)
    a.close()  # 不应抛异常


def test_adc_class_context_manager():
    """测试上下文管理器"""
    from sysu.adc import ADC, CH0
    with ADC(adc_id=1) as a:
        val = a.read(CH0)
        assert isinstance(val, int)


def test_adc_channel_constants():
    """测试通道常量定义"""
    from sysu import adc
    assert adc.CH0 == 0
    assert adc.CH15 == 15
    assert adc.CH_TEMP == 16
    assert adc.CH_VREFINT == 17
    assert adc.CH_VBAT == 18
