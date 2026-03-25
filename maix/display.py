"""
MaixPy Nano RT-Thread 显示模块
"""

import time
from . import _current_platform


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
        
        # 根据平台设置默认分辨率
        if _current_platform == 'stm32':
            self.width = width or 320
            self.height = height or 240
        else:
            self.width = width or 640
            self.height = height or 480
        
        print(f"[Display] 初始化显示器: {self.width}x{self.height} (平台: {_current_platform})")
        
        if _current_platform == 'stm32':
            self._init_stm32_display()
        else:
            self._init_mock_display()
    
    def _init_stm32_display(self):
        """初始化STM32显示"""
        try:
            import _maix_hal
            ret = _maix_hal.display_open(self.width, self.height)
            if ret != 0:
                raise RuntimeError(f"display_open 返回 {ret}")
            self._hal = _maix_hal
            self._initialized = True
            print("[Display] STM32显示器初始化成功")
        except ImportError:
            print("[Display] STM32平台 _maix_hal 未找到，使用模拟模式")
            self._init_mock_display()
    
    def _init_mock_display(self):
        """初始化模拟显示"""
        self._initialized = True
        print("[Display] 使用模拟显示器")
    
    def show(self, image):
        """
        显示图像
        
        Args:
            image: Image对象或图像数据
        """
        if not self._initialized:
            raise RuntimeError("显示器未初始化")
        
        if _current_platform == 'stm32':
            self._show_stm32(image)
        else:
            self._show_mock(image)
    
    def _show_stm32(self, image):
        """STM32显示图像（真实LCD调用）"""
        if hasattr(self, '_hal') and self._hal:
            import numpy as np
            if hasattr(image, 'data') and isinstance(image.data, np.ndarray):
                data = image.data
                if len(data.shape) == 3:
                    self._hal.display_show(data)
                    return
            # 回退：模拟打印
        print("[Display] STM32显示图像")
    
    def _show_mock(self, image):
        """模拟显示图像"""
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
        
        if _current_platform == 'stm32':
            try:
                # import _maix_hal  # STM32特定导入
                # _maix_hal.lcd_clear(color)
                print(f"[Display] STM32清屏: {color}")
            except ImportError:
                print(f"[Display] STM32清屏（模拟）: {color}")
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
        
        if _current_platform == 'stm32':
            try:
                # import _maix_hal  # STM32特定导入
                # _maix_hal.lcd_set_backlight(level)
                print(f"[Display] STM32设置背光: {level}%")
            except ImportError:
                print(f"[Display] STM32设置背光（模拟）: {level}%")
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
        
        if _current_platform == 'stm32':
            try:
                # import _maix_hal  # STM32特定导入
                # _maix_hal.lcd_set_rotation(rotation)
                print(f"[Display] STM32设置旋转: {rotation}度")
            except ImportError:
                print(f"[Display] STM32设置旋转（模拟）: {rotation}度")
        else:
            print(f"[Display] 模拟设置旋转: {rotation}度")
    
    def close(self):
        """关闭显示器"""
        if self._initialized:
            if _current_platform == 'stm32':
                try:
                    # import _maix_hal  # STM32特定导入
                    # _maix_hal.lcd_deinit()
                    pass
                except:
                    pass
            
            self._initialized = False
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
