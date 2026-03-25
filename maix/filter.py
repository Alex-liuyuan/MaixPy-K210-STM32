"""
maix/filter.py
常用数字滤波算法，适用于嵌入式传感器数据处理
"""

from collections import deque


class MovingAverage:
    """滑动平均滤波"""

    def __init__(self, window: int = 8):
        if window < 1:
            raise ValueError("window 必须 >= 1")
        self._buf = deque(maxlen=window)

    def update(self, x: float) -> float:
        self._buf.append(x)
        return sum(self._buf) / len(self._buf)

    def reset(self):
        self._buf.clear()


class MedianFilter:
    """中值滤波（去除脉冲噪声）"""

    def __init__(self, window: int = 5):
        if window < 1:
            raise ValueError("window 必须 >= 1")
        self._buf = deque(maxlen=window)

    def update(self, x: float) -> float:
        self._buf.append(x)
        return sorted(self._buf)[len(self._buf) // 2]

    def reset(self):
        self._buf.clear()


class LowPassFilter:
    """一阶低通滤波（指数加权移动平均）
    y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    alpha 越小截止频率越低，响应越慢
    """

    def __init__(self, alpha: float = 0.1):
        if not 0.0 < alpha <= 1.0:
            raise ValueError("alpha 必须在 (0, 1]")
        self._alpha = alpha
        self._y = None

    def update(self, x: float) -> float:
        if self._y is None:
            self._y = x
        else:
            self._y = self._alpha * x + (1.0 - self._alpha) * self._y
        return self._y

    def reset(self):
        self._y = None


class KalmanFilter1D:
    """一维卡尔曼滤波
    适用于单变量匀速/静止传感器（温度、ADC 等）
    """

    def __init__(self, q: float = 1e-3, r: float = 1e-1, p: float = 1.0):
        """
        q: 过程噪声协方差（系统不确定性）
        r: 测量噪声协方差（传感器噪声）
        p: 初始估计误差协方差
        """
        self._q = q
        self._r = r
        self._p = p
        self._x = None  # 状态估计

    def update(self, z: float) -> float:
        if self._x is None:
            self._x = z
            return self._x

        # 预测
        p_pred = self._p + self._q

        # 更新
        k = p_pred / (p_pred + self._r)          # 卡尔曼增益
        self._x = self._x + k * (z - self._x)   # 状态更新
        self._p = (1.0 - k) * p_pred             # 协方差更新

        return self._x

    def reset(self):
        self._x = None
        self._p = 1.0


class DeadZoneFilter:
    """死区滤波（抑制微小抖动）
    变化量小于 threshold 时保持上次输出
    """

    def __init__(self, threshold: float = 0.5):
        self._threshold = threshold
        self._last = None

    def update(self, x: float) -> float:
        if self._last is None or abs(x - self._last) >= self._threshold:
            self._last = x
        return self._last

    def reset(self):
        self._last = None


class LimitFilter:
    """限幅滤波（丢弃超出范围的突变值）
    相邻两次采样差值超过 max_delta 时，保留上次值
    """

    def __init__(self, max_delta: float = 10.0):
        self._max_delta = max_delta
        self._last = None

    def update(self, x: float) -> float:
        if self._last is None or abs(x - self._last) <= self._max_delta:
            self._last = x
        return self._last

    def reset(self):
        self._last = None
