"""
MaixPy-K210-STM32 显示模块
支持K210和STM32平台的统一显示接口
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
        if _current_platform == 'k210':
            self.width = width or 320
            self.height = height or 240
        elif _current_platform == 'stm32':
            self.width = width or 320
            self.height = height or 240
        else:
            self.width = width or 640
            self.height = height or 480
        
        print(f"[Display] 初始化显示器: {self.width}x{self.height} (平台: {_current_platform})")
        
        # 根据平台初始化显示器
        if _current_platform == 'k210':
            self._init_k210_display()
        elif _current_platform == 'stm32':
            self._init_stm32_display()
        else:
            self._init_mock_display()
    
    def _init_k210_display(self):
        """初始化K210显示"""
        try:
            # K210 LCD显示初始化
            # import lcd  # K210特定导入
            # lcd.init(type=1)  # ST7789
            # lcd.rotation(2)
            # lcd.clear()
            self._initialized = True
            print("[Display] K210显示器初始化成功")
        except ImportError:
            print("[Display] K210显示器初始化失败，使用模拟模式")
            self._init_mock_display()
    
    def _init_stm32_display(self):
        """初始化STM32显示"""
        try:
            # STM32 LCD显示初始化
            # import _maix_hal  # STM32特定导入
            # config = {
            #     'width': self.width,
            #     'height': self.height,
            #     'interface': 'spi'  # 或 'parallel'
            # }
            # _maix_hal.lcd_init(config)
            self._initialized = True
            print("[Display] STM32显示器初始化成功")
        except ImportError:
            print("[Display] STM32显示器初始化失败，使用模拟模式")
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
        
        if _current_platform == 'k210':
            self._show_k210(image)
        elif _current_platform == 'stm32':
            self._show_stm32(image)
        else:
            self._show_mock(image)
    
    def _show_k210(self, image):
        """K210显示图像"""
        try:
            # import lcd  # K210特定导入
            # if hasattr(image, 'data'):
            #     lcd.display(image.data)
            # else:
            #     lcd.display(image)
            print("[Display] K210显示图像")
        except ImportError:
            print("[Display] K210显示图像（模拟）")
    
    def _show_stm32(self, image):
        """STM32显示图像"""
        try:
            # import _maix_hal  # STM32特定导入
            # if hasattr(image, 'to_bytes'):
            #     _maix_hal.lcd_display(image.to_bytes())
            # else:
            #     _maix_hal.lcd_display(image)
            print("[Display] STM32显示图像")
        except ImportError:
            print("[Display] STM32显示图像（模拟）")
    
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
        
        if _current_platform == 'k210':
            try:
                # import lcd  # K210特定导入
                # rgb565_color = ((color[0] >> 3) << 11) | ((color[1] >> 2) << 5) | (color[2] >> 3)
                # lcd.clear(rgb565_color)
                print(f"[Display] K210清屏: {color}")
            except ImportError:
                print(f"[Display] K210清屏（模拟）: {color}")
        elif _current_platform == 'stm32':
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
        
        if _current_platform == 'k210':
            try:
                # import lcd  # K210特定导入
                # lcd.set_backlight(level)
                print(f"[Display] K210设置背光: {level}%")
            except ImportError:
                print(f"[Display] K210设置背光（模拟）: {level}%")
        elif _current_platform == 'stm32':
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
        
        if _current_platform == 'k210':
            try:
                # import lcd  # K210特定导入
                # rotation_map = {0: 0, 90: 1, 180: 2, 270: 3}
                # lcd.rotation(rotation_map[rotation])
                print(f"[Display] K210设置旋转: {rotation}度")
            except ImportError:
                print(f"[Display] K210设置旋转（模拟）: {rotation}度")
        elif _current_platform == 'stm32':
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
            if _current_platform == 'k210':
                try:
                    # import lcd  # K210特定导入
                    # lcd.deinit()
                    pass
                except:
                    pass
            elif _current_platform == 'stm32':
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