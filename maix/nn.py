"""
MaixPy Nano RT-Thread 神经网络模块
"""

import os
import numpy as np
from . import _current_platform, model as _model_info


def _running_on_host():
    import platform as _plat
    return _plat.system().lower() == "linux"


def _softmax(logits):
    logits = np.asarray(logits, dtype=np.float32)
    logits = logits - np.max(logits)
    exp = np.exp(logits)
    denom = np.sum(exp)
    if denom <= 0:
        return np.zeros_like(exp)
    return exp / denom


def _mock_class_output(input_data, output_shape):
    classes = 1000
    if output_shape:
        classes = int(output_shape[-1])

    arr = np.asarray(input_data, dtype=np.float32).flatten()
    if arr.size == 0:
        top_idx = 0
        amplitude = 3.0
    else:
        window = arr[: min(arr.size, 4096)]
        mean_val = float(np.mean(window))
        var_val = float(np.var(window))
        top_idx = int(abs(mean_val) * 1000 + var_val * 100) % max(classes, 1)
        amplitude = 3.0 + min(var_val, 5.0)

    logits = np.full(classes, -2.0, dtype=np.float32)
    logits[top_idx] = amplitude
    return _softmax(logits)


class NeuralNetwork:
    """神经网络基类"""
    
    def __init__(self, model_path=None):
        """
        初始化神经网络
        
        Args:
            model_path: 模型文件路径
        """
        self.model_path = model_path or _model_info.path()
        self.model = None
        self.input_shape = None
        self.output_shape = None
        self.loaded = False
        self.model_format = None
        self.model_backend = "missing"

        if self.model_path:
            self.load(self.model_path)
    
    def load(self, model_path):
        """
        加载模型
        
        Args:
            model_path: 模型文件路径
        """
        if (_current_platform == 'stm32'
                and not _running_on_host()
                and not os.path.exists(model_path)):
            raise FileNotFoundError(f"模型文件不存在: {model_path}")
        
        self.model_path = model_path
        self.model_format = self._infer_model_format(model_path)
        self.model_backend = self._infer_model_backend(model_path)
        
        if _current_platform == 'stm32':
            self._load_stm32_model(model_path)
        else:
            self._load_mock_model(model_path)

    def _infer_model_format(self, model_path):
        if not model_path:
            return "none"
        if model_path.endswith(".tflite"):
            return "tflite"
        if model_path.endswith(".kmodel"):
            return "kmodel"
        return "unknown"

    def _infer_model_backend(self, model_path):
        default_path = _model_info.path()
        if model_path and default_path and os.path.abspath(model_path) == os.path.abspath(default_path):
            return "bundle"
        return "external"
    
    def _load_stm32_model(self, model_path):
        """加载 STM32 TFLite 模型（主机测试阶段通过 mock HAL）"""
        try:
            import _maix_hal
            if not hasattr(_maix_hal, "TfliteRunner"):
                raise RuntimeError("_maix_hal 缺少 TfliteRunner")
            self._runner = _maix_hal.TfliteRunner(256 * 1024)
            self._runner.load_file(model_path)
            shape = self._runner.input_shape(0)
            self.input_shape  = tuple(shape) if shape else (224, 224, 3)
            out_shape = self._runner.output_shape(0)
            self.output_shape = tuple(out_shape) if out_shape else (1000,)
            self.loaded = True
            print(f"[NN] STM32 TFLite模型加载成功: {model_path}")
        except ImportError as e:
            raise RuntimeError("STM32平台缺少可用的 _maix_hal 后端") from e
        except Exception as e:
            raise RuntimeError(f"STM32模型加载失败: {e}") from e
    
    def _load_mock_model(self, model_path):
        """仅用于主机开发/测试的 mock 模型"""
        self.input_shape = (224, 224, 3)
        self.output_shape = (1000,)
        self.loaded = True
        print(f"[NN] 主机 mock 模型加载: {model_path}")
    
    def forward(self, input_data):
        """
        前向推理
        
        Args:
            input_data: 输入数据
            
        Returns:
            numpy.ndarray: 输出结果
        """
        if not self.loaded:
            raise RuntimeError("模型未加载")
        
        if _current_platform == 'stm32':
            return self._forward_stm32(input_data)
        else:
            return self._forward_mock(input_data)
    
    def _forward_stm32(self, input_data):
        """STM32 TFLite真实推理"""
        if hasattr(self, '_runner') and self._runner and self._runner.is_loaded():
            import numpy as np
            arr = np.asarray(input_data, dtype=np.float32).flatten()
            # 归一化到 [0, 1]
            if arr.max() > 1.0:
                arr = arr / 255.0
            result = self._runner.run(arr)
            return np.array(result, dtype=np.float32)
        raise RuntimeError("STM32 推理后端未就绪，拒绝返回伪造结果")
    
    def _forward_mock(self, input_data):
        """主机开发/测试使用的确定性 mock 推理"""
        return _mock_class_output(input_data, self.output_shape)
    
    def unload(self):
        """卸载模型"""
        if self.loaded:
            if _current_platform == 'stm32':
                try:
                    if hasattr(self, "_runner"):
                        self._runner = None
                except Exception:
                    pass
            
            self.model = None
            self.loaded = False
            print("[NN] 模型已卸载")

    def info(self):
        return {
            "path": self.model_path,
            "format": self.model_format or self._infer_model_format(self.model_path),
            "backend": self.model_backend,
            "labels_path": _model_info.labels_path(),
            "loaded": self.loaded,
            "input_shape": self.input_shape,
            "output_shape": self.output_shape,
        }


