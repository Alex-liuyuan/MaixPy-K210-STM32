"""MaixPy Nano RT-Thread UART 模块。"""

from . import _current_platform

# 尝试导入HAL绑定层
try:
    import _maix_hal as _hal
    _HAL_AVAILABLE = True
except ImportError:
    _hal = None
    _HAL_AVAILABLE = False


class UART:
    """统一UART接口"""

    PARITY_NONE = 0
    PARITY_EVEN = 1
    PARITY_ODD  = 2

    def __init__(self, uart_id=1, baudrate=115200, wordlen=8,
                 stopbits=1, parity=PARITY_NONE):
        """
        初始化UART

        Args:
            uart_id:  UART编号（1-8，对应USART1-UART8）
            baudrate: 波特率
            wordlen:  数据位（8或9）
            stopbits: 停止位（1或2）
            parity:   校验位（PARITY_NONE/EVEN/ODD）
        """
        self.uart_id  = uart_id - 1  # 转为0-based
        self.baudrate = baudrate
        self._handle  = None
        self._open(wordlen, stopbits, parity)

    def _open(self, wordlen, stopbits, parity):
        wl = 0 if wordlen == 8 else 1
        sb = 0 if stopbits == 1 else 1
        pa = parity

        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            self._handle = _hal.uart_init(self.uart_id, self.baudrate, wl, sb, pa)
            if self._handle is None:
                print(f"[UART] 初始化失败，使用模拟模式")
        else:
            print(f"[UART] 模拟UART{self.uart_id + 1} @ {self.baudrate}bps")

    def write(self, data, timeout=1000):
        """发送数据（bytes或str）"""
        if isinstance(data, str):
            data = data.encode()
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.uart_write(self._handle, bytes(data), timeout)
        print(f"[UART] 模拟发送 {len(data)} 字节")
        return 0

    def read(self, size=256, timeout=1000):
        """接收数据，返回bytes"""
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.uart_read(self._handle, size, timeout)
        return b''

    def read_dma(self, max_len=256):
        """读取DMA不定长接收缓冲（IDLE中断后调用）"""
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.uart_read_dma(self._handle, max_len)
        return b''

    def readline(self, timeout=1000):
        """读取一行（以\\n结尾）"""
        buf = b''
        while True:
            ch = self.read(1, timeout)
            if not ch:
                break
            buf += ch
            if ch == b'\n':
                break
        return buf

    def close(self):
        """关闭UART"""
        if hasattr(self, '_rx_stop'):
            self._rx_stop.set()
        if _current_platform == 'stm32' and self._handle is not None:
            _hal.uart_deinit(self._handle)
            self._handle = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


import threading as _threading


def _uart_set_received_callback(self, callback):
    """设置异步接收回调。callback(uart_instance, data: bytes)"""
    self._rx_callback = callback
    if not (hasattr(self, '_handle') and self._handle):
        self._start_mock_rx_thread()


def _uart_start_mock_rx_thread(self):
    if getattr(self, '_rx_thread', None) and self._rx_thread.is_alive():
        return
    self._rx_stop = _threading.Event()
    self._rx_thread = _threading.Thread(
        target=self._mock_rx_loop, daemon=True)
    self._rx_thread.start()


def _uart_mock_rx_loop(self):
    import time as _t
    while not self._rx_stop.is_set():
        data = self.read(256, timeout=10)
        if data and getattr(self, '_rx_callback', None):
            self._rx_callback(self, data)
        _t.sleep(0.01)


# 动态绑定到 UART 类
UART.set_received_callback = _uart_set_received_callback
UART._start_mock_rx_thread = _uart_start_mock_rx_thread
UART._mock_rx_loop = _uart_mock_rx_loop
