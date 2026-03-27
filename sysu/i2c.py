"""MaixPy Nano RT-Thread I2C 模块。"""

from . import _current_platform

try:
    import _maix_hal as _hal
    _HAL_AVAILABLE = True
except ImportError:
    _hal = None
    _HAL_AVAILABLE = False


class I2C:
    """统一I2C接口"""

    ADDR_7BIT  = False
    ADDR_10BIT = True

    def __init__(self, i2c_id=1, clock_speed=100_000, addr_10bit=False):
        """
        初始化I2C主机

        Args:
            i2c_id:      I2C编号（1-4）
            clock_speed: 时钟频率（Hz），标准100kHz，快速400kHz
            addr_10bit:  是否使用10bit地址
        """
        self.i2c_id = i2c_id - 1  # 0-based
        self._handle = None
        self._open(clock_speed, addr_10bit)

    def _open(self, clock_speed, addr_10bit):
        if _HAL_AVAILABLE:
            self._handle = _hal.i2c_init(self.i2c_id, clock_speed, addr_10bit)
            if self._handle is None:
                print(f"[I2C] 初始化失败，使用模拟模式")
        else:
            print(f"[I2C] 模拟I2C{self.i2c_id + 1}")

    def write(self, addr, data, timeout=100):
        """向设备发送数据"""
        if isinstance(data, (list, bytearray, int)):
            data = bytes([data] if isinstance(data, int) else data)
        if self._handle is not None and _HAL_AVAILABLE:
            return _hal.i2c_write(self._handle, addr, data, timeout)
        print(f"[I2C] 模拟写入 addr=0x{addr:02X} {len(data)}字节")
        return 0

    def read(self, addr, size, timeout=100):
        """从设备读取数据"""
        if self._handle is not None and _HAL_AVAILABLE:
            return _hal.i2c_read(self._handle, addr, size, timeout)
        return bytes(size)

    def write_reg(self, addr, reg, data, reg_16bit=False, timeout=100):
        """写寄存器（I2C内存写）"""
        if isinstance(data, (list, bytearray, int)):
            data = bytes([data] if isinstance(data, int) else data)
        if self._handle is not None and _HAL_AVAILABLE:
            return _hal.i2c_mem_write(self._handle, addr, reg,
                                       reg_16bit, data, timeout)
        print(f"[I2C] 模拟写寄存器 addr=0x{addr:02X} reg=0x{reg:02X}")
        return 0

    def read_reg(self, addr, reg, size, reg_16bit=False, timeout=100):
        """读寄存器（I2C内存读）"""
        if self._handle is not None and _HAL_AVAILABLE:
            return _hal.i2c_mem_read(self._handle, addr, reg,
                                      reg_16bit, size, timeout)
        return bytes(size)

    def scan(self):
        """扫描总线上的设备（返回地址列表）"""
        found = []
        for addr in range(0x08, 0x78):
            try:
                result = self.write(addr, b'\x00', timeout=10)
                if result == 0:
                    found.append(addr)
            except:
                pass
        return found

    def close(self):
        """关闭I2C"""
        if self._handle is not None and _HAL_AVAILABLE:
            _hal.i2c_deinit(self._handle)
            self._handle = None

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
