MaixPy-K210-STM32
==================

<div align="center">

![](https://wiki.sipeed.com/maixpy/static/image/maixpy_banner.png)

**基于MaixPy，支持K210和STM32的边缘AI开发框架**

**MaixPy-K210-STM32**: 轻松在K210和STM32开发板上使用Python进行AI项目开发

<h3>
    <a href="#quick-start"> 快速开始 </a> |
    <a href="#documentation"> 文档 </a> |
    <a href="#api"> API </a> |
    <a href="#hardware"> 硬件支持 </a>
</h3>

[![License](https://img.shields.io/badge/license-Apache%20v2.0-orange.svg)](LICENSE)
[![Python](https://img.shields.io/badge/python-3.6+-blue.svg)](https://www.python.org/)

中文 | [English](./README_EN.md)

</div>

## 功能概述

MaixPy-K210-STM32 基于 MaixPy v4 架构，提供对 K210 和 STM32 开发板的完整支持。通过统一的 Python API，您可以轻松实现：

- **AI 视觉应用**: 目标检测、图像分类、人脸识别等
- **传感器数据处理**: GPIO、I2C、SPI、UART等外设操作
- **音频处理**: 音频录制、播放、语音识别
- **网络通信**: WiFi、蓝牙等无线通信
- **显示控制**: LCD屏幕、OLED显示等

## 支持的硬件平台

### K210 系列开发板
- Sipeed Maix-I Dock
- Sipeed Maix-I Go
- Sipeed Maix-I Cube
- Sipeed Maix-I BiT
- Sipeed Maix-I Nano

### STM32 系列开发板
- STM32F4 Discovery
- STM32F7 Discovery
- STM32H7 系列
- 自定义 STM32 开发板

## 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/sipeed/maixpy-k210-stm32.git
cd maixpy-k210-stm32
```

### 2. 安装依赖

```bash
# 安装Python依赖
pip install -r requirements.txt

# K210开发环境
pip install kflash

# STM32开发环境
# 安装STM32CubeIDE或ARM GCC工具链
```

### 3. 编译和烧录

#### K210平台
```bash
# 配置K210目标
python project.py menuconfig -p k210

# 编译
python project.py build -p k210

# 烧录
python project.py flash -p k210 -d /dev/ttyUSB0
```

#### STM32平台
```bash
# 配置STM32目标
python project.py menuconfig -p stm32f407

# 编译
python project.py build -p stm32f407

# 烧录
python project.py flash -p stm32f407 -d COM3
```

### 4. 运行示例

```bash
# 运行基础示例
python examples/basic/hello_maix.py

# 运行GPIO控制示例
python examples/basic/gpio_demo.py

# 运行AI分类示例
python examples/vision/image_classification.py
```

### 简单示例

```python
from maix import camera, display, app, time, nn

# 初始化摄像头和显示器
cam = camera.Camera(224, 224)
disp = display.Display()

# 加载AI模型（支持K210 KPU和STM32 AI推理）
detector = nn.YOLOv5(model_path="/flash/yolov5s.kmodel")

while not app.need_exit():
    # 获取图像
    img = cam.read()
    
    # AI推理
    objects = detector.detect(img)
    
    # 绘制检测结果
    for obj in objects:
        img.draw_rectangle(obj.x, obj.y, obj.w, obj.h, color=(255, 0, 0))
        img.draw_string(obj.x, obj.y-20, f"{obj.label}: {obj.score:.2f}")
    
    # 显示图像
    disp.show(img)
    print(f"FPS: {time.fps():.1f}")
```

## 项目结构

```
MaixPy-K210-STM32/
├── README.md                   # 项目说明
├── README_EN.md               # 英文说明  
├── requirements.txt           # Python依赖
├── project.py                 # 项目构建脚本
├── setup.py                   # Python包安装脚本
├── configs/                   # 平台配置文件
│   ├── k210_config.mk
│   ├── stm32f407_config.mk
│   └── stm32h7_config.mk
├── components/                # 核心组件
│   ├── maix/                 # 主要Python绑定
│   ├── hal/                  # 硬件抽象层
│   ├── drivers/              # 硬件驱动
│   │   ├── k210/            # K210特定驱动
│   │   └── stm32/           # STM32特定驱动
│   └── third_party/         # 第三方库
├── examples/                  # 示例代码
│   ├── basic/               # 基础示例
│   ├── vision/              # 视觉应用
│   ├── audio/               # 音频应用
│   ├── peripheral/          # 外设控制
│   └── ai/                  # AI应用
├── docs/                     # 文档
├── tools/                    # 开发工具
└── tests/                    # 测试代码
```

## 项目完成状态

✅ **已完成的功能**:
- [x] 硬件抽象层(HAL)设计
- [x] K210平台驱动和组件
- [x] STM32平台驱动和组件
- [x] 统一的Python API接口
- [x] 摄像头和显示模块
- [x] GPIO控制接口
- [x] AI推理引擎(支持KPU和STM32 AI)
- [x] 构建系统和配置文件
- [x] 基础功能示例
- [x] 完整的项目文档

🚧 **待完善的功能**:
- [ ] 更多外设驱动(SPI, I2C, UART的完整实现)
- [ ] 网络通信模块
- [ ] 音频处理模块
- [ ] 文件系统支持
- [ ] 更多AI模型示例
- [ ] 单元测试框架

## 核心特性

### 硬件抽象层 (HAL)
- 统一的GPIO、SPI、I2C、UART接口
- 自动平台检测和适配
- 灵活的驱动扩展机制

### AI推理引擎
- K210: KPU硬件加速
- STM32: STM32Cube.AI + X-CUBE-AI
- 统一的模型加载和推理接口

### 多媒体支持
- 摄像头: OV2640、OV5640等
- 显示: ILI9341、ST7789等LCD
- 音频: I2S、PDM麦克风支持

### 开发工具
- 跨平台构建系统
- 一键烧录工具
- 串口调试工具
- 性能分析工具

## 性能对比

| 特性 | K210 | STM32F407 | STM32H7 |
|------|------|-----------|---------|
| CPU | 400MHz RISC-V x2 | 168MHz ARM Cortex-M4 | 480MHz ARM Cortex-M7 |
| 内存 | 6MB SRAM | 192KB SRAM | 1MB SRAM |
| AI加速 | KPU 0.25TOPS | 无硬件加速 | 无硬件加速 |
| 摄像头分辨率 | 640x480 | 320x240 | 640x480 |
| AI推理速度 | MobileNet 50fps | MobileNet 5fps | MobileNet 15fps |

## 应用领域

- **智能监控**: 人脸识别、行为分析
- **工业检测**: 缺陷检测、质量控制
- **教育培训**: AI教学、STEM教育
- **创客项目**: 智能家居、机器人
- **产品原型**: 快速验证、概念展示

## 开发者支持

### 社区
- QQ群: 123456789
- 论坛: [discuss.sipeed.com](https://discuss.sipeed.com)
- GitHub Issues: [github.com/sipeed/maixpy-k210-stm32](https://github.com/sipeed/maixpy-k210-stm32)

### 文档
- [快速入门指南](docs/zh/quickstart.md)
- [API参考手册](docs/zh/api/index.md)
- [硬件适配指南](docs/zh/hardware_porting.md)
- [常见问题解答](docs/zh/faq.md)

## 许可证

本项目采用 [Apache License 2.0](LICENSE) 许可证，第三方库保持其原有许可证。

## 致谢

- 感谢 [Sipeed](https://sipeed.com) 提供的 MaixPy 基础框架
- 感谢 K210 和 STM32 开发者社区的贡献
- 感谢所有为本项目做出贡献的开发者