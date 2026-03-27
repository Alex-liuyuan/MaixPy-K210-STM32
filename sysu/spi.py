"""MaixPy Nano RT-Thread SPI 模块。"""

from . import _current_platform

try:
    import _maix_hal as _hal
    _HAL_AVAILABLE = True
except ImportError:
    _hal = None
    _HAL_AVAILABLE = False


class SPI:
    """统一SPI接口"""

    MODE_MASTER = 0
    MODE_SLAVE  = 1
    DATA_8BIT   = 0
    DATA_16BIT  = 1
    CPOL_LOW    = 0
    CPOL_HIGH   = 1
    CPHA_1EDGE  = 0
    CPHA_2EDGE  = 1

    def __init__(self, spi_id=1, baudrate=1_000_000,
                 freq=None, mode=MODE_MASTER, datasize=DATA_8BIT,
                 cpol=CPOL_LOW, cpha=CPHA_1EDGE,
                 polarity=None, phase=None, bits=None,
                 soft_cs=False, cs_active_low=True):
        """
        初始化SPI

        Args:
            spi_id:        SPI编号（1-6）
            baudrate:      时钟频率（Hz），freq 为别名
            freq:          时钟频率别名（优先于 baudrate）
            mode:          主/从模式
            datasize:      数据位宽
            cpol/polarity: 时钟极性（polarity 为别名）
            cpha/phase:    时钟相位（phase 为别名）
            bits:          数据位宽别名
            soft_cs:       软件片选
            cs_active_low: 片选低有效
        """
        self.spi_id   = spi_id - 1  # 0-based
        self.baudrate = freq if freq is not None else baudrate
        self.soft_cs  = soft_cs
        self.cs_active_low = cs_active_low
        _cpol = polarity if polarity is not None else cpol
        _cpha = phase    if phase    is not None else cpha
        _ds   = bits     if bits     is not None else datasize
        self._handle  = None
        self._open(mode, _ds, _cpol, _cpha)

    def _open(self, mode, datasize, cpol, cpha):
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            self._handle = _hal.spi_init(self.spi_id, mode, self.baudrate,
                                          datasize, cpol, cpha)
            if self._handle is None:
                print(f"[SPI] 初始化失败，使用模拟模式")
        else:
            print(f"[SPI] 模拟SPI{self.spi_id + 1} @ {self.baudrate}Hz")

    def write(self, data, timeout=100):
        """发送数据"""
        if isinstance(data, (list, bytearray)):
            data = bytes(data)
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.spi_write(self._handle, data, timeout)
        print(f"[SPI] 模拟发送 {len(data)} 字节")
        return 0

    def read(self, size, timeout=100):
        """接收数据"""
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.spi_read(self._handle, size, timeout)
        return bytes(size)

    def transfer(self, data, timeout=100):
        """全双工收发"""
        if isinstance(data, (list, bytearray)):
            data = bytes(data)
        if _current_platform == 'stm32' and self._handle is not None:
            return _hal.spi_transfer(self._handle, data, timeout)
        return bytes(len(data))

    def write_read(self, data: bytes, length: int = -1) -> bytes:
        """写入并读取数据（对齐官方 MaixPy v4）"""
        n = length if length > 0 else len(data)
        return self.transfer(data[:n])

    def close(self):
        """关闭SPI"""
        if _current_platform == 'stm32' and self._handle is not None:
            _hal.spi_deinit(self._handle)
            self._handle = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
