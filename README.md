# SYSU_AIOTOS

跨架构边缘 AI 开发框架，面向 ARM Cortex-M 与 RISC-V，统一 RT-Thread Nano + MicroPython + Python API。

当前参考板：STM32F407 Nucleo（主线）、K210 Generic（实验）、QEMU olimex-stm32-h405（仿真）。

## 架构

```
┌─────────────────────────────────────────────────────┐
│  SYSU Studio (Web IDE)                               │
│  浏览器 → localhost:8210                              │
│  代码编辑 · 构建烧录 · 串口监控 · 模拟运行            │
├─────────────────────────────────────────────────────┤
│  Python 应用层                                       │
│  from sysu import GPIO, time, nn                    │
│  from sysu.camera import Camera                     │
│  from sysu.display import Display                   │
├─────────────────────────────────────────────────────┤
│  sysu Python 包                                      │
│  camera · display · nn · audio · audio_feature      │
│  gpio · spi · i2c · uart · pwm · adc               │
│  filter · pinmap · err                              │
├─────────────────────────────────────────────────────┤
│  _maix_hal (MicroPython C 绑定)                      │
│  modmaix_hal.c — 37 个函数绑定 + pwm/adc 子模块      │
├─────────────────────────────────────────────────────┤
│  C HAL 抽象层 (ops 注册模式)                          │
│  hal_gpio · hal_spi · hal_i2c · hal_uart            │
│  hal_pwm · hal_adc · hal_camera · hal_display       │
│  hal_audio                                          │
├──────────────────────┬──────────────────────────────┤
│  STM32 驱动           │  K210 驱动                    │
│  GPIO/SPI/I2C/UART   │  FPIOA+GPIOHS/SPI/I2C/UART  │
│  PWM/ADC/DCMI/LCD    │                              │
├──────────────────────┴──────────────────────────────┤
│  RT-Thread Nano                                      │
├──────────────────────┬──────────────────────────────┤
│  ARM Cortex-M        │  RISC-V                       │
└──────────────────────┴──────────────────────────────┘
```

## 当前状态

| 平台 | 状态 | 说明 |
|------|------|------|
| STM32F407 (rtthread) | 可用 | ARM 主线参考板，Python→C HAL 全链路打通 |
| QEMU (sim) | 可用 | 仿真验证，semihosting 模式 |
| K210 (k210) | 实验 | RISC-V 参考板，SDK 探测 + 实验性构建 |

已完成：
- 241 个 pytest 测试全部通过
- STM32 全外设 HAL ops 注册（GPIO/SPI/I2C/UART/PWM/ADC/Camera/Display/Audio）
- MicroPython `_maix_hal` 内建模块（37 个函数绑定）
- K210 驱动真实实现（FPIOA+GPIOHS/SPI/I2C/UART SDK）
- 统一 CLI 入口：构建、烧录、监视、仿真

未完成：
- 可写 VFS / 外部存储挂载
- 板上模型文件加载与 AI 推理闭环
- K210 实板稳定回归

## 快速开始

```bash
# 初始化子模块和依赖
git submodule update --init --recursive
pip install -r requirements.txt

# 主机侧验证（Linux 模拟模式）
pip install -e .
python3 -c "from sysu import *; print(version())"  # 1.0.0

# 运行测试
python3 -m pytest tests/ -v
```

### 构建固件

```bash
# STM32 真板
python3 project.py build -p rtthread
python3 project.py flash -p rtthread
python3 project.py monitor -p rtthread -d /dev/ttyACM0

# QEMU 仿真
python3 project.py build -p sim
python3 project.py monitor -p sim

# K210 实验
python3 project.py build -p k210
python3 project.py flash -p k210 -d /dev/ttyUSB0
```

### SYSU Studio (Web IDE)

```bash
# 启动 Web IDE（默认 http://localhost:8210）
python3 project.py studio

# 自定义端口
python3 project.py studio --studio-port 9000

# 或直接通过模块启动
python3 -m sysu.studio --port 8210
```

浏览器打开后即可：编写 Python 代码、选择板卡/串口、一键构建烧录、模拟运行、串口监控。

### 工具链

工具链需自行获取：
- STM32：`apt install gcc-arm-none-eabi` 或从 ARM 官网下载
- K210：Kendryte RISC-V 工具链，解压到 `third_party/toolchains/kendryte-toolchain/`

