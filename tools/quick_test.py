"""
快速功能测试脚本（开发机/仿真/硬件均可）
"""

import os
import sys
import time

# 确保可导入本地 maix 包
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

# 基础模块
from maix import GPIO
from maix import camera, display


def main():
    # GPIO 测试（使用模拟/实际引脚编码）
    led = GPIO(0x00000005, GPIO.MODE_OUTPUT)  # PA5
    led.on()
    time.sleep(0.1)
    led.off()

    # 摄像头 + 显示测试
    cam = camera.Camera(320, 240, "RGB888")
    disp = display.Display(320, 240)

    img = cam.read()
    img.draw_rectangle(10, 10, 100, 60, color=(255, 0, 0), thickness=2)
    img.draw_string(12, 14, "SYSU_AIOTOS", color=(255, 255, 255))
    disp.show(img)

    # 简单延时，确保日志可见
    time.sleep(0.2)

    cam.close()


if __name__ == "__main__":
    main()
