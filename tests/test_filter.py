"""
tests/test_filter.py
数字滤波算法测试
覆盖：MovingAverage / MedianFilter / LowPassFilter /
      KalmanFilter1D / DeadZoneFilter / LimitFilter
"""

import math
import random
import sys
import os

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from sysu.filter import (
    MovingAverage,
    MedianFilter,
    LowPassFilter,
    KalmanFilter1D,
    DeadZoneFilter,
    LimitFilter,
)

random.seed(42)


# ─────────────────────────────────────────────────────────────────────────────
# 工具函数
# ─────────────────────────────────────────────────────────────────────────────

def _sine_wave(n: int, freq: float = 1.0, fs: float = 100.0, amp: float = 1.0):
    """生成 n 点正弦序列"""
    return [amp * math.sin(2 * math.pi * freq * i / fs) for i in range(n)]


def _add_noise(signal, sigma: float = 0.1):
    """叠加高斯白噪声"""
    return [x + random.gauss(0, sigma) for x in signal]


def _add_spikes(signal, n_spikes: int = 5, amplitude: float = 10.0):
    """在随机位置插入脉冲噪声"""
    noisy = list(signal)
    indices = random.sample(range(len(noisy)), min(n_spikes, len(noisy)))
    for i in indices:
        noisy[i] += amplitude * random.choice([-1, 1])
    return noisy, indices


def _rmse(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)) / len(a))


# ─────────────────────────────────────────────────────────────────────────────
# MovingAverage
# ─────────────────────────────────────────────────────────────────────────────

class TestMovingAverage:

    def test_constant_signal(self):
        """常数输入，输出应等于该常数"""
        f = MovingAverage(window=8)
        for _ in range(20):
            out = f.update(5.0)
        assert abs(out - 5.0) < 1e-9

    def test_window_1_passthrough(self):
        """窗口为 1 时，输出等于输入"""
        f = MovingAverage(window=1)
        for v in [1.0, 3.0, -2.5, 100.0]:
            assert f.update(v) == v

    def test_average_correctness(self):
        """手动验证前几步均值"""
        f = MovingAverage(window=4)
        assert f.update(4.0) == pytest.approx(4.0)
        assert f.update(8.0) == pytest.approx(6.0)
        assert f.update(0.0) == pytest.approx(4.0)
        assert f.update(4.0) == pytest.approx(4.0)
        # 窗口满后滑动：[8, 0, 4, 2]
        assert f.update(2.0) == pytest.approx(3.5)

    def test_noise_reduction(self):
        """滤波后 RMSE 应小于原始噪声 RMSE"""
        clean = _sine_wave(200)
        noisy = _add_noise(clean, sigma=0.3)
        f = MovingAverage(window=10)
        filtered = [f.update(x) for x in noisy]
        assert _rmse(filtered, clean) < _rmse(noisy, clean)

    def test_reset(self):
        """reset 后状态清空，重新从单点均值开始"""
        f = MovingAverage(window=4)
        for v in [1, 2, 3, 4]:
            f.update(v)
        f.reset()
        assert f.update(10.0) == pytest.approx(10.0)

    def test_invalid_window(self):
        with pytest.raises(ValueError):
            MovingAverage(window=0)


# ─────────────────────────────────────────────────────────────────────────────
# MedianFilter
# ─────────────────────────────────────────────────────────────────────────────

