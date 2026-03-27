"""
tests/test_display.py
Display功能测试
"""

import pytest
import numpy as np
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


def test_display_hal_open_close(hal):
    """测试HAL层显示器打开/关闭"""
    assert hal.display_open(320, 240) == 0
    assert hal._disp_open is True
    assert hal.display_close() == 0
    assert hal._disp_open is False


def test_display_hal_show(hal):
    """测试HAL层显示图像"""
    hal.display_open(320, 240)
    img = np.zeros((240, 320, 3), dtype=np.uint8)
    assert hal.display_show(img) == 0
    last = hal._display_get_last_frame()
    assert last is not None
    assert last.shape == (240, 320, 3)


def test_display_hal_fill(hal):
    """测试HAL层填充"""
    hal.display_open(100, 100)
    assert hal.display_fill(255, 0, 0) == 0
    last = hal._display_get_last_frame()
    assert last is not None
    assert np.all(last[:, :, 0] == 255)
    assert np.all(last[:, :, 1] == 0)


def test_display_hal_size(hal):
    """测试HAL层尺寸查询"""
    hal.display_open(480, 320)
    assert hal.display_width() == 480
    assert hal.display_height() == 320


# ---- 高层 sysu.display.Display 接口测试 ----

def test_display_class_init():
    """测试Display类初始化"""
    from sysu.display import Display
    d = Display(320, 240)
    assert d.width == 320
    assert d.height == 240
    assert d._initialized is True
    d.close()


def test_display_class_default_size():
    """测试默认分辨率随平台变化"""
    from sysu.display import Display
    from sysu import _current_platform
    d = Display()
    if _current_platform == 'stm32':
        assert d.width == 320
        assert d.height == 240
    else:
        assert d.width == 640
        assert d.height == 480
    d.close()


def test_display_class_show():
    """测试show()方法"""
    from sysu.display import Display
    from sysu.camera import Image
    d = Display(320, 240)
    img = Image(np.zeros((240, 320, 3), dtype=np.uint8), 320, 240, "RGB888")
    d.show(img)  # 不应抛异常
    d.close()


def test_display_class_show_not_initialized():
    """测试未初始化时show()抛异常"""
    from sysu.display import Display
    d = Display(320, 240)
    d._initialized = False
    with pytest.raises(RuntimeError):
        d.show(None)


def test_display_class_clear():
    """测试clear()方法"""
    from sysu.display import Display
    d = Display(320, 240)
    d.clear()  # 默认黑色
    d.clear(color=(255, 0, 0))  # 红色
    d.close()


def test_display_class_size():
    """测试size()方法"""
    from sysu.display import Display
    d = Display(320, 240)
    assert d.size() == (320, 240)
    d.close()


def test_display_class_set_backlight():
    """测试set_backlight()方法"""
    from sysu.display import Display
    d = Display(320, 240)
    d.set_backlight(50)
    d.set_backlight(0)
    d.set_backlight(100)
    d.close()


def test_display_class_set_backlight_clamp():
    """测试背光值钳位"""
    from sysu.display import Display
    d = Display(320, 240)
    d.set_backlight(-10)   # 应钳位到0
    d.set_backlight(200)   # 应钳位到100
    d.close()


def test_display_class_set_rotation():
    """测试set_rotation()方法"""
    from sysu.display import Display
    d = Display(320, 240)
    for angle in [0, 90, 180, 270]:
        d.set_rotation(angle)
    d.close()


def test_display_class_set_rotation_invalid():
    """测试无效旋转角度"""
    from sysu.display import Display
    d = Display(320, 240)
    with pytest.raises(ValueError):
        d.set_rotation(45)
    d.close()


def test_display_class_close():
    """测试close()方法"""
    from sysu.display import Display
    d = Display(320, 240)
    assert d._initialized is True
    d.close()
    assert d._initialized is False


def test_display_class_double_close():
    """测试重复close()不抛异常"""
    from sysu.display import Display
    d = Display(320, 240)
    d.close()
    d.close()  # 第二次不应抛异常


def test_create_display_convenience():
    """测试便利函数"""
    from sysu.display import create_display
    d = create_display(160, 120)
    assert d.width == 160
    assert d.height == 120
    d.close()
