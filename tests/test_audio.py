"""
音频基础接口测试
"""

import numpy as np
import pytest
from maix.audio import Audio, AudioFrame


class TestAudioOpenClose:
    def test_audio_open_close(self):
        a = Audio(sample_rate=16000, channels=1)
        assert a._initialized
        a.close()
        assert not a._initialized

    def test_audio_default_params(self):
        a = Audio()
        assert a.sample_rate == 16000
        assert a.channels == 1
        assert a.format == "PCM_S16"
        assert a.frame_size == 512
        a.close()

    def test_audio_custom_params(self):
        a = Audio(sample_rate=44100, channels=2, format="PCM_S32", frame_size=1024)
        assert a.sample_rate == 44100
        assert a.channels == 2
        assert a.frame_size == 1024
        a.close()


class TestAudioRead:
    def test_audio_read_returns_int16_array(self):
        a = Audio()
        data = a.read()
        assert isinstance(data, np.ndarray)
        assert data.dtype == np.int16
        a.close()

    def test_audio_read_correct_length(self):
        a = Audio(frame_size=256)
        data = a.read()
        assert len(data) == 256
        a.close()

    def test_audio_read_custom_samples(self):
        a = Audio()
        data = a.read(samples=1024)
        assert len(data) == 1024
        a.close()

    def test_audio_read_default_frame_size(self):
        a = Audio(frame_size=512)
        data = a.read()
        assert len(data) == 512
        a.close()


class TestAudioWrite:
    def test_audio_write(self):
        a = Audio()
        data = np.zeros(512, dtype=np.int16)
        ret = a.write(data)
        assert ret == 0
        a.close()


class TestAudioVolume:
    def test_audio_set_get_volume(self):
        a = Audio()
        a.set_volume(50)
        vol = a.get_volume()
        assert isinstance(vol, (int, float))
        assert 0 <= vol <= 100
        a.close()

    def test_audio_volume_clamp(self):
        a = Audio()
        a.set_volume(0)
        assert a.get_volume() == 0
        a.set_volume(100)
        assert a.get_volume() == 100
        a.close()


class TestAudioStartStop:
    def test_audio_start_stop(self):
        a = Audio()
        a.start()
        a.stop()
        a.close()


class TestAudioFrame:
    def test_audio_frame_duration(self):
        data = np.zeros(16000, dtype=np.int16)
        frame = AudioFrame(data, sample_rate=16000, channels=1)
        assert abs(frame.duration_ms - 1000.0) < 0.1

    def test_audio_frame_duration_stereo(self):
        data = np.zeros(32000, dtype=np.int16)
        frame = AudioFrame(data, sample_rate=16000, channels=2)
        assert abs(frame.duration_ms - 1000.0) < 0.1

    def test_audio_frame_to_float_range(self):
        data = np.array([0, 16384, -16384, 32767, -32768], dtype=np.int16)
        frame = AudioFrame(data, sample_rate=16000)
        f = frame.to_float()
        assert f.dtype == np.float32
        assert np.all(f >= -1.0)
        assert np.all(f <= 1.0)

    def test_audio_frame_to_bytes(self):
        data = np.array([1, 2, 3], dtype=np.int16)
        frame = AudioFrame(data, sample_rate=16000)
        b = frame.to_bytes()
        assert isinstance(b, bytes)
        assert len(b) == 6  # 3 samples * 2 bytes

    def test_audio_frame_data_type(self):
        data = [100, 200, -100]
        frame = AudioFrame(data, sample_rate=8000)
        assert frame.data.dtype == np.int16
