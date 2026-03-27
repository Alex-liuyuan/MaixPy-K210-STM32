"""
tests/test_pwm.py
PWM功能测试
"""

import pytest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_pwm_init_hal(hal):
    """测试HAL层PWM初始化"""
    assert hal.pwm.init(0, 0, 83, 999, 500, 0) == 0


def test_pwm_start_stop_hal(hal):
    """测试HAL层PWM启停"""
    hal.pwm.init(0, 0)
    assert hal.pwm.start(0, 0) == 0
    assert hal.pwm.stop(0, 0) == 0


def test_pwm_set_get_duty_hal(hal):
    """测试HAL层PWM占空比设置/读取"""
    hal.pwm.init(0, 0, 83, 999, 500, 0)
    assert hal.pwm.set_duty(0, 0, 750) == 0
    assert hal.pwm.get_duty(0, 0) == 750


def test_pwm_deinit_hal(hal):
    """测试HAL层PWM释放"""
    hal.pwm.init(0, 0)
    assert hal.pwm.deinit(0, 0) == 0


# ---- 高层 sysu.pwm.PWM 接口测试 ----

def test_pwm_class_init_pwm_id():
    """测试通过 pwm_id 初始化"""
    from sysu.pwm import PWM
    p = PWM(pwm_id=1, freq=1000, duty=50.0)
    assert p.timer_id == 0   # (1-1)//4 = 0
    assert p.channel == 0    # (1-1)%4 = 0
    assert p._running is True  # enable=True 默认启动


def test_pwm_class_init_timer_channel():
    """测试通过 timer/channel 初始化"""
    from sysu.pwm import PWM
    p = PWM(timer=2, channel=1, freq=1000, duty=50.0)
    assert p.timer_id == 1   # 2-1
    assert p.channel == 0    # 1-1


def test_pwm_class_init_no_enable():
    """测试 enable=False 不自动启动"""
    from sysu.pwm import PWM
    p = PWM(pwm_id=1, freq=1000, duty=50.0, enable=False)
    assert p._running is False


def test_pwm_class_start_stop():
    """测试启停"""
    from sysu.pwm import PWM
    p = PWM(pwm_id=1, freq=1000, duty=50.0, enable=False)
    assert p._running is False
    p.start()
    assert p._running is True
    p.stop()
    assert p._running is False


def test_pwm_class_duty():
    """测试占空比设置"""
    from sysu.pwm import PWM
    p = PWM(pwm_id=1, freq=1000, duty=50.0)
    p.duty(75.0)  # 不应抛异常
    p.duty(0.0)
    p.duty(100.0)


def test_pwm_class_close():
    """测试PWM close()"""
    from sysu.pwm import PWM
    p = PWM(pwm_id=1, freq=1000, duty=50.0)
    assert p._running is True
    p.close()
    assert p._running is False


def test_pwm_class_context_manager():
    """测试上下文管理器"""
    from sysu.pwm import PWM
    with PWM(pwm_id=2, freq=500, duty=25.0) as p:
        assert p._running is True
        p.duty(50.0)
    assert p._running is False


def test_pwm_id_mapping():
    """测试 pwm_id 到 timer/channel 的映射"""
    from sysu.pwm import PWM
    # pwm_id=5 -> timer_id=1, channel=0
    p5 = PWM(pwm_id=5, freq=1000, duty=50.0)
    assert p5.timer_id == 1
    assert p5.channel == 0
    # pwm_id=8 -> timer_id=1, channel=3
    p8 = PWM(pwm_id=8, freq=1000, duty=50.0)
    assert p8.timer_id == 1
    assert p8.channel == 3
