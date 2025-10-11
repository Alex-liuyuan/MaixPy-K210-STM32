# MaixPy-K210-STM32 API 参考手册

## 核心模块

### maix 模块

#### 平台信息

```python
import maix

# 获取当前平台
platform = maix.platform()  # 返回 'k210', 'stm32', 'linux'

# 获取版本信息
version = maix.version()     # 返回版本号字符串

# 便利延时函数
maix.delay(1000)            # 延时1000毫秒
maix.delay_us(1000)         # 延时1000微秒
```

#### 时间模块 (maix.time)

```python
from maix import time

# 时间戳获取
timestamp = time.time()      # 秒级时间戳
timestamp_ms = time.time_ms()    # 毫秒时间戳
timestamp_us = time.time_us()    # 微秒时间戳

# 系统滴答
ticks = time.ticks_ms()      # 系统滴答(毫秒)

# 延时函数
time.sleep(1.0)             # 延时1秒
time.sleep_ms(1000)         # 延时1000毫秒
time.sleep_us(1000)         # 延时1000微秒

# FPS计算
time.fps_start()            # 开始FPS计算
fps = time.fps()            # 获取FPS值
```

#### 应用模块 (maix.app)

```python
from maix import app

# 应用控制
while not app.need_exit():   # 检查是否需要退出
    # 应用逻辑
    pass

app.exit()                  # 退出应用
app_id = app.app_id()       # 获取应用ID
```

#### 系统模块 (maix.sys)

```python
from maix import sys

device_id = sys.device_id()  # 获取设备ID
platform = sys.platform()   # 获取平台信息
version = sys.version()      # 获取版本信息
```

### GPIO 模块

```python
from maix import GPIO

# GPIO 模式常量
GPIO.MODE_INPUT     # 输入模式
GPIO.MODE_OUTPUT    # 输出模式
GPIO.MODE_AF        # 复用模式
GPIO.MODE_ANALOG    # 模拟模式

# GPIO 状态常量
GPIO.LOW = 0        # 低电平
GPIO.HIGH = 1       # 高电平

# 创建GPIO对象
gpio = GPIO(pin, mode, pull=None)

# GPIO操作
gpio.value()        # 读取GPIO值
gpio.value(1)       # 设置GPIO值
gpio.on()           # 设置为高电平
gpio.off()          # 设置为低电平
gpio.toggle()       # 翻转电平
```

### 摄像头模块 (maix.camera)

```python
from maix import camera

# 创建摄像头对象
cam = camera.Camera(width=320, height=240, format="RGB888")

# 摄像头操作
img = cam.read()    # 读取一帧图像
cam.close()         # 关闭摄像头

# 支持格式
# "RGB565", "RGB888", "YUV422", "GRAY"
```

### 显示模块 (maix.display)

```python
from maix import display

# 创建显示器对象
disp = display.Display(width=320, height=240)

# 显示操作
disp.show(img)              # 显示图像
disp.clear((0, 0, 0))       # 清屏
disp.size()                 # 获取显示器尺寸
disp.set_backlight(80)      # 设置背光亮度
disp.set_rotation(90)       # 设置旋转角度
disp.close()                # 关闭显示器
```

### 图像模块 (maix.camera.Image)

```python
# 图像对象由摄像头返回
img = cam.read()

# 绘制操作
img.draw_rectangle(x, y, w, h, color=(255, 0, 0), thickness=1)
img.draw_circle(x, y, radius, color=(255, 0, 0), thickness=1)
img.draw_string(x, y, text, color=(255, 255, 255), scale=1)
img.draw_line(x1, y1, x2, y2, color=(255, 0, 0), thickness=1)

# 图像处理
new_img = img.resize(width, height)     # 缩放
cropped = img.crop(x, y, w, h)          # 裁剪
img_copy = img.copy()                   # 复制
img.save("image.jpg")                   # 保存

# 图像属性
img.width       # 图像宽度
img.height      # 图像高度
img.format      # 图像格式
img.data        # 图像数据

# 颜色常量
img.COLOR_RED    = (255, 0, 0)
img.COLOR_GREEN  = (0, 255, 0)
img.COLOR_BLUE   = (0, 0, 255)
img.COLOR_WHITE  = (255, 255, 255)
img.COLOR_BLACK  = (0, 0, 0)
```

### 神经网络模块 (maix.nn)

#### 基础神经网络类

```python
from maix import nn

# 创建神经网络对象
model = nn.NeuralNetwork(model_path)

# 模型操作
model.load(model_path)      # 加载模型
output = model.forward(input_data)  # 前向推理
model.unload()              # 卸载模型

# 模型属性
model.input_shape           # 输入形状
model.output_shape          # 输出形状
model.loaded               # 是否已加载
```

#### 图像分类器

```python
from maix import nn

# 创建分类器
classifier = nn.Classifier(model_path, labels)

# 或使用便利函数
classifier = nn.load_classifier(model_path, labels)

# 分类操作
results = classifier.classify(img)
# 返回: [(class_id, confidence, label), ...]

# 分类器属性
width = classifier.input_width()    # 输入宽度
height = classifier.input_height()  # 输入高度
format = classifier.input_format()  # 输入格式
```

#### 目标检测器

```python
from maix import nn

# 创建检测器
detector = nn.Detector(model_path, labels, threshold=0.5)

# 或使用便利函数
detector = nn.load_detector(model_path, labels, threshold=0.5)

# 检测操作
results = detector.detect(img)
# 返回: [DetectionResult, ...]

# 检测结果对象
for result in results:
    print(f"类别: {result.label}")
    print(f"置信度: {result.score}")
    print(f"位置: ({result.x}, {result.y}, {result.w}, {result.h})")
```

## 平台特定功能

### K210 平台

K210平台提供以下特殊功能：

- **KPU加速器**: 硬件AI推理加速
- **DVP摄像头**: 高性能图像采集
- **双核RISC-V**: 支持多线程处理

### STM32 平台

STM32平台提供以下特殊功能：

- **STM32Cube.AI**: STM32原生AI推理
- **DCMI摄像头**: 数字摄像头接口
- **丰富外设**: ADC、DAC、PWM等

## 错误处理

```python
try:
    cam = camera.Camera(320, 240)
    img = cam.read()
except Exception as e:
    print(f"错误: {e}")
finally:
    cam.close()
```

## 性能优化建议

1. **图像尺寸**: 根据实际需求选择合适的图像尺寸
2. **模型选择**: K210使用.kmodel格式，STM32使用.tflite格式
3. **内存管理**: 及时释放不需要的资源
4. **并发处理**: 在支持的平台上使用多线程

## 调试技巧

```python
# 开启调试模式
import maix
print(f"平台: {maix.platform()}")
print(f"版本: {maix.version()}")

# 性能监控
from maix import time
time.fps_start()
# ... 处理逻辑 ...
fps = time.fps()
print(f"FPS: {fps:.2f}")
```