"""
SYSU_AIOTOS 主机侧 API 示例。

这个示例用于验证 `maix` 主机 mock API 的基本调用流程，
不代表当前固件已经具备板上摄像头/显示运行能力。
"""

from maix import camera, display, app, time, platform

def main():
    print("=== SYSU_AIOTOS 主机侧 hello 示例 ===")
    print("说明: 这是主机 mock API 演示，不是板上 Python 运行证明")

    print("初始化主机 mock 摄像头...")
    cam = camera.Camera(320, 240, "RGB888")

    print("初始化主机 mock 显示...")
    disp = display.Display(320, 240)

    frame_count = 0

    while not app.need_exit():
        try:
            time.fps_start()

            img = cam.read()

            img.draw_string(10, 10, "SYSU_AIOTOS Host Mock", color=(255, 255, 255))
            img.draw_string(10, 30, f"Frame: {frame_count}", color=(0, 255, 0))
            img.draw_string(10, 50, f"Platform: {platform()}", color=(255, 255, 0))
            img.draw_rectangle(50, 80, 100, 60, color=(255, 0, 0), thickness=2)
            img.draw_string(55, 95, "Mock Frame", color=(255, 255, 255))

            disp.show(img)

            fps = time.fps()
            if frame_count % 30 == 0:
                print(f"主机 mock 帧时间: {1000 / fps:.02f}ms, FPS: {fps:.02f}")

            frame_count += 1
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    print("清理资源...")
    cam.close()
    disp.close()

    print("程序结束")

if __name__ == "__main__":
    main()
