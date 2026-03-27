# API 参考手册

## maix 核心模块

```python
from maix import time, app, model, sys, GPIO, platform, version, delay
```

### 版本与平台

| 函数/属性 | 说明 |
|-----------|------|
| `version()` | 返回版本字符串，如 `"1.0.0"` |
| `platform()` | 返回平台名：`"stm32"` / `"linux"` |
| `sys.device_id()` | 设备唯一ID |
| `sys.platform()` | 同 `platform()` |

### 时间 (maix.time)

| 方法 | 说明 |
|------|------|
| `time.ticks_ms()` | 系统滴答（毫秒） |
| `time.ticks_us()` | 系统滴答（微秒） |
| `time.ticks_s()` | 系统滴答（秒） |
| `time.ticks_diff(start)` | 计算经过毫秒数，处理32bit溢出 |
| `time.sleep(s)` | 秒延时 |
| `time.sleep_ms(ms)` | 毫秒延时 |
| `time.sleep_us(us)` | 微秒延时 |
| `time.localtime()` | 本地时间 (datetime) |
| `time.timezone()` | 时区偏移（秒） |
| `time.fps()` | 计算FPS |

### 应用控制 (maix.app)

| 方法 | 说明 |
|------|------|
| `app.need_exit()` | 是否需要退出 |
| `app.exit()` | 设置退出标志 |
| `app.app_id()` | 应用ID |

### GPIO

```python
from maix import GPIO

led = GPIO(pin, GPIO.Mode.OUT)  # 新枚举
led = GPIO(pin, GPIO.MODE_OUTPUT)  # 旧常量兼容

led.on()
led.off()
led.toggle()
val = led.value()      # 读取
led.value(1)           # 写入
```

## maix.err — 错误处理

```python
from maix.err import check_raise, check_bool, HardwareError, ERR_NONE

ret = check_raise(hal_func(), "操作失败")  # 非零抛 HardwareError
ok = check_bool(hal_func())                # 返回 True/False
```

## maix.pinmap — 引脚复用

```python
from maix import pinmap

pinmap.set_pin_function("PA9", "USART1_TX")
func = pinmap.get_pin_function("PA9")
caps = pinmap.get_pin_capabilities("PA9")  # ['GPIO', 'USART1_TX', ...]
info = pinmap.get_pins_info()
```

## maix.camera — 摄像头与图像

### Camera

```python
from maix.camera import Camera

cam = Camera(320, 240, "RGB888")
img = cam.read()
cam.close()
```

### Image

```python
img.draw_rectangle(x, y, w, h, color=(255,0,0), thickness=1)
img.draw_circle(x, y, radius, color=(255,0,0), thickness=1)
img.draw_line(x1, y1, x2, y2, color=(255,0,0), thickness=1)
img.draw_string(x, y, "text", color=(255,255,255), scale=1)

resized = img.resize(160, 120)
cropped = img.crop(x, y, w, h)
copy = img.copy()
data = img.to_bytes()

blobs = img.find_blobs([(r_min, r_max, g_min, g_max, b_min, b_max)],
                        area_threshold=10)
for b in blobs:
    print(b.x, b.y, b.w, b.h, b.pixels, b.cx, b.cy)
```

## maix.display — 显示

```python
from maix.display import Display, create_display

d = Display(320, 240)
d.show(img)
d.clear(color=(0,0,0))
w, h = d.size()
d.set_backlight(80)
d.set_rotation(90)
d.close()
```

## maix.nn — 神经网络

```python
from maix import nn

# 通用推理
model = nn.NN("model.tflite")
out = model.forward(input_data)

# 分类
classifier = nn.Classifier(model_path, labels)
results = classifier.classify(img)

# 检测
detector = nn.Detector(model_path, labels, threshold=0.5)
results = detector.detect(img)

# YOLOv5/v8
det = nn.YOLOv5("yolo.tflite", labels, conf_th=0.5, iou_th=0.45)
det = nn.YOLOv8("yolo.tflite", labels)
results = det.detect(img)
```

## maix.uart — 串口

```python
from maix.uart import UART

u = UART(uart_id=1, baudrate=115200)
u.write(b"hello")
data = u.read(10)
u.set_received_callback(lambda s, data: print(data))
u.close()
```

## maix.spi — SPI

```python
from maix.spi import SPI

s = SPI(spi_id=1, baudrate=1_000_000)
s.write(b"\x01\x02")
rx = s.read(4)
rx = s.transfer(b"\xAA\xBB")
s.close()
```

## maix.i2c — I2C

```python
from maix.i2c import I2C

bus = I2C(i2c_id=1, clock_speed=400_000)
bus.write(0x50, b"\x00\x10")
data = bus.read(0x50, 4)
bus.mem_write(0x50, 0x00, b"\xFF")
data = bus.mem_read(0x50, 0x00, 2)
bus.close()
```

## maix.adc — ADC

