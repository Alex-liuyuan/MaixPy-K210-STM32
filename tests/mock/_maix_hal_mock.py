"""
Linux模拟HAL层 - 实现与 _maix_hal.so 相同的接口
用于在Linux上运行单元测试，无需真实硬件
"""

import numpy as np
from collections import deque

__version__ = "0.1.0"
platform    = "stm32f407"

# ------------------------------------------------------------------ #
# GPIO                                                                 #
# ------------------------------------------------------------------ #
_gpio_state = {}

class GpioMode:
    INPUT  = 0
    OUTPUT = 1
    AF     = 2
    ANALOG = 3

class GpioPull:
    NONE = 0
    UP   = 1
    DOWN = 2

class GpioItMode:
    RISING         = 0
    FALLING        = 1
    RISING_FALLING = 2

def gpio_pin(port, pin_num):
    return (port << 16) | pin_num

def gpio_init(pin, mode=1, pull=0):
    _gpio_state[pin] = 0
    return 0

def gpio_deinit(pin):
    _gpio_state.pop(pin, None)
    return 0

def gpio_write(pin, state):
    _gpio_state[pin] = 1 if state else 0
    return 0

def gpio_read(pin):
    return _gpio_state.get(pin, 0)

def gpio_toggle(pin):
    _gpio_state[pin] = 1 - _gpio_state.get(pin, 0)
    return 0

def gpio_enable_irq(pin, mode, callback):
    return 0

def gpio_disable_irq(pin):
    return 0

# ------------------------------------------------------------------ #
# UART                                                                 #
# ------------------------------------------------------------------ #
_uart_rx_bufs = {}
_uart_handle_counter = [0x1000]

def uart_init(uart_id, baudrate=115200, wordlen=0, stopbits=0, parity=0):
    h = _uart_handle_counter[0]
    _uart_handle_counter[0] += 1
    _uart_rx_bufs[h] = deque()
    return h

def uart_deinit(handle):
    _uart_rx_bufs.pop(handle, None)
    return 0

def uart_write(handle, data, timeout=1000):
    return 0  # 模拟：丢弃发送数据

def uart_read(handle, size, timeout=1000):
    buf = _uart_rx_bufs.get(handle, deque())
    out = []
    while buf and len(out) < size:
        out.append(buf.popleft())
    return bytes(out)

def uart_read_dma(handle, max_len=256):
    return uart_read(handle, max_len)

def _uart_inject_rx(data):
    """测试辅助：向所有UART RX缓冲注入数据"""
    for buf in _uart_rx_bufs.values():
        for b in data:
            buf.append(b if isinstance(b, int) else ord(b))

# ------------------------------------------------------------------ #
# SPI                                                                  #
# ------------------------------------------------------------------ #
_spi_handle_counter = [0x2000]

def spi_init(spi_id, mode=0, baudrate=1000000, datasize=0, cpol=0, cpha=0):
    h = _spi_handle_counter[0]
    _spi_handle_counter[0] += 1
    return h

def spi_deinit(handle):
    return 0

def spi_write(handle, data, timeout=100):
    return 0

def spi_read(handle, size, timeout=100):
    return bytes(size)

def spi_transfer(handle, tx_data, timeout=100):
    return bytes(len(tx_data))

# ------------------------------------------------------------------ #
# I2C                                                                  #
# ------------------------------------------------------------------ #
_i2c_handle_counter = [0x3000]
_i2c_mem = {}  # {(handle, dev_addr, mem_addr): bytes}

def i2c_init(i2c_id, clock_speed=100000, addr_10bit=False):
    h = _i2c_handle_counter[0]
    _i2c_handle_counter[0] += 1
    return h

def i2c_deinit(handle):
    return 0

def i2c_write(handle, addr, data, timeout=100):
    return 0

def i2c_read(handle, addr, size, timeout=100):
    return bytes(size)

def i2c_mem_write(handle, dev_addr, mem_addr, mem_16bit, data, timeout=100):
    _i2c_mem[(handle, dev_addr, mem_addr)] = bytes(data)
    return 0

def i2c_mem_read(handle, dev_addr, mem_addr, mem_16bit, size, timeout=100):
    stored = _i2c_mem.get((handle, dev_addr, mem_addr), b'')
    return (stored + bytes(size))[:size]

# ------------------------------------------------------------------ #
# Camera                                                               #
# ------------------------------------------------------------------ #
_cam_open   = False
_cam_width  = 320
_cam_height = 240

def camera_open(width=320, height=240, format="RGB565"):
    global _cam_open, _cam_width, _cam_height
    _cam_width, _cam_height = width, height
    _cam_open = True
    return 0

def camera_close():
    global _cam_open
    _cam_open = False
    return 0

def camera_frame_ready():
    return _cam_open

def camera_read_frame():
    if not _cam_open:
        return None
    # 生成渐变测试图像 (H, W, 3) uint8
    h, w = _cam_height, _cam_width
    img = np.zeros((h, w, 3), dtype=np.uint8)
    img[:, :, 0] = np.linspace(0, 255, w, dtype=np.uint8)   # R渐变
    img[:, :, 1] = np.linspace(0, 255, h, dtype=np.uint8).reshape(h, 1)  # G渐变
    img[:, :, 2] = 128
    return img