class TestMedianFilter:

    def test_constant_signal(self):
        f = MedianFilter(window=5)
        for _ in range(10):
            out = f.update(3.0)
        assert out == pytest.approx(3.0)

    def test_spike_rejection(self):
        """脉冲噪声应被中值滤波抑制"""
        f = MedianFilter(window=5)
        # 先填满窗口（全为 1.0）
        for _ in range(4):
            f.update(1.0)
        # 注入脉冲
        out = f.update(100.0)
        # 窗口 [1,1,1,1,100]，中值 = 1.0
        assert out == pytest.approx(1.0)

    def test_odd_window_median(self):
        """奇数窗口，中值精确"""
        f = MedianFilter(window=5)
        for v in [3, 1, 4, 1, 5]:
            out = f.update(v)
        # sorted([3,1,4,1,5]) = [1,1,3,4,5]，中值 = 3
        assert out == pytest.approx(3.0)

    def test_even_window_median(self):
        """偶数窗口，取下中位数"""
        f = MedianFilter(window=4)
        for v in [2, 4, 6, 8]:
            out = f.update(v)
        # sorted = [2,4,6,8]，index 2 → 6
        assert out == pytest.approx(6.0)

    def test_spike_rmse(self):
        """含脉冲噪声时，中值滤波 RMSE 优于原始信号"""
        clean = _sine_wave(200)
        noisy, _ = _add_spikes(clean, n_spikes=10, amplitude=5.0)
        f = MedianFilter(window=5)
        filtered = [f.update(x) for x in noisy]
        assert _rmse(filtered, clean) < _rmse(noisy, clean)

    def test_reset(self):
        f = MedianFilter(window=3)
        for v in [10, 20, 30]:
            f.update(v)
        f.reset()
        assert f.update(7.0) == pytest.approx(7.0)

    def test_invalid_window(self):
        with pytest.raises(ValueError):
            MedianFilter(window=0)


# ─────────────────────────────────────────────────────────────────────────────
# LowPassFilter
# ─────────────────────────────────────────────────────────────────────────────

class TestLowPassFilter:

    def test_first_sample_passthrough(self):
        """第一个样本直接输出"""
        f = LowPassFilter(alpha=0.5)
        assert f.update(42.0) == pytest.approx(42.0)

    def test_convergence_to_constant(self):
        """持续输入常数，输出应收敛到该常数"""
        f = LowPassFilter(alpha=0.2)
        for _ in range(100):
            out = f.update(10.0)
        assert abs(out - 10.0) < 1e-3

    def test_alpha_1_passthrough(self):
        """alpha=1 时等同于直通"""
        f = LowPassFilter(alpha=1.0)
        for v in [1.0, 5.0, -3.0]:
            assert f.update(v) == pytest.approx(v)

    def test_smoothing_effect(self):
        """低通滤波后高频噪声应减少"""
        clean = _sine_wave(300, freq=1.0, fs=100.0)
        noisy = _add_noise(clean, sigma=0.5)
        f = LowPassFilter(alpha=0.15)
        filtered = [f.update(x) for x in noisy]
        assert _rmse(filtered, clean) < _rmse(noisy, clean)

    def test_step_response_monotone(self):
        """阶跃响应应单调趋近目标值"""
        f = LowPassFilter(alpha=0.3)
        f.update(0.0)
        prev = 0.0
        for _ in range(30):
            out = f.update(1.0)
            assert out >= prev - 1e-12
            prev = out
        assert out < 1.0  # 未完全到达

    def test_reset(self):
        f = LowPassFilter(alpha=0.5)
        f.update(100.0)
        f.reset()
        assert f.update(1.0) == pytest.approx(1.0)

    def test_invalid_alpha(self):
        with pytest.raises(ValueError):
            LowPassFilter(alpha=0.0)
        with pytest.raises(ValueError):
            LowPassFilter(alpha=1.1)


# ─────────────────────────────────────────────────────────────────────────────
# KalmanFilter1D
# ─────────────────────────────────────────────────────────────────────────────

