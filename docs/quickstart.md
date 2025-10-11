# MaixPy-K210-STM32 快速入门指南

## 概述

MaixPy-K210-STM32 是基于 MaixPy v4 架构的跨平台开发框架，提供对 K210 和 STM32 开发板的统一 Python API 支持。

## 支持的硬件平台

### K210 系列
- Sipeed Maix-I Dock
- Sipeed Maix-I Go  
- Sipeed Maix-I Cube
- Sipeed Maix-I BiT
- Sipeed Maix-I Nano

### STM32 系列
- STM32F407 Discovery
- STM32F767 Nucleo
- STM32H743 Nucleo
- 自定义 STM32 开发板

## 环境准备

### 基础环境

```bash
# 安装Python依赖
pip install -r requirements.txt

# 安装构建工具
pip install cmake
```

### K210 开发环境

```bash
# 安装K210烧录工具
pip install kflash

# 下载K210工具链
# 参考: https://github.com/kendryte/kendryte-gnu-toolchain
```

### STM32 开发环境

```bash
# 安装STM32CubeIDE或使用ARM工具链
# 下载地址: https://www.st.com/en/development-tools/stm32cubeide.html

# 或使用ARM GCC工具链
sudo apt-get install gcc-arm-none-eabi
```

## 编译和烧录

### K210 平台

```bash
# 配置K210平台
python project.py menuconfig -p k210

# 编译固件
python project.py build -p k210

# 烧录到开发板
python project.py flash -p k210 -d /dev/ttyUSB0
```

### STM32 平台

```bash
# 配置STM32F407平台
python project.py menuconfig -p stm32f407

# 编译固件
python project.py build -p stm32f407

# 烧录到开发板
python project.py flash -p stm32f407 -d COM3
```

## 第一个程序

### 基础示例

```python
from maix import camera, display, app, time

# 初始化摄像头和显示器
cam = camera.Camera(320, 240)
disp = display.Display()

while not app.need_exit():
    # 获取图像
    img = cam.read()
    
    # 显示图像
    disp.show(img)
    
    # 计算FPS
    fps = time.fps()
    print(f"FPS: {fps:.2f}")
```

### GPIO 控制

```python
from maix import GPIO, time

# 初始化LED
led = GPIO(0, GPIO.MODE_OUTPUT)

# 闪烁LED
while True:
    led.on()
    time.sleep_ms(500)
    led.off()
    time.sleep_ms(500)
```

### AI 图像分类

```python
from maix import camera, display, nn

# 加载分类模型
classifier = nn.Classifier("/flash/mobilenet.kmodel")

# 初始化摄像头
cam = camera.Camera(224, 224)
disp = display.Display()

while True:
    img = cam.read()
    
    # 运行分类
    results = classifier.classify(img)
    
    # 显示结果
    if results:
        label = results[0][2]
        confidence = results[0][1]
        img.draw_string(10, 10, f"{label}: {confidence:.3f}")
    
    disp.show(img)
```

## 常见问题

### Q: 如何切换平台？
A: 使用 `python project.py menuconfig -p <platform>` 重新配置目标平台。

### Q: 烧录失败怎么办？
A: 检查串口权限、波特率设置和开发板连接。

### Q: 模型加载失败？
A: 确认模型格式是否与平台匹配（K210用.kmodel，STM32用.tflite）。

### Q: 如何添加新的硬件支持？
A: 参考 `components/drivers/` 目录下的平台实现，创建新的HAL驱动。

## 更多资源

- [API 参考手册](api/index.md)
- [硬件适配指南](hardware_porting.md)
- [示例代码](../examples/)
- [常见问题解答](faq.md)