```python
from maix.adc import ADC, CH0, CH1, RES_BIT_12

a = ADC(adc_id=1, resolution=RES_BIT_12, vref=3.3)
raw = a.read(CH0)           # 原始值 0-4095
vol = a.read_vol(CH0)       # 电压值（V）
vol = a.read_voltage(CH0)   # 别名
results = a.read_multi([CH0, CH1])  # 多通道DMA
a.close()
```

通道常量：`CH0`-`CH15`, `CH_TEMP`, `CH_VREFINT`, `CH_VBAT`

## maix.pwm — PWM

```python
from maix.pwm import PWM

# 通过 pwm_id（自动映射 timer/channel）
p = PWM(pwm_id=1, freq=1000, duty=50.0, enable=True)

# 通过 timer/channel
p = PWM(timer=2, channel=1, freq=1000, duty=50.0)

p.duty(75.0)
p.start()
p.stop()
p.close()
```

## maix.filter — 数字滤波器

```python
from maix.filter import (MovingAverage, MedianFilter, LowPassFilter,
                          KalmanFilter1D, DeadZoneFilter, LimitFilter)

ma = MovingAverage(window=5)
filtered = ma.update(raw_value)
ma.reset()
```

| 滤波器 | 参数 | 适用场景 |
|--------|------|----------|
| `MovingAverage(window)` | 窗口大小 | 平滑噪声 |
| `MedianFilter(window)` | 窗口大小 | 去除尖峰 |
| `LowPassFilter(alpha)` | 平滑系数 0-1 | 高频噪声 |
| `KalmanFilter1D(q, r)` | 过程/测量噪声 | 高斯噪声 |
| `DeadZoneFilter(threshold)` | 死区阈值 | 抖动抑制 |
| `LimitFilter(max_delta)` | 最大变化量 | 突变过滤 |

## maix.audio — 音频采集与播放

### Audio

```python
from maix.audio import Audio, AudioFrame

a = Audio(sample_rate=16000, channels=1, format="PCM_S16", frame_size=512)
data = a.read()           # 返回 int16 numpy 数组，长度 = frame_size
data = a.read(samples=1024)  # 指定采样数
a.write(data)
a.start()
a.stop()
a.set_volume(80)
vol = a.get_volume()
a.close()
```

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `sample_rate` | 16000 | 采样率 (8000/16000/44100/48000) |
| `channels` | 1 | 声道数 (1=mono, 2=stereo) |
| `format` | `"PCM_S16"` | 音频格式 (`"PCM_S16"`, `"PCM_S32"`, `"PDM"`) |
| `frame_size` | 512 | 每帧采样数 |

### AudioFrame

```python
frame = AudioFrame(data, sample_rate=16000, channels=1)
ms = frame.duration_ms       # 帧时长（毫秒）
b = frame.to_bytes()         # 转为 bytes
f = frame.to_float()         # 归一化到 [-1.0, 1.0]
```

## maix.audio_feature — 音频特征提取

纯 numpy 实现，零外部依赖。

```python
from maix.audio_feature import (
    compute_mfcc, compute_mel_spectrogram, compute_spectrogram,
    compute_energy, pre_emphasis,
)
import numpy as np

signal = np.zeros(16000, dtype=np.float64)

# MFCC
mfcc = compute_mfcc(signal, sample_rate=16000, n_mfcc=13,
                     n_fft=512, hop_length=160, n_mels=40)
# → (n_frames, 13)

# Mel 频谱图
mel = compute_mel_spectrogram(signal, sample_rate=16000,
                               n_fft=512, hop_length=160, n_mels=40)
# → (n_frames, 40) 对数能量

# 短时傅里叶变换频谱图
spec = compute_spectrogram(signal, n_fft=512, hop_length=160)
# → (n_frames, 257) 幅度谱

# 帧能量
energy = compute_energy(signal, frame_size=512, hop_length=160)
# → (n_frames,)

# 预加重
emphasized = pre_emphasis(signal, coeff=0.97)
```

## maix.nn — 音频 AI 推理类

### SpeechKWS（语音关键词识别）

```python
from maix.nn import SpeechKWS

kws = SpeechKWS(keywords=["yes", "no", "stop", "go"], threshold=0.8,
                sample_rate=16000, frame_duration_ms=1000)
results = kws.recognize(audio_data)
# → [("yes", 0.92), ("no", 0.05), ...]
```

### AudioClassifier（音频事件分类）

```python
from maix.nn import AudioClassifier

clf = AudioClassifier(labels=["speech", "music", "noise"], threshold=0.5,
                      sample_rate=16000, clip_duration_ms=1000)
results = clf.classify(audio_data)
# → [(0, 0.85, "speech"), (2, 0.10, "noise"), ...]
```

### VAD（语音活动检测）

```python
from maix.nn import VAD

vad = VAD(threshold=0.5, sample_rate=16000, frame_duration_ms=30)
is_sp = vad.is_speech(audio_frame)   # → bool
segments = vad.process(audio_data)
# → [{"start_ms": 0, "end_ms": 300, "confidence": 0.9}, ...]
```