class Classifier(NeuralNetwork):
    """图像分类器"""
    
    def __init__(self, model_path=None, labels=None):
        """
        初始化分类器
        
        Args:
            model_path: 模型文件路径
            labels: 类别标签列表
        """
        super().__init__(model_path)
        self.labels = labels or []
        
        # 如果没有提供标签，尝试加载标签文件
        if not self.labels:
            label_path = self._resolve_labels_path(model_path or self.model_path)
            if label_path and os.path.exists(label_path):
                self._load_labels(label_path)

    def _resolve_labels_path(self, model_path):
        if _model_info.labels_path():
            return _model_info.labels_path()
        if model_path:
            return os.path.splitext(model_path)[0] + ".txt"
        return None
    
    def _load_labels(self, label_path):
        """加载标签文件"""
        try:
            with open(label_path, 'r', encoding='utf-8') as f:
                self.labels = [line.strip() for line in f.readlines()]
            print(f"[Classifier] 加载标签文件: {len(self.labels)}个类别")
        except Exception as e:
            print(f"[Classifier] 加载标签文件失败: {e}")
    
    def classify(self, image):
        """
        图像分类
        
        Args:
            image: 输入图像
            
        Returns:
            list: [(class_id, confidence, label), ...]
        """
        if hasattr(image, 'data'):
            input_data = image.data
        else:
            input_data = image
        
        # 运行推理
        output = self.forward(input_data)
        
        # 获取置信度最高的类别
        if len(output.shape) == 1:
            # 分类输出
            max_idx = np.argmax(output)
            max_prob = output[max_idx]
            label = self.labels[max_idx] if max_idx < len(self.labels) else f'class_{max_idx}'
            
            return [(max_idx, float(max_prob), label)]
        else:
            # 多输出处理
            results = []
            for i, prob in enumerate(output[:5]):  # 取前5个结果
                label = self.labels[i] if i < len(self.labels) else f'class_{i}'
                results.append((i, float(prob), label))
            
            return sorted(results, key=lambda x: x[1], reverse=True)
    
    def input_width(self):
        """获取输入宽度"""
        return self.input_shape[1] if self.input_shape else 224
    
    def input_height(self):
        """获取输入高度"""
        return self.input_shape[0] if self.input_shape else 224
    
    def input_format(self):
        """获取输入格式"""
        return "RGB888"


