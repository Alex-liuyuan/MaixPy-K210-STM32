"""
tests/test_spi.py
SPI功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_spi_init_deinit(hal):
    h = hal.spi_init(0, 0, 1_000_000)
    assert h is not None
    assert hal.spi_deinit(h) == 0


def test_spi_write(hal):
    h = hal.spi_init(0)
    assert hal.spi_write(h, b"\x01\x02\x03") == 0
    hal.spi_deinit(h)


def test_spi_read(hal):
    h = hal.spi_init(0)
    data = hal.spi_read(h, 4)
    assert isinstance(data, bytes)
    assert len(data) == 4
    hal.spi_deinit(h)


def test_spi_transfer(hal):
    h = hal.spi_init(0)
    rx = hal.spi_transfer(h, b"\xAA\xBB\xCC")
    assert isinstance(rx, bytes)
    assert len(rx) == 3
    hal.spi_deinit(h)


def test_spi_class():
    """测试 sysu.spi.SPI 高层接口"""
    from sysu.spi import SPI
    s = SPI(spi_id=1, baudrate=500_000)
    assert s.write(b"\xFF") == 0
    rx = s.read(2)
    assert len(rx) == 2
    rx2 = s.transfer(b"\x01\x02")
    assert len(rx2) == 2
    s.close()


def test_spi_class_context_manager():
    from sysu.spi import SPI
    with SPI(spi_id=2) as s:
        s.write(bytes([0x9F]))
