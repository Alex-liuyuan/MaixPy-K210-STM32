"""
MaixPy Nano RT-Thread 显示模块
"""

import time
from . import _current_platform

try:
    import _maix_hal
    _HAL_AVAILABLE = True
except ImportError:
    _maix_hal = None
    _HAL_AVAILABLE = False


class Display:
    """统一显示接口"""

    def __init__(self, width=None, height=None):
        """
        初始化显示器

        Args:
            width: 显示宽度，None为自动检测
            height: 显示高度，None为自动检测
        """
        self._initialized = False
        self._hal = None

        # 根据平台设置默认分辨率
        if _current_platform == 'stm32':
            self.width = width or 320
            self.height = height or 240
        else:
            self.width = width or 640
            self.height = height or 480

        print(f"[Display] 初始化显示器: {self.width}x{self.height} (平台: {_current_platform})")

        if _current_platform == 'stm32' and _HAL_AVAILABLE:
            ret = _maix_hal.display_open(self.width, self.height)
            if ret != 0:
                print(f"[Display] display_open 返回 {ret}，使用模拟模式")
            else:
                self._hal = _maix_hal
                self._initialized = True
                print("[Display] STM32显示器初始化成功")
                return

        # 模拟模式
        self._initialized = True
        if _current_platform == 'stm32':
            print("[Display] STM32平台 _maix_hal 未找到，使用模拟模式")
        else:
            print("[Display] 使用模拟显示器")

    def show(self, image):
        """
        显示图像

        Args:
            image: Image对象或图像数据
        """
        if not self._initialized:
            raise RuntimeError("显示器未初始化")

        if self._hal:
            import numpy as np
            if hasattr(image, 'data') and isinstance(image.data, np.ndarray):
                data = image.data
                if len(data.shape) == 3:
                    self._hal.display_show(data)
                    return
        # 模拟打印
        if hasattr(image, 'width') and hasattr(image, 'height'):
            print(f"[Display] 模拟显示图像: {image.width}x{image.height}")
        else:
            print("[Display] 模拟显示图像")

    def clear(self, color=(0, 0, 0)):
        """
        清屏

        Args:
            color: 清屏颜色 (R, G, B)
        """
        if not self._initialized:
            return

        if self._hal:
            self._hal.display_fill(color[0], color[1], color[2])
        else:
            print(f"[Display] 模拟清屏: {color}")

    def size(self):
        """
        获取显示器尺寸

        Returns:
            tuple: (width, height)
        """
        return (self.width, self.height)

    def set_backlight(self, level):
        """
        设置背光亮度

        Args:
            level: 亮度等级 (0-100)
        """
        level = max(0, min(100, level))

        if self._hal and hasattr(self._hal, 'lcd_set_backlight'):
            self._hal.lcd_set_backlight(level)
        else:
            print(f"[Display] 模拟设置背光: {level}%")

    def set_rotation(self, rotation):
        """
        设置屏幕旋转

        Args:
            rotation: 旋转角度 (0, 90, 180, 270)
        """
        if rotation not in [0, 90, 180, 270]:
            raise ValueError("rotation must be 0, 90, 180, or 270")

        if self._hal and hasattr(self._hal, 'lcd_set_rotation'):
            self._hal.lcd_set_rotation(rotation)
        else:
            print(f"[Display] 模拟设置旋转: {rotation}度")

    def close(self):
        """关闭显示器"""
        if self._initialized:
            if self._hal:
                try:
                    self._hal.display_close()
                except Exception:
                    pass
            self._initialized = False
            self._hal = None
            print("[Display] 显示器已关闭")

    def __del__(self):
        """析构函数"""
        self.close()


# 便利函数
def create_display(width=None, height=None):
    """
    创建显示器对象

    Args:
        width: 显示宽度
        height: 显示高度

    Returns:
        Display: 显示器对象
    """
    return Display(width, height)