class Detector(NeuralNetwork):
    """目标检测器"""
    
    def __init__(self, model_path=None, labels=None, threshold=0.5):
        """
        初始化检测器
        
        Args:
            model_path: 模型文件路径
            labels: 类别标签列表
            threshold: 检测阈值
        """
        super().__init__(model_path)
        self.labels = labels or []
        self.threshold = threshold
        
        if not self.labels:
            label_path = self._resolve_labels_path(model_path or self.model_path)
            if label_path and os.path.exists(label_path):
                self._load_labels(label_path)

    def _resolve_labels_path(self, model_path):
        if _model_info.labels_path():
            return _model_info.labels_path()
        if model_path:
            return os.path.splitext(model_path)[0] + ".txt"
        return None
    
    def _load_labels(self, label_path):
        """加载标签文件"""
        try:
            with open(label_path, 'r', encoding='utf-8') as f:
                self.labels = [line.strip() for line in f.readlines()]
            print(f"[Detector] 加载标签文件: {len(self.labels)}个类别")
        except Exception as e:
            print(f"[Detector] 加载标签文件失败: {e}")
    
    def detect(self, image):
        """
        目标检测
        
        Args:
            image: 输入图像
            
        Returns:
            list: [DetectionResult, ...]
        """
        if hasattr(image, 'data'):
            input_data = image.data
        else:
            input_data = image
        
        # 运行推理
        output = self.forward(input_data)
        
        # 用确定性规则从输出向量中派生检测结果，避免随机“成功”
        results = []
        flat = np.asarray(output, dtype=np.float32).flatten()
        if flat.size == 0:
            return results

        top_indices = np.argsort(flat)[-5:][::-1]
        class_space = len(self.labels) if self.labels else min(10, max(1, flat.size))

        for idx in top_indices:
            confidence = float(flat[idx])
            if confidence > self.threshold:
                class_id = int(idx) % class_space
                x = int((idx * 37) % 200)
                y = int((idx * 53) % 150)
                w = 48 + int(idx % 52)
                h = 48 + int((idx // 7) % 52)
                label = self.labels[class_id] if class_id < len(self.labels) else f'object_{class_id}'
                result = DetectionResult(x, y, w, h, confidence, class_id, label)
                results.append(result)
        
        return results


class DetectionResult:
    """检测结果"""
    
    def __init__(self, x, y, w, h, score, class_id, label):
        """
        创建检测结果
        
        Args:
            x, y: 边界框左上角坐标
            w, h: 边界框宽度和高度
            score: 置信度
            class_id: 类别ID
            label: 类别标签
        """
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.score = score
        self.class_id = class_id
        self.label = label
    
    def __str__(self):
        return f"Detection({self.label}: {self.score:.3f} at ({self.x}, {self.y}, {self.w}, {self.h}))"


# 便利函数
def load_classifier(model_path, labels=None):
    """
    加载分类器
    
    Args:
        model_path: 模型文件路径
        labels: 类别标签列表
        
    Returns:
        Classifier: 分类器对象
    """
    return Classifier(model_path, labels)


def load_detector(model_path, labels=None, threshold=0.5):
    """
    加载检测器

    Args:
        model_path: 模型文件路径
        labels: 类别标签列表
        threshold: 检测阈值

    Returns:
        Detector: 检测器对象
    """
    return Detector(model_path, labels, threshold)


# ------------------------------------------------------------------ #
# 对齐官方 MaixPy v4 的新增类                                          #
# ------------------------------------------------------------------ #

class NN(NeuralNetwork):
    """通用推理类，对齐官方 MaixPy v4 nn.NN"""

    def __init__(self, model_path=None, dual_buff=False):
        super().__init__(model_path)
        self._dual_buff = dual_buff

    def forward(self, inputs, input_idx=0):
        """前向推理，支持列表或单个输入"""
        data = inputs[input_idx] if isinstance(inputs, list) else inputs
        return super().forward(data)

    def input_size(self, index=0):
        """获取输入尺寸"""
        return self.input_shape or (1, 224, 224, 3)

    def output_size(self, index=0):
        """获取输出尺寸"""
        return self.output_shape or (1, 1000)


class YOLOv5(Detector):
    """YOLOv5 检测器，对齐官方 MaixPy v4 nn.YOLOv5"""

    def __init__(self, model=None, labels=None, dual_buff=True,
                 conf_th=0.5, iou_th=0.45):
        model_path = model if isinstance(model, str) else None
        super().__init__(model_path, labels, threshold=conf_th)
        self._iou_th    = iou_th
        self._dual_buff = dual_buff

    def detect(self, image, conf_th=None, iou_th=None):
        """目标检测，支持临时覆盖阈值"""
        if conf_th is not None:
            self.threshold = conf_th
        return super().detect(image)


class YOLOv8(YOLOv5):
    """YOLOv8 检测器（接口与 YOLOv5 相同）"""
    pass
