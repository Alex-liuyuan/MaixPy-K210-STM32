"""串口 WebSocket 桥接：在独立线程中读取串口，通过 asyncio queue 转发到 WebSocket。"""

from __future__ import annotations

import asyncio
import base64
import threading
from typing import Any


class SerialBridge:
    """管理一个串口连接，在后台线程中读取数据，通过 asyncio queue 转发。"""

    def __init__(self):
        self._serial = None
        self._read_thread: threading.Thread | None = None
        self._running = False
        self._queue: asyncio.Queue | None = None
        self._loop: asyncio.AbstractEventLoop | None = None

    @property
    def connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    def open(self, port: str, baudrate: int = 115200, loop: asyncio.AbstractEventLoop | None = None):
        """打开串口并启动后台读取线程。"""
        if self.connected:
            self.close()

        try:
            import serial
        except ImportError:
            raise RuntimeError("pyserial 未安装，请执行: pip install pyserial")

        self._serial = serial.Serial(port, baudrate=baudrate, timeout=0.1)
        self._running = True
        self._loop = loop or asyncio.get_event_loop()
        self._queue = asyncio.Queue()
        self._read_thread = threading.Thread(target=self._reader_loop, daemon=True)
        self._read_thread.start()

    def _reader_loop(self):
        """后台线程：持续读取串口数据并放入 asyncio queue。"""
        while self._running and self._serial and self._serial.is_open:
            try:
                data = self._serial.read(256)
                if data and self._loop and self._queue:
                    self._loop.call_soon_threadsafe(self._queue.put_nowait, data)
            except Exception:
                break

    async def read(self) -> bytes | None:
        """异步读取一条串口数据。"""
        if self._queue is None:
            return None
        try:
            return await asyncio.wait_for(self._queue.get(), timeout=0.5)
        except asyncio.TimeoutError:
            return None

    def write(self, data: bytes):
        """向串口写入数据。"""
        if self._serial and self._serial.is_open:
            self._serial.write(data)

    def close(self):
        """关闭串口和读取线程。"""
        self._running = False
        if self._serial and self._serial.is_open:
            try:
                self._serial.close()
            except Exception:
                pass
        self._serial = None
        if self._read_thread and self._read_thread.is_alive():
            self._read_thread.join(timeout=2)
        self._read_thread = None
        self._queue = None
