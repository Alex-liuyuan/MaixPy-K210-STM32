"""
tests/test_i2c.py
I2C功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_i2c_init_deinit(hal):
    h = hal.i2c_init(0, 100_000)
    assert h is not None
    assert hal.i2c_deinit(h) == 0


def test_i2c_write_read(hal):
    h = hal.i2c_init(0)
    assert hal.i2c_write(h, 0x48, b"\x00") == 0
    data = hal.i2c_read(h, 0x48, 2)
    assert isinstance(data, bytes)
    assert len(data) == 2
    hal.i2c_deinit(h)


def test_i2c_mem_write_read(hal):
    h = hal.i2c_init(0)
    # 写入寄存器0x10
    assert hal.i2c_mem_write(h, 0x50, 0x10, False, b"\xAB\xCD") == 0
    # 读回
    data = hal.i2c_mem_read(h, 0x50, 0x10, False, 2)
    assert data == b"\xAB\xCD"
    hal.i2c_deinit(h)


def test_i2c_class():
    """测试 sysu.i2c.I2C 高层接口"""
    from sysu.i2c import I2C
    bus = I2C(i2c_id=1, clock_speed=400_000)
    assert bus.write(0x68, b"\x6B\x00") == 0
    data = bus.read(0x68, 6)
    assert len(data) == 6
    bus.close()


def test_i2c_mem_class():
    from sysu.i2c import I2C
    with I2C(i2c_id=1) as bus:
        bus.write_reg(0x50, 0x00, b"\x12\x34")
        data = bus.read_reg(0x50, 0x00, 2)
        assert data == b"\x12\x34"
