"""
MaixPy-K210-STM32 核心模块
支持K210和STM32平台的统一Python API
"""

import sys
import os
import time as _time

# 版本信息
__version__ = "1.0.0"

# 平台检测
def _detect_platform():
    """自动检测当前运行平台"""
    try:
        # 尝试导入平台特定模块
        import platform
        machine = platform.machine().lower()
        
        if 'k210' in machine or 'riscv' in machine:
            return 'k210'
        elif 'arm' in machine:
            return 'stm32'
        elif 'x86' in machine or 'amd64' in machine:
            return 'linux'
        else:
            return 'unknown'
    except:
        return 'unknown'

# 全局平台变量
_current_platform = _detect_platform()

class PlatformError(Exception):
    """平台相关错误"""
    pass

class MockModule:
    """模拟模块基类"""
    def __init__(self, module_name):
        self.module_name = module_name
        print(f"[MaixPy-K210-STM32] 加载模拟模块: {module_name}")

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
        if _current_platform == 'k210':
            # K210实现
            try:
                # from machine import time_pulse_us  # K210特定导入
                return int(_time.perf_counter() * 1000)
            except ImportError:
                return int(_time.perf_counter() * 1000)
        elif _current_platform == 'stm32':
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

# 应用模块
class App:
    """应用控制接口"""
    
    def __init__(self):
        self._exit_flag = False
        self._app_id = f"maixpy_k210_stm32_{_current_platform}"
        
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
        if _current_platform == 'k210':
            return "k210_device"
        elif _current_platform == 'stm32':
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
    
    # GPIO模式定义
    MODE_INPUT = 0
    MODE_OUTPUT = 1
    MODE_AF = 2
    MODE_ANALOG = 3
    
    # GPIO状态定义
    LOW = 0
    HIGH = 1
    
    def __init__(self, pin, mode=MODE_OUTPUT, pull=None):
        self.pin = pin
        self.mode = mode
        self.pull = pull
        self._init_gpio()
    
    def _init_gpio(self):
        """初始化GPIO"""
        if _current_platform == 'k210':
            # K210 GPIO初始化
            try:
                # from fpioa_manager import fm  # K210特定导入
                # from Maix import GPIO as K210_GPIO  # K210特定导入
                # self._gpio = K210_GPIO(self.pin, self.mode)
                self._gpio = None  # 暂时使用模拟模式
            except ImportError:
                print(f"[GPIO] K210平台GPIO初始化失败，使用模拟模式")
                self._gpio = None
                
        elif _current_platform == 'stm32':
            # STM32 GPIO初始化
            try:
                # 调用HAL层GPIO接口
                # import _maix_hal  # STM32特定导入
                # _maix_hal.gpio_init(self.pin, self.mode, self.pull or 0)
                self._gpio = None  # 暂时使用模拟模式
            except ImportError:
                print(f"[GPIO] STM32平台GPIO初始化失败，使用模拟模式")
                self._gpio = None
        else:
            # 其他平台模拟
            print(f"[GPIO] 模拟GPIO {self.pin}, 模式: {self.mode}")
            self._gpio = None
    
    def value(self, val=None):
        """读写GPIO值"""
        if val is None:
            # 读取
            if _current_platform == 'k210' and self._gpio:
                return self._gpio.value()
            elif _current_platform == 'stm32':
                try:
                    # import _maix_hal  # STM32特定导入
                    # return _maix_hal.gpio_read(self.pin)
                    return 0  # 暂时返回模拟值
                except ImportError:
                    return 0
            else:
                return 0
        else:
            # 写入
            if _current_platform == 'k210' and self._gpio:
                self._gpio.value(val)
            elif _current_platform == 'stm32':
                try:
                    # import _maix_hal  # STM32特定导入
                    # _maix_hal.gpio_write(self.pin, val)
                    print(f"[GPIO] STM32模拟写入 GPIO{self.pin} = {val}")
                except ImportError:
                    pass
            else:
                print(f"[GPIO] 模拟写入 GPIO{self.pin} = {val}")
    
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

print(f"[MaixPy-K210-STM32] 初始化完成，当前平台: {_current_platform}")
print(f"[MaixPy-K210-STM32] 版本: {__version__}")