def camera_read_frame_raw():
    img = camera_read_frame()
    if img is None:
        return None
    # RGB888 → RGB565
    r = (img[:, :, 0] >> 3).astype(np.uint16)
    g = (img[:, :, 1] >> 2).astype(np.uint16)
    b = (img[:, :, 2] >> 3).astype(np.uint16)
    rgb565 = (r << 11) | (g << 5) | b
    return rgb565.flatten().view(np.uint8)

def camera_width():  return _cam_width
def camera_height(): return _cam_height

# ------------------------------------------------------------------ #
# Display                                                              #
# ------------------------------------------------------------------ #
_disp_open   = False
_disp_width  = 240
_disp_height = 240
_disp_last_frame = None

def display_open(width=240, height=240):
    global _disp_open, _disp_width, _disp_height
    _disp_width, _disp_height = width, height
    _disp_open = True
    return 0

def display_close():
    global _disp_open
    _disp_open = False
    return 0

def display_show(img):
    global _disp_last_frame
    _disp_last_frame = np.array(img)
    return 0

def display_show_rgb565(img):
    global _disp_last_frame
    _disp_last_frame = np.array(img)
    return 0

def display_fill(r=0, g=0, b=0):
    global _disp_last_frame
    _disp_last_frame = np.full((_disp_height, _disp_width, 3),
                                [r, g, b], dtype=np.uint8)
    return 0

def display_width():  return _disp_width
def display_height(): return _disp_height

def _display_get_last_frame():
    """测试辅助：获取最后一帧"""
    return _disp_last_frame

# ------------------------------------------------------------------ #
# NN (TfliteRunner stub)                                               #
# ------------------------------------------------------------------ #
class TfliteRunner:
    """TFLite推理器模拟实现"""

    def __init__(self, arena_size=256*1024):
        self._loaded    = False
        self._in_shape  = []
        self._out_shape = []
        self._input     = None

    def load_file(self, path):
        # 模拟：接受任意路径，设置固定形状
        self._loaded    = True
        self._in_shape  = [1, 224, 224, 3]
        self._out_shape = [1, 1000]

    def load_bytes(self, data):
        self._loaded    = True
        self._in_shape  = [1, 224, 224, 3]
        self._out_shape = [1, 1000]

    def is_loaded(self):
        return self._loaded

    def input_count(self):  return 1 if self._loaded else 0
    def output_count(self): return 1 if self._loaded else 0

    def input_shape(self, index=0):
        return self._in_shape if self._loaded else []

    def output_shape(self, index=0):
        return self._out_shape if self._loaded else []

    def input_type(self, index=0):  return 0  # float32
    def output_type(self, index=0): return 0

    def run(self, input_arr, input_idx=0, output_idx=0):
        if not self._loaded:
            raise RuntimeError("模型未加载")
        self._input = np.asarray(input_arr, dtype=np.float32)
        n = 1
        for d in self._out_shape:
            n *= d
        window = self._input.flatten()[: min(self._input.size, 4096)]
        mean_val = float(window.mean()) if window.size else 0.0
        var_val = float(window.var()) if window.size else 0.0
        top_idx = int(abs(mean_val) * 1000 + var_val * 100) % max(n, 1)

        logits = np.full(n, -2.0, dtype=np.float32)
        logits[top_idx] = 3.0 + min(var_val, 5.0)
        logits -= logits.max()
        out = np.exp(logits)
        out /= out.sum()
        return out

    def run_uint8(self, input_arr, input_idx=0, output_idx=0):
        arr = np.asarray(input_arr, dtype=np.float32) / 255.0
        return self.run(arr)

    def last_invoke_ms(self):   return 1.0
    def output_scale(self, i=0):      return 1.0
    def output_zero_point(self, i=0): return 0

# ------------------------------------------------------------------ #
# PWM / ADC 子模块                                                     #
# ------------------------------------------------------------------ #
class _PwmModule:
    _state = {}  # {(timer_id, channel): duty_permille}

    def init(self, timer_id, channel=0, prescaler=83, period=999,
             pulse=500, polarity=0):
        self._state[(timer_id, channel)] = int(pulse * 1000 / max(1, period))
        return 0

    def deinit(self, timer_id, channel=0):
        self._state.pop((timer_id, channel), None)
        return 0

    def start(self, timer_id, channel=0): return 0
    def stop(self, timer_id, channel=0):  return 0

    def set_duty(self, timer_id, channel, duty_permille):
        self._state[(timer_id, channel)] = duty_permille
        return 0

    def get_duty(self, timer_id, channel):
        return self._state.get((timer_id, channel), 0)

pwm = _PwmModule()


class _AdcModule:
    def init(self, adc_id=0, scan_mode=False, continuous=False): return 0
    def deinit(self, adc_id=0): return 0

    def read(self, adc_id=0, channel=0):
        return (channel * 256 + adc_id * 17) % 4096

    def read_voltage(self, adc_id=0, channel=0, vref=3.3):
        return self.read(adc_id, channel) * vref / 4095.0

    def start_scan_dma(self, adc_id=0, channels=None): return 0

    def get_dma_result(self, adc_id=0, index=0):
        return (index * 256 + adc_id * 17) % 4096

adc = _AdcModule()