class TestKalmanFilter1D:

    def test_first_sample_passthrough(self):
        f = KalmanFilter1D()
        assert f.update(5.0) == pytest.approx(5.0)

    def test_convergence_to_constant(self):
        """静止信号加噪声，卡尔曼应收敛到真值附近"""
        f = KalmanFilter1D(q=1e-4, r=0.1)
        true_val = 25.0
        for _ in range(200):
            z = true_val + random.gauss(0, 0.3)
            out = f.update(z)
        assert abs(out - true_val) < 0.5

    def test_noise_reduction(self):
        """卡尔曼滤波后 RMSE 应优于原始噪声"""
        clean = [5.0] * 200
        noisy = _add_noise(clean, sigma=1.0)
        f = KalmanFilter1D(q=1e-4, r=1.0)
        filtered = [f.update(x) for x in noisy]
        assert _rmse(filtered, clean) < _rmse(noisy, clean)

    def test_tracks_ramp(self):
        """缓慢斜坡信号，卡尔曼应能跟踪（误差有界）"""
        f = KalmanFilter1D(q=0.01, r=0.1)
        errors = []
        for i in range(100):
            true_val = i * 0.1
            z = true_val + random.gauss(0, 0.05)
            out = f.update(z)
            if i > 10:  # 跳过初始收敛阶段
                errors.append(abs(out - true_val))
        assert max(errors) < 1.0

    def test_high_r_more_smooth(self):
        """r 越大（信任测量越少），输出越平滑"""
        noisy = _add_noise([0.0] * 100, sigma=2.0)
        f_low_r  = KalmanFilter1D(q=1e-3, r=0.1)
        f_high_r = KalmanFilter1D(q=1e-3, r=10.0)
        out_low  = [f_low_r.update(x)  for x in noisy]
        out_high = [f_high_r.update(x) for x in noisy]
        # 高 r 的输出方差应更小
        var_low  = sum(x**2 for x in out_low)  / len(out_low)
        var_high = sum(x**2 for x in out_high) / len(out_high)
        assert var_high < var_low

    def test_reset(self):
        f = KalmanFilter1D()
        for _ in range(50):
            f.update(random.gauss(100, 5))
        f.reset()
        assert f.update(0.0) == pytest.approx(0.0)


# ─────────────────────────────────────────────────────────────────────────────
# DeadZoneFilter
# ─────────────────────────────────────────────────────────────────────────────

class TestDeadZoneFilter:

    def test_first_sample_passthrough(self):
        f = DeadZoneFilter(threshold=1.0)
        assert f.update(3.0) == pytest.approx(3.0)

    def test_small_change_suppressed(self):
        """变化量小于阈值时，输出保持不变"""
        f = DeadZoneFilter(threshold=2.0)
        f.update(10.0)
        assert f.update(10.5) == pytest.approx(10.0)
        assert f.update(11.0) == pytest.approx(10.0)
        assert f.update(11.9) == pytest.approx(10.0)

    def test_large_change_passes(self):
        """变化量 >= 阈值时，输出更新"""
        f = DeadZoneFilter(threshold=2.0)
        f.update(10.0)
        assert f.update(12.0) == pytest.approx(12.0)
        assert f.update(9.9)  == pytest.approx(9.9)

    def test_threshold_boundary(self):
        """恰好等于阈值时应通过"""
        f = DeadZoneFilter(threshold=1.0)
        f.update(0.0)
        assert f.update(1.0) == pytest.approx(1.0)

    def test_jitter_suppression(self):
        """微小抖动信号，死区滤波后输出应比输入更稳定"""
        base = 50.0
        jitter = [base + random.uniform(-0.3, 0.3) for _ in range(100)]
        f = DeadZoneFilter(threshold=0.5)
        filtered = [f.update(x) for x in jitter]
        # 输出变化次数应远少于输入
        changes_in  = sum(1 for i in range(1, len(jitter))   if jitter[i]   != jitter[i-1])
        changes_out = sum(1 for i in range(1, len(filtered)) if filtered[i] != filtered[i-1])
        assert changes_out < changes_in

    def test_reset(self):
        f = DeadZoneFilter(threshold=5.0)
        f.update(100.0)
        f.reset()
        assert f.update(1.0) == pytest.approx(1.0)


# ─────────────────────────────────────────────────────────────────────────────
# LimitFilter
# ─────────────────────────────────────────────────────────────────────────────

