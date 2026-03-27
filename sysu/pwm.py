"""MaixPy Nano RT-Thread PWM 模块。"""

from . import _current_platform

try:
    import _maix_hal as _hal
    _HAL_AVAILABLE = True
except ImportError:
    _hal = None
    _HAL_AVAILABLE = False


class PWM:
    """统一PWM接口

    示例（1kHz，50%占空比）：
        pwm = PWM(timer=2, channel=1, freq=1000, duty=50.0)
        pwm.duty(75.0)
        pwm.stop()

    简化构造（通过 pwm_id 自动映射 timer/channel）：
        pwm = PWM(pwm_id=1, freq=1000, duty=50.0)
    """

    def __init__(self, pwm_id=None, freq=1000, duty=50.0, enable=True,
                 timer=None, channel=1, sysclk=84_000_000):
        """
        初始化PWM

        Args:
            pwm_id:  PWM通道编号（1-16），自动映射到 timer/channel
            freq:    PWM频率（Hz）
            duty:    占空比（0.0-100.0 %）
            enable:  初始化后立即启动
            timer:   定时器编号（1-14），pwm_id 为 None 时使用
            channel: 通道（1-4），pwm_id 为 None 时使用
            sysclk:  定时器时钟频率（Hz），F407默认84MHz
        """
        if pwm_id is not None:
            self.timer_id = (pwm_id - 1) // 4
            self.channel  = (pwm_id - 1) % 4
        else:
            t = timer if timer is not None else 2
            self.timer_id = t - 1
            self.channel  = channel - 1
        self._running = False
        self._init(freq, duty, sysclk)
        if enable:
            self.start()

    def _init(self, freq, duty, sysclk):
        # 计算预分频和周期
        # prescaler+1 使计数器时钟 = sysclk / (prescaler+1)
        # period+1    使PWM频率    = 计数器时钟 / (period+1)
        prescaler = max(0, sysclk // (freq * 1000) - 1)
        period    = max(0, sysclk // ((prescaler + 1) * freq) - 1)
        pulse     = int(period * duty / 100.0)

        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            ret = _hal.pwm.init(self.timer_id, self.channel,
                                prescaler, period, pulse, 0)
            if ret != 0:
                print(f"[PWM] 初始化失败 ret={ret}，使用模拟模式")
        else:
            print(f"[PWM] 模拟TIM{self.timer_id+1}CH{self.channel+1} "
                  f"freq={freq}Hz duty={duty}%")

        self._period = period

    def start(self):
        """启动PWM输出"""
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.pwm.start(self.timer_id, self.channel)
        self._running = True

    def stop(self):
        """停止PWM输出"""
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.pwm.stop(self.timer_id, self.channel)
        self._running = False

    def duty(self, percent):
        """设置占空比（0.0-100.0 %）"""
        permille = int(percent * 10)  # 转为0-1000
        permille = max(0, min(1000, permille))
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.pwm.set_duty(self.timer_id, self.channel, permille)
        else:
            print(f"[PWM] 模拟设置占空比 {percent:.1f}%")

    def close(self):
        """释放PWM"""
        self.stop()
        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            _hal.pwm.deinit(self.timer_id, self.channel)

    def __del__(self):
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
