"""
MaixPy Nano RT-Thread 核心模块
"""

import sys
import os
import time as _time

# 版本信息
__version__ = "0.1.0"

def _normalize_platform_name(name):
    """将平台名归一化为上层 API 使用的类别。"""
    if not name:
        return None

    value = str(name).strip().lower()
    if value in ("linux", "stm32"):
        return value
    if value == "rtthread" or value.startswith("stm32"):
        return "stm32"
    return None

# 平台检测
def _detect_platform():
    """自动检测当前运行平台

    优先级：
    1. MAIX_PLATFORM 环境变量
    2. PLATFORM=rtthread 兼容
    3. _maix_hal.platform 属性
    4. Cortex-M machine 字符串
    5. 默认 'linux'
    """
    import platform as _plat

    env = _normalize_platform_name(os.environ.get("MAIX_PLATFORM"))
    if env:
        return env

    env2 = _normalize_platform_name(os.environ.get("PLATFORM"))
    if env2:
        return env2

    try:
        import _maix_hal as _hal
        hal_plat = _normalize_platform_name(getattr(_hal, "platform", ""))
        if hal_plat:
            return hal_plat
    except ImportError:
        pass

    machine = _plat.machine().lower()
    if "stm32" in machine or "cortex-m" in machine:
        return "stm32"
    # x86/amd64/aarch64/armv7l 均归为 linux
    return "linux"

# 全局平台变量
_current_platform = _detect_platform()

class PlatformError(Exception):
    """平台相关错误"""
    pass

class MockModule:
    """模拟模块基类"""
    def __init__(self, module_name):
        self.module_name = module_name

# 时间模块
class Time:
    """统一的时间接口"""
    
    def __init__(self):
        self._fps_start_time = _time.perf_counter()
        
    def time(self):
        """获取当前时间戳(秒)"""
        return _time.time()
    
    def time_ms(self):
        """获取当前时间戳(毫秒)"""
        return int(_time.time() * 1000)
    
    def time_us(self):
        """获取当前时间戳(微秒)"""
        return int(_time.time() * 1000000)
    
    def ticks_ms(self):
        """获取系统滴答(毫秒)"""
        if _current_platform == 'stm32':
            # STM32实现
            return int(_time.perf_counter() * 1000)
        else:
            # Linux/其他平台
            return int(_time.perf_counter() * 1000)
    
    def sleep_ms(self, ms):
        """毫秒延时"""
        _time.sleep(ms / 1000.0)
    
    def sleep_us(self, us):
        """微秒延时"""
        _time.sleep(us / 1000000.0)
    
    def sleep(self, s):
        """秒延时"""
        _time.sleep(s)
    
    def fps(self):
        """计算FPS"""
        current_time = _time.perf_counter()
        fps_value = 1.0 / max(current_time - self._fps_start_time, 0.001)
        self._fps_start_time = current_time
        return min(fps_value, 300.0)
    
    def fps_start(self):
        """开始FPS计算"""
        self._fps_start_time = _time.perf_counter()

    def ticks_s(self) -> int:
        """获取系统滴答（秒）"""
        return int(_time.perf_counter())

    def ticks_us(self) -> int:
        """获取系统滴答（微秒）"""
        return int(_time.perf_counter() * 1_000_000)

    def ticks_diff(self, start_ticks_ms: int) -> int:
        """计算经过毫秒数，处理 32bit 溢出"""
        diff = self.ticks_ms() - start_ticks_ms
        return diff + (1 << 32) if diff < 0 else diff

    def localtime(self):
        """获取本地时间"""
        import datetime as _dt
        return _dt.datetime.now()

    def timezone(self) -> int:
        """获取时区偏移（秒）"""
        import time as _t
        return -_t.timezone

# 应用模块
class App:
    """应用控制接口"""
    
    def __init__(self):
        self._exit_flag = False
        self._app_id = f"maixpy_nano_rtthread_{_current_platform}"
        
    def need_exit(self):
        """检查是否需要退出"""
        return self._exit_flag
    
    def app_id(self):
        """获取应用ID"""
        return self._app_id
    
    def set_exit_flag(self, flag):
        """设置退出标志"""
        self._exit_flag = flag
    
    def exit(self):
        """退出应用"""
        self._exit_flag = True

# 系统信息模块
class Sys:
    """系统信息接口"""
    
    def device_id(self):
        """获取设备ID"""
        if _current_platform == 'stm32':
            return "stm32_device"
        else:
            return "unknown_device"
    
    def platform(self):
        """获取平台信息"""
        return _current_platform
    
    def version(self):
        """获取版本信息"""
        return __version__

# GPIO模块
class GPIO:
    """统一GPIO接口"""

    # 内部枚举（对齐官方 MaixPy v4）
    class Mode:
        OUT    = 1
        IN     = 0
        AF     = 2
        ANALOG = 3

    class Pull:
        NONE = 0
        UP   = 1
        DOWN = 2

    # 旧常量保留（向后兼容）
    MODE_INPUT  = 0
    MODE_OUTPUT = 1
    MODE_AF     = 2
    MODE_ANALOG = 3

    # GPIO状态定义
    LOW  = 0
    HIGH = 1

    def __init__(self, pin, mode=MODE_OUTPUT, pull=None):
        self.pin  = pin
        self.mode = mode
        self.pull = pull
        self._init_gpio()

    def _init_gpio(self):
        """初始化GPIO"""
        if _current_platform == 'stm32':
            try:
                import _maix_hal
                _maix_hal.gpio_init(self.pin, self.mode, self.pull or 0)
                self._gpio = _maix_hal
            except ImportError:
                self._gpio = None
        else:
            self._gpio = None
            self._mock_state = 0
    
    def value(self, val=None):
        """读写GPIO值"""
        if val is None:
            if _current_platform == 'stm32' and self._gpio:
                return self._gpio.gpio_read(self.pin)
            elif _current_platform == 'stm32':
                return 0
            else:
                return getattr(self, '_mock_state', 0)
        else:
            if _current_platform == 'stm32' and self._gpio:
                self._gpio.gpio_write(self.pin, val)
            else:
                self._mock_state = 1 if val else 0
    
    def on(self):
        """置高"""
        self.value(self.HIGH)
    
    def off(self):
        """置低"""
        self.value(self.LOW)
    
    def toggle(self):
        """翻转"""
        current = self.value()
        if current is not None:
            self.value(1 - current)
        else:
            self.value(self.HIGH)

# 创建全局实例
time = Time()
app = App()
sys = Sys()

# 平台信息
def platform():
    """获取当前平台"""
    return _current_platform

def version():
    """获取版本信息"""
    return __version__

# 便利函数
def delay(ms):
    """延时(毫秒)"""
    time.sleep_ms(ms)

def delay_us(us):
    """延时(微秒)"""
    time.sleep_us(us)

import logging as _logging
_log = _logging.getLogger("maix")
_log.debug(f"[MaixPy Nano RT-Thread] 初始化完成，当前平台: {_current_platform}")
_log.debug(f"[MaixPy Nano RT-Thread] 版本: {__version__}")
