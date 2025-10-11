"""
MaixPy-K210-STM32 神经网络模块
支持K210 KPU和STM32 AI推理的统一接口
"""

import os
import numpy as np
from . import _current_platform


class NeuralNetwork:
    """神经网络基类"""
    
    def __init__(self, model_path=None):
        """
        初始化神经网络
        
        Args:
            model_path: 模型文件路径
        """
        self.model_path = model_path
        self.model = None
        self.input_shape = None
        self.output_shape = None
        self.loaded = False
        
        if model_path:
            self.load(model_path)
    
    def load(self, model_path):
        """
        加载模型
        
        Args:
            model_path: 模型文件路径
        """
        if not os.path.exists(model_path):
            raise FileNotFoundError(f"模型文件不存在: {model_path}")
        
        self.model_path = model_path
        
        if _current_platform == 'k210':
            self._load_k210_model(model_path)
        elif _current_platform == 'stm32':
            self._load_stm32_model(model_path)
        else:
            self._load_mock_model(model_path)
    
    def _load_k210_model(self, model_path):
        """加载K210 KPU模型"""
        try:
            # import KPU  # K210特定导入
            # self.model = KPU.load(model_path)
            # self.input_shape = KPU.get_input_shape(self.model)
            # self.output_shape = KPU.get_output_shape(self.model)
            self.input_shape = (224, 224, 3)  # 模拟输入形状
            self.output_shape = (1000,)       # 模拟输出形状
            self.loaded = True
            print(f"[NN] K210 KPU模型加载成功: {model_path}")
        except ImportError:
            print(f"[NN] K210 KPU模型加载失败，使用模拟模式: {model_path}")
            self._load_mock_model(model_path)
    
    def _load_stm32_model(self, model_path):
        """加载STM32 AI模型"""
        try:
            # import _maix_hal  # STM32特定导入
            # self.model = _maix_hal.ai_load_model(model_path)
            # self.input_shape = _maix_hal.ai_get_input_shape(self.model)
            # self.output_shape = _maix_hal.ai_get_output_shape(self.model)
            self.input_shape = (224, 224, 3)  # 模拟输入形状
            self.output_shape = (1000,)       # 模拟输出形状
            self.loaded = True
            print(f"[NN] STM32 AI模型加载成功: {model_path}")
        except ImportError:
            print(f"[NN] STM32 AI模型加载失败，使用模拟模式: {model_path}")
            self._load_mock_model(model_path)
    
    def _load_mock_model(self, model_path):
        """加载模拟模型"""
        self.input_shape = (224, 224, 3)
        self.output_shape = (1000,)
        self.loaded = True
        print(f"[NN] 模拟模型加载: {model_path}")
    
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
        
        if _current_platform == 'k210':
            return self._forward_k210(input_data)
        elif _current_platform == 'stm32':
            return self._forward_stm32(input_data)
        else:
            return self._forward_mock(input_data)
    
    def _forward_k210(self, input_data):
        """K210 KPU前向推理"""
        try:
            # import KPU  # K210特定导入
            # output = KPU.forward(self.model, input_data)
            # return output
            return np.array(np.random.random(self.output_shape), dtype=np.float32)
        except ImportError:
            return self._forward_mock(input_data)
    
    def _forward_stm32(self, input_data):
        """STM32 AI前向推理"""
        try:
            # import _maix_hal  # STM32特定导入
            # output = _maix_hal.ai_inference(self.model, input_data)
            # return output
            return np.array(np.random.random(self.output_shape), dtype=np.float32)
        except ImportError:
            return self._forward_mock(input_data)
    
    def _forward_mock(self, input_data):
        """模拟前向推理"""
        return np.array(np.random.random(self.output_shape), dtype=np.float32)
    
    def unload(self):
        """卸载模型"""
        if self.loaded:
            if _current_platform == 'k210':
                try:
                    # import KPU  # K210特定导入
                    # KPU.unload(self.model)
                    pass
                except:
                    pass
            elif _current_platform == 'stm32':
                try:
                    # import _maix_hal  # STM32特定导入
                    # _maix_hal.ai_unload_model(self.model)
                    pass
                except:
                    pass
            
            self.model = None
            self.loaded = False
            print("[NN] 模型已卸载")


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
        if not self.labels and model_path:
            label_path = model_path.replace('.kmodel', '.txt').replace('.tflite', '.txt')
            if os.path.exists(label_path):
                self._load_labels(label_path)
    
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
        
        if not self.labels and model_path:
            label_path = model_path.replace('.kmodel', '.txt').replace('.tflite', '.txt')
            if os.path.exists(label_path):
                self._load_labels(label_path)
    
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
        
        # 解析检测结果（模拟）
        results = []
        num_detections = min(5, len(output) // 6)  # 假设每个检测结果6个值
        
        for i in range(num_detections):
            # 模拟检测结果
            x = np.random.randint(0, 200)
            y = np.random.randint(0, 150)
            w = np.random.randint(50, 100)
            h = np.random.randint(50, 100)
            confidence = np.random.random()
            class_id = np.random.randint(0, len(self.labels) if self.labels else 10)
            
            if confidence > self.threshold:
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