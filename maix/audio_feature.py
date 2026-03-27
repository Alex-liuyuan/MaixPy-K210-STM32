"""
MaixPy Nano RT-Thread 音频特征提取模块

纯 numpy 实现，零外部依赖。支持 MFCC、Mel 频谱图、短时傅里叶变换、帧能量、预加重。
"""

import numpy as np


def _hz_to_mel(hz):
    """Hz → Mel 频率"""
    return 2595.0 * np.log10(1.0 + hz / 700.0)


def _mel_to_hz(mel):
    """Mel → Hz 频率"""
    return 700.0 * (10.0 ** (mel / 2595.0) - 1.0)


def _mel_filterbank(n_mels, n_fft, sample_rate):
    """构建 Mel 三角滤波器组

    Returns:
        (n_mels, n_fft//2+1) numpy 数组
    """
    n_freqs = n_fft // 2 + 1
    low_mel = _hz_to_mel(0)
    high_mel = _hz_to_mel(sample_rate / 2.0)
    mel_points = np.linspace(low_mel, high_mel, n_mels + 2)
    hz_points = _mel_to_hz(mel_points)
    bin_points = np.floor((n_fft + 1) * hz_points / sample_rate).astype(int)

    filters = np.zeros((n_mels, n_freqs))
    for i in range(n_mels):
        left = bin_points[i]
        center = bin_points[i + 1]
        right = bin_points[i + 2]
        for j in range(left, center):
            if center != left:
                filters[i, j] = (j - left) / (center - left)
        for j in range(center, right):
            if right != center:
                filters[i, j] = (right - j) / (right - center)
    return filters


def _dct_ii(x, n_out):
    """Type-II DCT（手写实现，避免 scipy 依赖）

    Args:
        x: (n_frames, n_features) 输入
        n_out: 输出系数数量

    Returns:
        (n_frames, n_out) DCT 系数
    """
    n = x.shape[1]
    k = np.arange(n_out).reshape(-1, 1)
    ns = np.arange(n).reshape(1, -1)
    basis = np.cos(np.pi * k * (2 * ns + 1) / (2 * n))
    return x @ basis.T


def _frame_signal(signal, frame_size, hop_length):
    """将信号分帧

    Returns:
        (n_frames, frame_size) numpy 数组
    """
    sig_len = len(signal)
    if sig_len <= frame_size:
        # 不足一帧时零填充
        padded = np.zeros(frame_size, dtype=signal.dtype)
        padded[:sig_len] = signal
        return padded.reshape(1, -1)
    n_frames = 1 + (sig_len - frame_size) // hop_length
    indices = (np.arange(frame_size).reshape(1, -1)
               + np.arange(n_frames).reshape(-1, 1) * hop_length)
    return signal[indices]


def pre_emphasis(audio_data, coeff=0.97):
    """预加重滤波

    Args:
        audio_data: 一维音频信号
        coeff: 预加重系数

    Returns:
        预加重后的信号
    """
    signal = np.asarray(audio_data, dtype=np.float64)
    return np.append(signal[0], signal[1:] - coeff * signal[:-1])


def compute_energy(audio_data, frame_size=512, hop_length=160):
    """计算帧能量

    Args:
        audio_data: 一维音频信号
        frame_size: 帧长
        hop_length: 帧移

    Returns:
        (n_frames,) 每帧能量
    """
    signal = np.asarray(audio_data, dtype=np.float64)
    frames = _frame_signal(signal, frame_size, hop_length)
    return np.sum(frames ** 2, axis=1)


def compute_spectrogram(audio_data, n_fft=512, hop_length=160):
    """计算短时傅里叶变换频谱图（幅度谱）

    Args:
        audio_data: 一维音频信号
        n_fft: FFT 窗口大小
        hop_length: 帧移

    Returns:
        (n_frames, n_fft//2+1) 幅度谱
    """
    signal = np.asarray(audio_data, dtype=np.float64)
    frames = _frame_signal(signal, n_fft, hop_length)
    # Hann 窗
    window = 0.5 * (1 - np.cos(2 * np.pi * np.arange(n_fft) / n_fft))
    windowed = frames * window
    spectrum = np.fft.rfft(windowed, n=n_fft)
    return np.abs(spectrum)


def compute_mel_spectrogram(audio_data, sample_rate=16000,
                            n_fft=512, hop_length=160, n_mels=40):
    """计算 Mel 频谱图

    Args:
        audio_data: 一维音频信号
        sample_rate: 采样率
        n_fft: FFT 窗口大小
        hop_length: 帧移
        n_mels: Mel 滤波器数量

    Returns:
        (n_frames, n_mels) Mel 频谱图（对数能量）
    """
    spec = compute_spectrogram(audio_data, n_fft, hop_length)
    power_spec = spec ** 2
    mel_basis = _mel_filterbank(n_mels, n_fft, sample_rate)
    mel_spec = power_spec @ mel_basis.T
    # 对数压缩，加小量避免 log(0)
    return np.log(mel_spec + 1e-10)


def compute_mfcc(audio_data, sample_rate=16000, n_mfcc=13,
                 n_fft=512, hop_length=160, n_mels=40):
    """提取 MFCC 特征

    Args:
        audio_data: 一维音频信号
        sample_rate: 采样率
        n_mfcc: MFCC 系数数量
        n_fft: FFT 窗口大小
        hop_length: 帧移
        n_mels: Mel 滤波器数量

    Returns:
        (n_frames, n_mfcc) MFCC 特征矩阵
    """
    mel_spec = compute_mel_spectrogram(audio_data, sample_rate,
                                       n_fft, hop_length, n_mels)
    return _dct_ii(mel_spec, n_mfcc)
