"""
MaixPy Nano RT-Thread 摄像头模块
"""

import time
import numpy as np
from . import _current_platform


class Camera:
    """统一摄像头接口"""
    
    def __init__(self, width=320, height=240, format="RGB565"):
        """
        初始化摄像头
        
        Args:
            width: 图像宽度
            height: 图像高度
            format: 图像格式 ("RGB565", "RGB888", "YUV422", "GRAY")
        """
        self.width = width
        self.height = height
        self.format = format
        self._initialized = False
        
        print(f"[Camera] 初始化摄像头: {width}x{height} {format} (平台: {_current_platform})")
        
        if _current_platform == 'stm32':
            self._init_stm32_camera()
        else:
            self._init_mock_camera()
    
    def _init_stm32_camera(self):
        """初始化STM32摄像头"""
        try:
            import _maix_hal
            ret = _maix_hal.camera_open(self.width, self.height, self.format)
            if ret != 0:
                raise RuntimeError(f"camera_open 返回 {ret}")
            self._hal = _maix_hal
            self._initialized = True
            print("[Camera] STM32摄像头初始化成功")
        except ImportError:
            print("[Camera] STM32平台 _maix_hal 未找到，使用模拟模式")
            self._init_mock_camera()
    
    def _init_mock_camera(self):
        """初始化模拟摄像头"""
        self._initialized = True
        print("[Camera] 使用模拟摄像头")
    
    def read(self):
        """
        读取一帧图像
        
        Returns:
            Image: 图像对象
        """
        if not self._initialized:
            raise RuntimeError("摄像头未初始化")
        
        if _current_platform == 'stm32':
            return self._read_stm32()
        else:
            return self._read_mock()
    
    def _read_stm32(self):
        """读取STM32摄像头图像（真实DCMI调用）"""
        if hasattr(self, '_hal') and self._hal:
            # 等待新帧（最多50ms轮询）
            import time as _t
            deadline = _t.perf_counter() + 0.05
            while not self._hal.camera_frame_ready():
                if _t.perf_counter() > deadline:
                    break
            frame = self._hal.camera_read_frame()
            if frame is not None:
                return Image(frame, self.width, self.height, "RGB888")
        return self._generate_test_image()
    
    def _read_mock(self):
        """读取模拟摄像头图像"""
        return self._generate_test_image()
    
    def _generate_test_image(self):
        """生成测试图像"""
        # 生成随机彩色图像
        if self.format == "RGB565":
            # 16位RGB565格式
            data = np.random.randint(0, 65535, (self.height, self.width), dtype=np.uint16)
        elif self.format == "RGB888":
            # 24位RGB888格式
            data = np.random.randint(0, 255, (self.height, self.width, 3), dtype=np.uint8)
        elif self.format == "GRAY":
            # 8位灰度格式
            data = np.random.randint(0, 255, (self.height, self.width), dtype=np.uint8)
        else:
            # 默认RGB888
            data = np.random.randint(0, 255, (self.height, self.width, 3), dtype=np.uint8)
        
        return Image(data, self.width, self.height, self.format)
    
    def close(self):
        """关闭摄像头"""
        if self._initialized:
            if _current_platform == 'stm32':
                try:
                    if hasattr(self, '_hal') and self._hal:
                        self._hal.camera_close()
                except:
                    pass

            self._initialized = False
            print("[Camera] 摄像头已关闭")
    
    def __del__(self):
        """析构函数"""
        self.close()


