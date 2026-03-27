"""
图像分类示例。

当前仓库里，这个脚本只用于主机侧 mock 推理流程验证。
它不代表 RT-Thread Nano 固件已经具备板上模型加载或板上 Python 推理能力。
"""

from maix import camera, display, app, time, nn

def main():
    print("=== 图像分类示例 ===")
    print("说明: 当前运行的是主机 mock 推理，不是板上模型运行")

    labels = [
        'person', 'bicycle', 'car', 'motorcycle', 'airplane',
        'bus', 'train', 'truck', 'boat', 'traffic light',
        'fire hydrant', 'stop sign', 'parking meter', 'bench', 'bird',
        'cat', 'dog', 'horse', 'sheep', 'cow',
        'elephant', 'bear', 'zebra', 'giraffe', 'backpack'
    ]

    from maix import platform
    current_platform = platform()
    print(f"当前平台: {current_platform}")

    print("初始化主机 mock 摄像头...")
    cam = camera.Camera(224, 224, "RGB888")

    print("初始化主机 mock 显示器...")
    disp = display.Display()

    print("加载主机 mock 分类器...")
    classifier = nn.Classifier(None, labels)

    frame_count = 0

    while not app.need_exit():
        try:
            time.fps_start()

            img = cam.read()
            results = classifier.classify(img)

            if results:
                best_result = results[0]
                class_id, confidence, label = best_result

                img.draw_string(10, 10, "Host Mock Classification", color=(255, 255, 255))
                img.draw_string(10, 30, f"平台: {current_platform}", color=(255, 255, 0))

                result_color = (0, 255, 0) if confidence > 0.7 else (255, 255, 0) if confidence > 0.5 else (255, 0, 0)
                img.draw_string(10, 60, f"类别: {label}", color=result_color)
                img.draw_string(10, 80, f"置信度: {confidence:.3f}", color=result_color)
                img.draw_string(10, 100, f"类别ID: {class_id}", color=(200, 200, 200))

                bar_width = int(confidence * 200)
                img.draw_rectangle(10, 120, bar_width, 10, color=result_color)
                img.draw_rectangle(10, 120, 200, 10, color=(100, 100, 100), thickness=1)

                if confidence > 0.8:
                    img.draw_rectangle(5, 5, img.width-10, img.height-10, color=(0, 255, 0), thickness=3)
                elif confidence > 0.6:
                    img.draw_rectangle(5, 5, img.width-10, img.height-10, color=(255, 255, 0), thickness=2)

            img.draw_string(10, img.height-40, f"帧数: {frame_count}", color=(255, 255, 255))
            fps = time.fps()
            img.draw_string(10, img.height-20, f"FPS: {fps:.1f}", color=(255, 255, 255))

            disp.show(img)

            if frame_count % 30 == 0 and results:
                best_result = results[0]
                print(f"第{frame_count}帧 - 主机 mock 分类: {best_result[2]} (置信度: {best_result[1]:.3f})")

            frame_count += 1
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    print("清理资源...")
    classifier.unload()
    cam.close()
    disp.close()
    
    print("AI分类示例结束")

if __name__ == "__main__":
    main()
