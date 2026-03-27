"""
音频 AI 推理测试
"""

import numpy as np
import pytest
from sysu.nn import SpeechKWS, AudioClassifier, VAD


def _sine_wave(freq=440, sr=16000, duration=1.0):
    t = np.arange(int(sr * duration)) / sr
    return (np.sin(2 * np.pi * freq * t) * 16000).astype(np.float32)


def _silence(sr=16000, duration=1.0):
    return np.zeros(int(sr * duration), dtype=np.float32)


class TestSpeechKWS:
    def test_kws_recognize_returns_list(self):
        kws = SpeechKWS(keywords=["yes", "no", "stop", "go"])
        signal = _sine_wave(duration=1.0)
        results = kws.recognize(signal)
        assert isinstance(results, list)
        assert len(results) > 0

    def test_kws_confidence_range(self):
        kws = SpeechKWS(keywords=["yes", "no"])
        signal = _sine_wave(duration=1.0)
        results = kws.recognize(signal)
        for kw, conf in results:
            assert 0.0 <= conf <= 1.0

    def test_kws_with_keywords(self):
        keywords = ["hello", "world", "test"]
        kws = SpeechKWS(keywords=keywords)
        signal = _sine_wave(duration=1.0)
        results = kws.recognize(signal)
        returned_kws = [kw for kw, _ in results]
        for kw in returned_kws:
            assert kw in keywords

    def test_kws_deterministic(self):
        kws = SpeechKWS(keywords=["a", "b", "c"])
        signal = _sine_wave(duration=0.5)
        r1 = kws.recognize(signal)
        r2 = kws.recognize(signal)
        assert r1 == r2

    def test_kws_sorted_by_confidence(self):
        kws = SpeechKWS(keywords=["yes", "no", "stop", "go"])
        signal = _sine_wave(duration=1.0)
        results = kws.recognize(signal)
        confs = [c for _, c in results]
        assert confs == sorted(confs, reverse=True)


class TestAudioClassifier:
    def test_audio_classifier_classify(self):
        clf = AudioClassifier(labels=["speech", "music", "noise"])
        signal = _sine_wave(duration=1.0)
        results = clf.classify(signal)
        assert isinstance(results, list)
        assert len(results) > 0

    def test_audio_classifier_labels(self):
        labels = ["dog", "cat", "bird", "car"]
        clf = AudioClassifier(labels=labels)
        signal = _sine_wave(duration=1.0)
        results = clf.classify(signal)
        for class_id, conf, label in results:
            assert isinstance(class_id, int)
            assert 0.0 <= conf <= 1.0
            assert isinstance(label, str)

    def test_audio_classifier_confidence_sum(self):
        """分类器输出经 softmax，置信度总和应接近 1"""
        clf = AudioClassifier(labels=["a", "b", "c"])
        signal = _sine_wave(duration=1.0)
        results = clf.classify(signal)
        total = sum(c for _, c, _ in results)
        # 可能只返回部分类别，但总和不应超过 1
        assert total <= 1.01

    def test_audio_classifier_sorted(self):
        clf = AudioClassifier(labels=["a", "b", "c", "d", "e"])
        signal = _sine_wave(duration=1.0)
        results = clf.classify(signal)
        confs = [c for _, c, _ in results]
        assert confs == sorted(confs, reverse=True)


class TestVAD:
    def test_vad_is_speech_returns_bool(self):
        vad = VAD(threshold=0.5)
        frame = _sine_wave(freq=440, duration=0.03)
        result = vad.is_speech(frame)
        assert isinstance(result, bool)

    def test_vad_process_returns_segments(self):
        vad = VAD(threshold=0.5, frame_duration_ms=30)
        signal = _sine_wave(duration=0.5)
        segments = vad.process(signal)
        assert isinstance(segments, list)
        for seg in segments:
            assert "start_ms" in seg
            assert "end_ms" in seg
            assert "confidence" in seg
            assert seg["end_ms"] > seg["start_ms"]

    def test_vad_silence_no_speech(self):
        vad = VAD(threshold=100.0)  # 高阈值确保静音不触发
        signal = _silence(duration=0.5)
        segments = vad.process(signal)
        assert len(segments) == 0

    def test_vad_loud_signal_has_speech(self):
        vad = VAD(threshold=0.001, frame_duration_ms=30)
        signal = _sine_wave(freq=440, duration=0.5)
        result = vad.is_speech(signal[:480])
        assert result is True

    def test_vad_process_segment_continuity(self):
        """语音段应该是连续的，不重叠"""
        vad = VAD(threshold=0.5, frame_duration_ms=30)
        signal = _sine_wave(duration=1.0)
        segments = vad.process(signal)
        for i in range(1, len(segments)):
            assert segments[i]["start_ms"] >= segments[i - 1]["end_ms"]
