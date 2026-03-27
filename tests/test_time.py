"""
sysu time 增强功能测试
"""
import os
os.environ["MAIX_PLATFORM"] = "linux"

from sysu import time


def test_ticks_ms_increases():
    t0 = time.ticks_ms()
    time.sleep_ms(10)
    t1 = time.ticks_ms()
    assert t1 >= t0


def test_ticks_s():
    s = time.ticks_s()
    assert isinstance(s, int)
    assert s >= 0


def test_ticks_us():
    us = time.ticks_us()
    assert isinstance(us, int)
    assert us >= 0


def test_ticks_diff_normal():
    t0 = time.ticks_ms()
    time.sleep_ms(15)
    diff = time.ticks_diff(t0)
    assert diff >= 10


def test_ticks_diff_overflow():
    # 模拟 32bit 溢出：start 接近最大值，当前值较小
    max32 = 1 << 32
    # 手动构造：当前 ticks 比 start 小（溢出场景）
    fake_start = time.ticks_ms() + 100  # 未来时间点，模拟溢出
    diff = time.ticks_diff(fake_start)
    # 溢出时应加 2^32 修正
    assert diff >= 0


def test_localtime():
    import datetime
    lt = time.localtime()
    assert isinstance(lt, datetime.datetime)


def test_timezone():
    tz = time.timezone()
    assert isinstance(tz, int)
    # 时区偏移在 -12h ~ +14h 之间
    assert -12 * 3600 <= tz <= 14 * 3600


def test_ticks_us_greater_than_ms():
    ms = time.ticks_ms()
    us = time.ticks_us()
    # 微秒值应远大于毫秒值
    assert us > ms