class TestLimitFilter:

    def test_first_sample_passthrough(self):
        f = LimitFilter(max_delta=5.0)
        assert f.update(7.0) == pytest.approx(7.0)

    def test_normal_change_passes(self):
        """变化量 <= max_delta 时正常更新"""
        f = LimitFilter(max_delta=10.0)
        f.update(0.0)
        assert f.update(10.0) == pytest.approx(10.0)
        assert f.update(5.0)  == pytest.approx(5.0)

    def test_spike_rejected(self):
        """超出限幅范围的突变值应被丢弃"""
        f = LimitFilter(max_delta=5.0)
        f.update(10.0)
        assert f.update(100.0) == pytest.approx(10.0)  # 突变，保留旧值
        assert f.update(10.0)  == pytest.approx(10.0)  # 回到正常

    def test_boundary_exact(self):
        """恰好等于 max_delta 时应通过"""
        f = LimitFilter(max_delta=5.0)
        f.update(0.0)
        assert f.update(5.0) == pytest.approx(5.0)

    def test_negative_spike(self):
        """负方向突变也应被限幅"""
        f = LimitFilter(max_delta=5.0)
        f.update(50.0)
        assert f.update(-100.0) == pytest.approx(50.0)

    def test_gradual_ramp_passes(self):
        """缓慢斜坡（每步 <= max_delta）应全部通过"""
        f = LimitFilter(max_delta=1.0)
        expected = 0.0
        for i in range(20):
            out = f.update(float(i))
            assert out == pytest.approx(float(i))

    def test_reset(self):
        f = LimitFilter(max_delta=1.0)
        f.update(999.0)
        f.reset()
        assert f.update(0.0) == pytest.approx(0.0)


# ─────────────────────────────────────────────────────────────────────────────
# 综合对比测试
# ─────────────────────────────────────────────────────────────────────────────

class TestFilterComparison:

    def test_all_filters_reduce_noise(self):
        """所有滤波器在高斯噪声下 RMSE 均优于原始信号
        使用静止信号（常数），卡尔曼在此场景下收敛最快
        """
        clean = [5.0] * 300
        noisy = _add_noise(clean, sigma=0.4)
        WARMUP = 20

        filters = {
            "MovingAverage": MovingAverage(window=8),
            "MedianFilter":  MedianFilter(window=5),
            "LowPassFilter": LowPassFilter(alpha=0.2),
            "KalmanFilter":  KalmanFilter1D(q=1e-4, r=0.16),
        }

        raw_rmse = _rmse(noisy[WARMUP:], clean[WARMUP:])
        for name, f in filters.items():
            filtered = [f.update(x) for x in noisy]
            assert _rmse(filtered[WARMUP:], clean[WARMUP:]) < raw_rmse, \
                f"{name} 未能降低 RMSE（raw={raw_rmse:.4f}）"

    def test_median_best_for_spikes(self):
        """脉冲噪声场景下，中值滤波应优于滑动平均"""
        clean = [0.0] * 200
        noisy, _ = _add_spikes(clean, n_spikes=20, amplitude=50.0)

        ma = MovingAverage(window=5)
        mf = MedianFilter(window=5)

        out_ma = [ma.update(x) for x in noisy]
        out_mf = [mf.update(x) for x in noisy]

        assert _rmse(out_mf, clean) < _rmse(out_ma, clean)

    def test_kalman_best_for_gaussian(self):
        """高斯噪声场景下，卡尔曼滤波 RMSE 应优于简单滑动平均"""
        clean = [10.0] * 300
        noisy = _add_noise(clean, sigma=2.0)

        ma = MovingAverage(window=5)
        kf = KalmanFilter1D(q=1e-5, r=4.0)

        out_ma = [ma.update(x) for x in noisy]
        out_kf = [kf.update(x) for x in noisy]

        # 跳过前 20 个收敛样本
        assert _rmse(out_kf[20:], clean[20:]) < _rmse(out_ma[20:], clean[20:])

    def test_chained_filters(self):
        """限幅 + 低通串联：先去脉冲，再平滑"""
        clean = _sine_wave(200, freq=1.0, fs=100.0)
        noisy, _ = _add_spikes(clean, n_spikes=10, amplitude=20.0)
        noisy = _add_noise(noisy, sigma=0.2)

        limit = LimitFilter(max_delta=2.0)
        lpf   = LowPassFilter(alpha=0.2)

        filtered = [lpf.update(limit.update(x)) for x in noisy]
        assert _rmse(filtered, clean) < _rmse(noisy, clean)