## Python API 示例

```python
from sysu import GPIO, time
from sysu.camera import Camera
from sysu.display import Display

# GPIO
led = GPIO(0x00000005, GPIO.Mode.OUT)
led.on()
time.sleep_ms(500)
led.off()

# 摄像头 + 显示
cam = Camera(320, 240, "RGB888")
disp = Display(320, 240)
img = cam.read()
img.draw_string(10, 10, "Hello MaixPy", color=(255, 255, 255))
disp.show(img)
cam.close()
disp.close()
```

```python
from sysu.spi import SPI
from sysu.i2c import I2C
from sysu.uart import UART
from sysu.adc import ADC, CH0
from sysu.pwm import PWM

# SPI
s = SPI(spi_id=1, baudrate=1_000_000)
s.write(b"\x01\x02")
rx = s.transfer(b"\xAA\xBB")
s.close()

# I2C
bus = I2C(i2c_id=1, clock_speed=400_000)
bus.mem_write(0x50, 0x00, b"\xFF")
data = bus.mem_read(0x50, 0x00, 2)
bus.close()

# UART
u = UART(uart_id=1, baudrate=115200)
u.write(b"hello")
u.close()

# ADC
a = ADC(adc_id=1, vref=3.3)
voltage = a.read_vol(CH0)
a.close()

# PWM
p = PWM(pwm_id=1, freq=1000, duty=50.0, enable=True)
p.duty(75.0)
p.close()
```

```python
from sysu.audio import Audio
from sysu.audio_feature import compute_mfcc
from sysu.nn import SpeechKWS, VAD
import numpy as np

# 音频采集
a = Audio(sample_rate=16000, channels=1)
data = a.read(16000)  # 读取 1 秒音频
a.close()

# 特征提取
mfcc = compute_mfcc(data, sample_rate=16000, n_mfcc=13)

# 语音关键词识别
kws = SpeechKWS(keywords=["yes", "no", "stop", "go"])
results = kws.recognize(data)
print(results)  # [("yes", 0.92), ("no", 0.05), ...]

# 语音活动检测
vad = VAD(threshold=0.5, frame_duration_ms=30)
segments = vad.process(data)
print(segments)  # [{"start_ms": 0, "end_ms": 300, "confidence": 0.9}, ...]
```

## 目录结构

```
MaixPy-K210-STM32/
├── sysu/                          Python 包（核心 API）
│   └── studio/                    Web IDE（FastAPI 后端 + 静态前端）
├── components/
│   ├── hal/                       C HAL 抽象层
│   ├── drivers/
│   │   ├── stm32/                 STM32 平台驱动
│   │   └── k210/                  K210 平台驱动
│   └── micropython_rtthread/      MicroPython RT-Thread 端口 + _maix_hal 绑定
├── runtime/                       固件运行时（启动、文件系统、AI 后端）
├── runtime_bundle/                运行时资源包
├── platforms/
│   ├── stm32/                     STM32 链接脚本、启动文件
│   ├── k210/                      K210 板级代码
│   └── sim/                       QEMU 仿真
├── cmake/                         CMake 工具链和模块
├── tools/                         烧录、监控等工具脚本
├── boards/                        板卡 JSON 配置
├── tests/                         pytest 测试（241 个）
├── reference/
│   ├── docs/                      文档（架构、API、移植指南）
│   └── examples/                  示例代码
├── third_party/                   RT-Thread Nano、TFLite Micro、MaixPy-v1
├── project.py                     统一构建入口
├── setup.py / pyproject.toml      Python 打包
└── requirements.txt               Python 依赖
```

## 设计原则

1. 一次构建只允许一个目标架构和一个板卡
2. OS 主线固定为 RT-Thread Nano
3. Python API 不暴露芯片私有语义，板级差异留在底层驱动
4. 所有"已支持"能力必须可验证，不伪造成功闭环
5. 新板卡优先扩展 `boards/` 和板级驱动，不另开独立目录

## 文档

- [架构设计](reference/docs/architecture.md)
- [快速开始](reference/docs/quickstart.md)
- [API 参考](reference/docs/api_reference.md)
- [项目结构](reference/docs/project_layout.md)
- [硬件移植](reference/docs/hardware_porting.md)
- [板卡配置](reference/docs/board_schema.md)
- [FAQ](reference/docs/faq.md)

## 许可证

Apache 2.0
