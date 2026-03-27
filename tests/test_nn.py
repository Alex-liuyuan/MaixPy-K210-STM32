"""
tests/test_nn.py
神经网络推理测试（模拟HAL）
"""

import pytest
import numpy as np
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


@pytest.fixture
def runner(hal):
    """创建并加载模拟TfliteRunner"""
    r = hal.TfliteRunner(64 * 1024)
    r.load_file("dummy_model.tflite")  # 模拟：路径不需要真实存在
    return r


def test_runner_load(runner):
    assert runner.is_loaded()


def test_runner_input_shape(runner):
    shape = runner.input_shape(0)
    assert len(shape) == 4
    assert shape[1] == 224
    assert shape[2] == 224
    assert shape[3] == 3


def test_runner_output_shape(runner):
    shape = runner.output_shape(0)
    assert len(shape) == 2
    assert shape[1] == 1000


def test_runner_run_float(runner):
    inp = np.random.rand(1 * 224 * 224 * 3).astype(np.float32)
    out = runner.run(inp)
    assert isinstance(out, np.ndarray)
    assert len(out) == 1000
    # softmax输出应在[0,1]范围内
    assert float(out.min()) >= 0.0
    assert float(out.max()) <= 1.0


def test_runner_run_uint8(runner):
    inp = np.random.randint(0, 255, 1 * 224 * 224 * 3, dtype=np.uint8)
    out = runner.run_uint8(inp)
    assert len(out) == 1000


def test_runner_output_sum_approx_one(runner):
    """softmax输出之和应约为1"""
    inp = np.ones(1 * 224 * 224 * 3, dtype=np.float32) * 0.5
    out = runner.run(inp)
    assert abs(float(out.sum()) - 1.0) < 0.01


def test_runner_last_invoke_ms(runner):
    inp = np.zeros(1 * 224 * 224 * 3, dtype=np.float32)
    runner.run(inp)
    assert runner.last_invoke_ms() >= 0.0


def test_classifier_classify():
    """测试 sysu.nn.Classifier 高层接口"""
    from sysu.nn import Classifier
    from sysu.camera import Image

    labels = [f"class_{i}" for i in range(1000)]
    clf = Classifier("dummy.tflite", labels)

    data = np.random.randint(0, 255, (224, 224, 3), dtype=np.uint8)
    img = Image(data, 224, 224, "RGB888")

    results = clf.classify(img)
    assert len(results) >= 1
    class_id, confidence, label = results[0]
    assert 0 <= class_id < 1000
    assert 0.0 <= confidence <= 1.0
    assert label.startswith("class_")


def test_classifier_top_result_is_highest():
    """分类结果应按置信度降序排列"""
    from sysu.nn import Classifier
    from sysu.camera import Image

    labels = [f"cls_{i}" for i in range(10)]
    clf = Classifier("dummy.tflite", labels)
    data = np.zeros((224, 224, 3), dtype=np.uint8)
    img = Image(data, 224, 224, "RGB888")

    results = clf.classify(img)
    if len(results) > 1:
        assert results[0][1] >= results[1][1]


def test_detector_detect():
    """测试 sysu.nn.Detector 高层接口"""
    from sysu.nn import Detector, DetectionResult
    from sysu.camera import Image

    labels = ["cat", "dog", "bird"]
    det = Detector("dummy.tflite", labels, threshold=0.0)

    data = np.random.randint(0, 255, (224, 224, 3), dtype=np.uint8)
    img = Image(data, 224, 224, "RGB888")

    results = det.detect(img)
    assert isinstance(results, list)
    for r in results:
        assert isinstance(r, DetectionResult)
        assert hasattr(r, 'x')
        assert hasattr(r, 'score')
        assert 0.0 <= r.score <= 1.0


def test_nn_unload():
    from sysu.nn import NeuralNetwork
    nn = NeuralNetwork("dummy.tflite")
    assert nn.loaded
    nn.unload()
    assert not nn.loaded


def test_model_info_helper(monkeypatch):
    from sysu import model

    monkeypatch.setattr(model, "path", lambda: "/tmp/demo_model.tflite")
    monkeypatch.setattr(model, "labels_path", lambda: "/tmp/demo_model.txt")
    monkeypatch.setattr(model, "exists", lambda: True)
    monkeypatch.setattr(model, "size", lambda: 128)

    info = model.info()
    assert info["path"] == "/tmp/demo_model.tflite"
    assert info["format"] == "tflite"
    assert info["labels_path"] == "/tmp/demo_model.txt"
    assert info["backend"] == "bundle"
    assert info["present"] is True


def test_neural_network_uses_default_model(monkeypatch):
    import sysu.nn as nn_mod

    monkeypatch.setattr(nn_mod._model_info, "path", lambda: "/tmp/default_model.tflite")
    monkeypatch.setattr(nn_mod._model_info, "labels_path", lambda: "/tmp/default_model.txt")

    nn = nn_mod.NeuralNetwork()
    assert nn.model_path == "/tmp/default_model.tflite"
    assert nn.loaded is True
    assert nn.info()["backend"] == "bundle"
    assert nn.info()["labels_path"] == "/tmp/default_model.txt"
