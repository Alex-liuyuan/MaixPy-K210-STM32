"""MaixPy Nano RT-Thread ADC 模块。"""

from . import _current_platform

try:
    import _maix_hal as _hal
    _HAL_AVAILABLE = True
except ImportError:
    _hal = None
    _HAL_AVAILABLE = False

# 分辨率常量
RES_BIT_8  = 8
RES_BIT_10 = 10
RES_BIT_12 = 12

# STM32 ADC通道常量（与HAL库一致）
CH0  = 0
CH1  = 1
CH2  = 2
CH3  = 3
CH4  = 4
CH5  = 5
CH6  = 6
CH7  = 7
CH8  = 8
CH9  = 9
CH10 = 10
CH11 = 11
CH12 = 12
CH13 = 13
CH14 = 14
CH15 = 15
CH_TEMP   = 16  # 内部温度传感器
CH_VREFINT = 17  # 内部参考电压
CH_VBAT   = 18  # 电池电压


class ADC:
    """统一ADC接口

    示例：
        adc = ADC(adc_id=1)
        raw = adc.read(CH0)          # 原始值
        volt = adc.read_vol(CH0)     # 电压值（V）
    """

    RESOLUTION = 12  # 默认 12bit ADC

    def __init__(self, adc_id=1, resolution=RES_BIT_12, vref=3.3):
        """
        初始化ADC

        Args:
            adc_id:     ADC编号（1-3）
            resolution: 分辨率位数（8/10/12），默认12
            vref:       参考电压（V），默认3.3V
        """
        self.adc_id    = adc_id - 1  # 0-based
        self.resolution = resolution
        self.vref       = vref
        self._max_val   = (1 << resolution) - 1
        self._init()

    def _init(self):
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            ret = _hal.adc.init(self.adc_id, False, False)
            if ret != 0:
                pass  # 静默降级到模拟模式

    def read(self, channel=CH0):
        """读取原始值（0 ~ 2^resolution-1）"""
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            val = _hal.adc.read(self.adc_id, channel)
            return val if val is not None else 0
        return (channel * 256) % (self._max_val + 1)

    def read_vol(self, channel=None, vref: float = None) -> float:
        """读取电压值（V），对齐官方 MaixPy v4"""
        v  = vref    if vref    is not None else self.vref
        ch = channel if channel is not None else CH0
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            val = _hal.adc.read_voltage(self.adc_id, ch, v)
            return val if val is not None else 0.0
        return self.read(ch) * v / self._max_val

    # 向后兼容别名
    def read_voltage(self, channel=CH0):
        """读取电压值（V）—— read_vol 的向后兼容别名"""
        return self.read_vol(channel)

    def read_multi(self, channels):
        """多通道DMA扫描读取，返回原始值列表"""
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.adc.start_scan_dma(self.adc_id, channels)
            import time as _t
            _t.sleep(0.001)  # 等待DMA完成
            return [_hal.adc.get_dma_result(self.adc_id, i)
                    for i in range(len(channels))]
        return [self.read(ch) for ch in channels]

    def close(self):
        """释放ADC"""
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.adc.deinit(self.adc_id)

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