class Image:
    """图像类"""
    
    def __init__(self, data, width, height, format="RGB888"):
        """
        创建图像对象
        
        Args:
            data: 图像数据
            width: 图像宽度
            height: 图像高度
            format: 图像格式
        """
        self.data = data
        self.width = width
        self.height = height
        self.format = format
        
        # 颜色定义
        self.COLOR_RED = (255, 0, 0)
        self.COLOR_GREEN = (0, 255, 0)
        self.COLOR_BLUE = (0, 0, 255)
        self.COLOR_YELLOW = (255, 255, 0)
        self.COLOR_PURPLE = (255, 0, 255)
        self.COLOR_CYAN = (0, 255, 255)
        self.COLOR_WHITE = (255, 255, 255)
        self.COLOR_BLACK = (0, 0, 0)
    
    def draw_rectangle(self, x, y, w, h, color=(255, 0, 0), thickness=1):
        """绘制矩形（numpy切片实现）"""
        if not isinstance(self.data, np.ndarray) or len(self.data.shape) != 3:
            return self
        x0, y0 = max(0, x), max(0, y)
        x1 = min(self.width,  x + w)
        y1 = min(self.height, y + h)
        c = np.array(color, dtype=np.uint8)
        t = max(1, thickness)
        # 上下边
        self.data[y0:y0+t,   x0:x1] = c
        self.data[y1-t:y1,   x0:x1] = c
        # 左右边
        self.data[y0:y1, x0:x0+t]   = c
        self.data[y0:y1, x1-t:x1]   = c
        return self
    
    def draw_circle(self, x, y, radius, color=(255, 0, 0), thickness=1):
        """
        绘制圆形
        
        Args:
            x, y: 圆心坐标
            radius: 半径
            color: 颜色 (R, G, B)
            thickness: 线条粗细
        """
        print(f"[Image] 绘制圆形: ({x}, {y}), 半径: {radius}, 颜色: {color}")
        return self
    
    def draw_string(self, x, y, text, color=(255, 255, 255), scale=1):
        """绘制文字（内嵌5x7点阵字体，支持大小写，numpy实现）"""
        if not isinstance(self.data, np.ndarray) or len(self.data.shape) != 3:
            return self
        # 5x7 ASCII点阵字体（大写 + 数字 + 符号）
        FONT5X7 = {
            ' ': [0x00,0x00,0x00,0x00,0x00],
            '0': [0x3E,0x51,0x49,0x45,0x3E],
            '1': [0x00,0x42,0x7F,0x40,0x00],
            '2': [0x42,0x61,0x51,0x49,0x46],
            '3': [0x21,0x41,0x45,0x4B,0x31],
            '4': [0x18,0x14,0x12,0x7F,0x10],
            '5': [0x27,0x45,0x45,0x45,0x39],
            '6': [0x3C,0x4A,0x49,0x49,0x30],
            '7': [0x01,0x71,0x09,0x05,0x03],
            '8': [0x36,0x49,0x49,0x49,0x36],
            '9': [0x06,0x49,0x49,0x29,0x1E],
            'A': [0x7E,0x11,0x11,0x11,0x7E],
            'B': [0x7F,0x49,0x49,0x49,0x36],
            'C': [0x3E,0x41,0x41,0x41,0x22],
            'D': [0x7F,0x41,0x41,0x22,0x1C],
            'E': [0x7F,0x49,0x49,0x49,0x41],
            'F': [0x7F,0x09,0x09,0x09,0x01],
            'G': [0x3E,0x41,0x49,0x49,0x7A],
            'H': [0x7F,0x08,0x08,0x08,0x7F],
            'I': [0x00,0x41,0x7F,0x41,0x00],
            'J': [0x20,0x40,0x41,0x3F,0x01],
            'K': [0x7F,0x08,0x14,0x22,0x41],
            'L': [0x7F,0x40,0x40,0x40,0x40],
            'M': [0x7F,0x02,0x0C,0x02,0x7F],
            'N': [0x7F,0x04,0x08,0x10,0x7F],
            'O': [0x3E,0x41,0x41,0x41,0x3E],
            'P': [0x7F,0x09,0x09,0x09,0x06],
            'Q': [0x3E,0x41,0x51,0x21,0x5E],
            'R': [0x7F,0x09,0x19,0x29,0x46],
            'S': [0x46,0x49,0x49,0x49,0x31],
            'T': [0x01,0x01,0x7F,0x01,0x01],
            'U': [0x3F,0x40,0x40,0x40,0x3F],
            'V': [0x1F,0x20,0x40,0x20,0x1F],
            'W': [0x3F,0x40,0x38,0x40,0x3F],
            'X': [0x63,0x14,0x08,0x14,0x63],
            'Y': [0x07,0x08,0x70,0x08,0x07],
            'Z': [0x61,0x51,0x49,0x45,0x43],
        }
        # 小写字母 5x7 点阵
        FONT5X7_LOWER = {
            'a': [0x20,0x54,0x54,0x54,0x78],
            'b': [0x7F,0x48,0x44,0x44,0x38],
            'c': [0x38,0x44,0x44,0x44,0x20],
            'd': [0x38,0x44,0x44,0x48,0x7F],
            'e': [0x38,0x54,0x54,0x54,0x18],
            'f': [0x08,0x7E,0x09,0x01,0x02],
            'g': [0x0C,0x52,0x52,0x52,0x3E],
            'h': [0x7F,0x08,0x04,0x04,0x78],
            'i': [0x00,0x44,0x7D,0x40,0x00],
            'j': [0x20,0x40,0x44,0x3D,0x00],
            'k': [0x7F,0x10,0x28,0x44,0x00],
            'l': [0x00,0x41,0x7F,0x40,0x00],
            'm': [0x7C,0x04,0x18,0x04,0x78],
            'n': [0x7C,0x08,0x04,0x04,0x78],
            'o': [0x38,0x44,0x44,0x44,0x38],
            'p': [0x7C,0x14,0x14,0x14,0x08],
            'q': [0x08,0x14,0x14,0x18,0x7C],
            'r': [0x7C,0x08,0x04,0x04,0x08],
            's': [0x48,0x54,0x54,0x54,0x20],
            't': [0x04,0x3F,0x44,0x40,0x20],
            'u': [0x3C,0x40,0x40,0x20,0x7C],
            'v': [0x1C,0x20,0x40,0x20,0x1C],
            'w': [0x3C,0x40,0x30,0x40,0x3C],
            'x': [0x44,0x28,0x10,0x28,0x44],
            'y': [0x0C,0x50,0x50,0x50,0x3C],
            'z': [0x44,0x64,0x54,0x4C,0x44],
        }
        c = np.array(color, dtype=np.uint8)
        s = max(1, int(scale))
        cx = x
        for ch in str(text):
            # 优先查小写字典，再查大写字典（大写转换兜底）
            bits = FONT5X7_LOWER.get(ch) or FONT5X7.get(ch.upper(), FONT5X7[' '])
            for col_idx, byte in enumerate(bits):
                for row_idx in range(7):
                    if byte & (1 << row_idx):
                        px = cx + col_idx * s
                        py = y  + row_idx * s
                        if 0 <= py < self.height and 0 <= px < self.width:
                            self.data[py:py+s, px:px+s] = c
            cx += (5 + 1) * s
        return self
    
    def draw_line(self, x1, y1, x2, y2, color=(255, 0, 0), thickness=1):
        """绘制直线（Bresenham算法，numpy实现）"""
        if not isinstance(self.data, np.ndarray) or len(self.data.shape) != 3:
            return self
        c = np.array(color, dtype=np.uint8)
        t = max(1, thickness)
        dx, dy = abs(x2 - x1), abs(y2 - y1)
        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1
        err = dx - dy
        cx, cy = x1, y1
        while True:
            if 0 <= cy < self.height and 0 <= cx < self.width:
                self.data[max(0,cy-t//2):cy+t//2+1,
                          max(0,cx-t//2):cx+t//2+1] = c
            if cx == x2 and cy == y2:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy; cx += sx
            if e2 < dx:
                err += dx; cy += sy
        return self
    
    def resize(self, width, height):
        """缩放图像（numpy最近邻插值）"""
        if isinstance(self.data, np.ndarray) and len(self.data.shape) == 3:
            h_old, w_old = self.data.shape[:2]
            row_idx = (np.arange(height) * h_old / height).astype(int)
            col_idx = (np.arange(width)  * w_old / width).astype(int)
            new_data = self.data[np.ix_(row_idx, col_idx)]
        else:
            new_data = np.zeros((height, width, 3), dtype=np.uint8)
        return Image(new_data, width, height, self.format)
    
    def crop(self, x, y, w, h):
        """
        裁剪图像
        
        Args:
            x, y: 左上角坐标
            w, h: 裁剪宽度和高度
            
        Returns:
            Image: 裁剪后的图像
        """
        print(f"[Image] 裁剪图像: ({x}, {y}, {w}, {h})")
        
        # 裁剪数据
        if self.format == "RGB888" and len(self.data.shape) == 3:
            cropped_data = self.data[y:y+h, x:x+w, :]
        else:
            cropped_data = self.data[y:y+h, x:x+w]
        
        return Image(cropped_data, w, h, self.format)
    
    def save(self, path):
        """
        保存图像
        
        Args:
            path: 保存路径
        """
        print(f"[Image] 保存图像: {path}")
        # 这里可以实现实际的保存逻辑
        return True
    
    def to_bytes(self):
        """
        转换为字节数据
        
        Returns:
            bytes: 图像字节数据
        """
        if isinstance(self.data, np.ndarray):
            return self.data.tobytes()
        else:
            return bytes(self.data)
    
    def copy(self):
        """
        复制图像
        
        Returns:
            Image: 图像副本
        """
        if isinstance(self.data, np.ndarray):
            new_data = self.data.copy()
        else:
            new_data = self.data[:]

        return Image(new_data, self.width, self.height, self.format)

    def find_blobs(self, thresholds, area_threshold=10):
        """
        简单色块检测（numpy 实现）。

        Args:
            thresholds: [(r_min, r_max, g_min, g_max, b_min, b_max), ...]
            area_threshold: 最小像素数，低于此值的色块被过滤

        Returns:
            list[BlobResult]
        """
        if not isinstance(self.data, np.ndarray) or len(self.data.shape) != 3:
            return []
        results = []
        for thr in thresholds:
            r_lo, r_hi = thr[0], thr[1]
            g_lo, g_hi = thr[2], thr[3]
            b_lo, b_hi = thr[4], thr[5]
            mask = (
                (self.data[:, :, 0] >= r_lo) & (self.data[:, :, 0] <= r_hi) &
                (self.data[:, :, 1] >= g_lo) & (self.data[:, :, 1] <= g_hi) &
                (self.data[:, :, 2] >= b_lo) & (self.data[:, :, 2] <= b_hi)
            )
            ys, xs = np.where(mask)
            if len(xs) < area_threshold:
                continue
            x = int(xs.min())
            y = int(ys.min())
            w = int(xs.max()) - x + 1
            h = int(ys.max()) - y + 1
            results.append(BlobResult(x, y, w, h, len(xs),
                                       int(xs.mean()), int(ys.mean())))
        return results


class BlobResult:
    """色块检测结果"""

    def __init__(self, x, y, w, h, pixels, cx, cy):
        self.x      = x
        self.y      = y
        self.w      = w
        self.h      = h
        self.pixels = pixels
        self.cx     = cx
        self.cy     = cy

    def __repr__(self):
        return (f"BlobResult(x={self.x}, y={self.y}, w={self.w}, h={self.h}, "
                f"pixels={self.pixels}, cx={self.cx}, cy={self.cy})")
