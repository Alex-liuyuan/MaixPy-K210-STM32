"""
MaixPy Nano RT-Thread 音频模块
"""

import numpy as np
from . import _current_platform


class AudioFrame:
    """音频帧数据容器"""

    def __init__(self, data, sample_rate=16000, channels=1):
        """
        Args:
            data: int16 numpy 数组
            sample_rate: 采样率
            channels: 声道数
        """
        self.data = np.asarray(data, dtype=np.int16)
        self.sample_rate = sample_rate
        self.channels = channels

    @property
    def duration_ms(self):
        """帧时长（毫秒）"""
        samples = len(self.data) // max(self.channels, 1)
        return samples * 1000.0 / self.sample_rate

    def to_bytes(self):
        """转换为字节数据"""
        return self.data.tobytes()

    def to_float(self):
        """归一化到 [-1.0, 1.0]"""
        return self.data.astype(np.float32) / 32768.0


class Audio:
    """统一音频接口"""

    # 格式枚举
    FMT_PCM_S16 = 0
    FMT_PCM_S32 = 1
    FMT_PDM = 2

    _FORMAT_MAP = {"PCM_S16": 0, "PCM_S32": 1, "PDM": 2}

    def __init__(self, sample_rate=16000, channels=1, format="PCM_S16",
                 frame_size=512):
        """
        初始化音频设备

        Args:
            sample_rate: 采样率 (8000/16000/44100/48000)
            channels: 声道数 (1=mono, 2=stereo)
            format: 音频格式 ("PCM_S16", "PCM_S32", "PDM")
            frame_size: 每帧采样数
        """
        self.sample_rate = sample_rate
        self.channels = channels
        self.format = format
        self.frame_size = frame_size
        self._initialized = False
        self._hal = None

        fmt_code = self._FORMAT_MAP.get(format, 0)

        if _current_platform == 'stm32':
            self._init_stm32(sample_rate, channels, fmt_code, frame_size)
        else:
            self._init_mock(sample_rate, channels, fmt_code, frame_size)

    def _init_stm32(self, sr, ch, fmt, fs):
        try:
            import _maix_hal
            ret = _maix_hal.audio_open(sr, ch, fmt, fs)
            if ret != 0:
                raise RuntimeError(f"audio_open 返回 {ret}")
            self._hal = _maix_hal
            self._initialized = True
        except ImportError:
            self._init_mock(sr, ch, fmt, fs)

    def _init_mock(self, sr, ch, fmt, fs):
        try:
            import _maix_hal
            _maix_hal.audio_open(sr, ch, fmt, fs)
            self._hal = _maix_hal
        except ImportError:
            pass
        self._initialized = True

    def read(self, samples=None):
        """
        读取音频数据

        Args:
            samples: 采样数，默认为 frame_size

        Returns:
            numpy.ndarray: int16 数组
        """
        if not self._initialized:
            raise RuntimeError("音频设备未初始化")
        n = samples if samples is not None else self.frame_size
        if self._hal and hasattr(self._hal, 'audio_read'):
            return self._hal.audio_read(n)
        # 无 HAL 时生成静音
        return np.zeros(n, dtype=np.int16)

    def write(self, data):
        """写入音频数据"""
        if not self._initialized:
            raise RuntimeError("音频设备未初始化")
        if self._hal and hasattr(self._hal, 'audio_write'):
            return self._hal.audio_write(data)
        return 0

    def start(self):
        """开始音频流"""
        if self._hal and hasattr(self._hal, 'audio_start'):
            self._hal.audio_start()

    def stop(self):
        """停止音频流"""
        if self._hal and hasattr(self._hal, 'audio_stop'):
            self._hal.audio_stop()

    def set_volume(self, vol):
        """设置音量 (0-100)"""
        if self._hal and hasattr(self._hal, 'audio_set_volume'):
            return self._hal.audio_set_volume(vol)
        return 0

    def get_volume(self):
        """获取音量"""
        if self._hal and hasattr(self._hal, 'audio_get_volume'):
            return self._hal.audio_get_volume()
        return 0

    def close(self):
        """关闭音频设备"""
        if self._initialized:
            if self._hal and hasattr(self._hal, 'audio_close'):
                self._hal.audio_close()
            self._initialized = False

    def __del__(self):
        self.close()
