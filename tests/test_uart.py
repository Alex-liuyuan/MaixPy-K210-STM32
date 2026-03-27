"""
tests/test_uart.py
UART功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_uart_init_deinit(hal):
    h = hal.uart_init(0, 115200)
    assert h is not None
    assert hal.uart_deinit(h) == 0


def test_uart_write(hal):
    h = hal.uart_init(0, 115200)
    assert hal.uart_write(h, b"hello", 100) == 0
    hal.uart_deinit(h)


def test_uart_read_empty(hal):
    h = hal.uart_init(0, 115200)
    data = hal.uart_read(h, 10, 10)
    assert isinstance(data, bytes)
    assert len(data) == 0
    hal.uart_deinit(h)


def test_uart_inject_and_read(hal):
    h = hal.uart_init(0, 115200)
    hal._uart_inject_rx(b"world")
    data = hal.uart_read(h, 5, 100)
    assert data == b"world"
    hal.uart_deinit(h)


def test_uart_class():
    """测试 sysu.uart.UART 高层接口"""
    from sysu.uart import UART
    u = UART(uart_id=1, baudrate=9600)
    assert u.write(b"test") == 0
    data = u.read(10, timeout=10)
    assert isinstance(data, bytes)
    u.close()


def test_uart_class_context_manager():
    from sysu.uart import UART
    with UART(uart_id=2, baudrate=115200) as u:
        u.write("AT\r\n")
