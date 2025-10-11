"""
MaixPy-K210-STM32 基础示例
类似于原始MaixPy的hello_maix.py
"""

from maix import camera, display, app, time, platform

def main():
    """主函数"""
    print("=== MaixPy-K210-STM32 基础示例 ===")
    print("按Ctrl+C退出程序")
    
    # 初始化摄像头 - 支持K210和STM32
    print("初始化摄像头...")
    cam = camera.Camera(320, 240, "RGB888")
    
    # 初始化显示器 - 支持K210和STM32  
    print("初始化显示器...")
    disp = display.Display(320, 240)
    
    print("开始运行，显示摄像头图像...")
    
    frame_count = 0
    
    while not app.need_exit():
        try:
            # 开始FPS计算，可选
            time.fps_start()
            
            # 从摄像头获取一帧图像
            img = cam.read()
            
            # 在图像上绘制一些信息
            img.draw_string(10, 10, f"MaixPy-K210-STM32", color=(255, 255, 255))
            img.draw_string(10, 30, f"Frame: {frame_count}", color=(0, 255, 0))
            img.draw_string(10, 50, f"Platform: {platform()}", color=(255, 255, 0))
            
            # 绘制一个矩形框
            img.draw_rectangle(50, 80, 100, 60, color=(255, 0, 0), thickness=2)
            img.draw_string(55, 95, "Detection", color=(255, 255, 255))
            
            # 显示图像到屏幕
            disp.show(img)
            
            # 计算并显示FPS
            fps = time.fps()
            if frame_count % 30 == 0:  # 每30帧显示一次FPS
                print(f"时间: {1000/fps:.02f}ms, FPS: {fps:.02f}")
            
            frame_count += 1
            
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    # 清理资源
    print("清理资源...")
    cam.close()
    disp.close()
    
    print("程序结束")

if __name__ == "__main__":
    main()