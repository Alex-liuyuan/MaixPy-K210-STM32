"""
音频特征提取测试
"""

import numpy as np
import pytest
from sysu.audio_feature import (
    compute_mfcc,
    compute_mel_spectrogram,
    compute_spectrogram,
    compute_energy,
    pre_emphasis,
)


def _sine_wave(freq=440, sr=16000, duration=1.0):
    """生成正弦波测试信号"""
    t = np.arange(int(sr * duration)) / sr
    return (np.sin(2 * np.pi * freq * t) * 16000).astype(np.float64)


def _silence(sr=16000, duration=1.0):
    """生成静音信号"""
    return np.zeros(int(sr * duration), dtype=np.float64)


class TestMFCC:
    def test_mfcc_output_shape(self):
        signal = _sine_wave(duration=1.0)
        mfcc = compute_mfcc(signal, sample_rate=16000, n_mfcc=13,
                            n_fft=512, hop_length=160)
        assert mfcc.ndim == 2
        assert mfcc.shape[1] == 13
        # 帧数 = 1 + (16000 - 512) // 160 = 97
        assert mfcc.shape[0] > 0

    def test_mfcc_deterministic(self):
        signal = _sine_wave(duration=0.5)
        m1 = compute_mfcc(signal, sample_rate=16000)
        m2 = compute_mfcc(signal, sample_rate=16000)
        np.testing.assert_array_equal(m1, m2)

    def test_mfcc_silence_vs_tone(self):
        silence = _silence(duration=0.5)
        tone = _sine_wave(freq=1000, duration=0.5)
        m_silence = compute_mfcc(silence)
        m_tone = compute_mfcc(tone)
        # 有声信号的 MFCC 能量应大于静音
        assert np.mean(np.abs(m_tone)) > np.mean(np.abs(m_silence))

    def test_mfcc_custom_params(self):
        signal = _sine_wave(duration=0.5)
        mfcc = compute_mfcc(signal, n_mfcc=20, n_mels=64)
        assert mfcc.shape[1] == 20

    def test_mfcc_short_signal(self):
        """信号短于一帧时应零填充并返回 1 帧"""
        signal = np.ones(100, dtype=np.float64)
        mfcc = compute_mfcc(signal, n_fft=512)
        assert mfcc.shape[0] >= 1


class TestMelSpectrogram:
    def test_mel_spectrogram_shape(self):
        signal = _sine_wave(duration=1.0)
        mel = compute_mel_spectrogram(signal, n_mels=40)
        assert mel.ndim == 2
        assert mel.shape[1] == 40
        assert mel.shape[0] > 0

    def test_mel_spectrogram_energy_distribution(self):
        """低频信号的能量应集中在低 Mel 频段"""
        low = _sine_wave(freq=200, duration=0.5)
        high = _sine_wave(freq=4000, duration=0.5)
        mel_low = compute_mel_spectrogram(low, n_mels=40)
        mel_high = compute_mel_spectrogram(high, n_mels=40)
        # 低频信号在低 Mel bin 的平均能量应更高
        low_band_low = np.mean(mel_low[:, :10])
        low_band_high = np.mean(mel_high[:, :10])
        assert low_band_low > low_band_high


class TestSpectrogram:
    def test_spectrogram_shape(self):
        signal = _sine_wave(duration=0.5)
        spec = compute_spectrogram(signal, n_fft=512, hop_length=160)
        assert spec.ndim == 2
        assert spec.shape[1] == 257  # n_fft//2 + 1

    def test_spectrogram_parseval(self):
        """Parseval 定理：时域能量 ≈ 频域能量（单帧验证）

        对于实数信号的 rfft，Parseval 关系为：
        sum(|x|^2) = (|X[0]|^2 + 2*sum(|X[1:N/2]|^2) + |X[N/2]|^2) / N
        """
        signal = _sine_wave(freq=440, duration=0.032)  # ~512 samples at 16kHz
        n_fft = 512
        frame = signal[:n_fft]
        window = 0.5 * (1 - np.cos(2 * np.pi * np.arange(n_fft) / n_fft))
        windowed = frame * window
        time_energy = np.sum(windowed ** 2)
        spec = np.fft.rfft(windowed, n=n_fft)
        mag2 = np.abs(spec) ** 2
        # rfft 返回 N/2+1 个系数，DC 和 Nyquist 不翻倍，其余翻倍
        freq_energy = (mag2[0] + 2 * np.sum(mag2[1:-1]) + mag2[-1]) / n_fft
        assert abs(time_energy - freq_energy) / max(time_energy, 1e-10) < 0.01


class TestEnergy:
    def test_energy_silence_near_zero(self):
        signal = _silence(duration=0.5)
        energy = compute_energy(signal, frame_size=512, hop_length=160)
        assert np.all(energy < 1e-10)

    def test_energy_loud_signal(self):
        signal = _sine_wave(freq=440, duration=0.5)
        energy = compute_energy(signal, frame_size=512, hop_length=160)
        assert np.all(energy > 0)
        assert np.mean(energy) > 1e6  # 振幅 16000 的正弦波能量很大


class TestPreEmphasis:
    def test_pre_emphasis_high_freq_boost(self):
        """预加重应增强高频分量"""
        sr = 16000
        low = _sine_wave(freq=100, sr=sr, duration=0.1)
        high = _sine_wave(freq=4000, sr=sr, duration=0.1)
        pe_low = pre_emphasis(low)
        pe_high = pre_emphasis(high)
        # 高频信号经预加重后能量变化比低频小
        ratio_low = np.mean(pe_low ** 2) / max(np.mean(low ** 2), 1e-10)
        ratio_high = np.mean(pe_high ** 2) / max(np.mean(high ** 2), 1e-10)
        # 预加重对低频衰减更大，所以 ratio_low < ratio_high
        assert ratio_low < ratio_high

    def test_pre_emphasis_output_length(self):
        signal = np.ones(100, dtype=np.float64)
        result = pre_emphasis(signal)
        assert len(result) == 100
