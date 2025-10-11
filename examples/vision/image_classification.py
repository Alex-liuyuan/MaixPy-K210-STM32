"""
图像分类示例
演示K210 KPU和STM32 AI推理的统一接口
"""

from maix import camera, display, app, time, nn
import os

def main():
    """主函数"""
    print("=== 图像分类示例 ===")
    print("按Ctrl+C退出程序")
    
    # 模型路径（根据平台自动选择）
    model_paths = {
        'k210': '/flash/models/mobilenet_v2.kmodel',
        'stm32': '/models/mobilenet_v2.tflite',
        'linux': './models/mobilenet_v2.onnx'
    }
    
    # 类别标签
    labels = [
        'person', 'bicycle', 'car', 'motorcycle', 'airplane',
        'bus', 'train', 'truck', 'boat', 'traffic light',
        'fire hydrant', 'stop sign', 'parking meter', 'bench', 'bird',
        'cat', 'dog', 'horse', 'sheep', 'cow',
        'elephant', 'bear', 'zebra', 'giraffe', 'backpack'
    ]
    
    # 获取当前平台
    from maix import platform
    current_platform = platform()
    print(f"当前平台: {current_platform}")
    
    # 初始化摄像头
    print("初始化摄像头...")
    cam = camera.Camera(224, 224, "RGB888")  # MobileNet输入尺寸
    
    # 初始化显示器
    print("初始化显示器...")
    disp = display.Display()
    
    # 加载分类模型
    print("加载AI模型...")
    model_path = model_paths.get(current_platform, './models/demo_model.bin')
    
    if os.path.exists(model_path):
        classifier = nn.Classifier(model_path, labels)
        print(f"模型加载成功: {model_path}")
    else:
        print(f"模型文件不存在: {model_path}")
        print("使用模拟分类器...")
        classifier = nn.Classifier(None, labels)
    
    print("开始AI图像分类...")
    print("- 将摄像头对准物体进行分类")
    print("- 分类结果将显示在屏幕上")
    
    frame_count = 0
    
    while not app.need_exit():
        try:
            # 开始FPS计算
            time.fps_start()
            
            # 获取图像
            img = cam.read()
            
            # 运行分类
            results = classifier.classify(img)
            
            # 获取最佳结果
            if results:
                best_result = results[0]
                class_id, confidence, label = best_result
                
                # 在图像上绘制结果
                img.draw_string(10, 10, f"MaixPy-K210-STM32 AI分类", color=(255, 255, 255))
                img.draw_string(10, 30, f"平台: {current_platform}", color=(255, 255, 0))
                
                # 绘制分类结果
                result_color = (0, 255, 0) if confidence > 0.7 else (255, 255, 0) if confidence > 0.5 else (255, 0, 0)
                img.draw_string(10, 60, f"类别: {label}", color=result_color)
                img.draw_string(10, 80, f"置信度: {confidence:.3f}", color=result_color)
                img.draw_string(10, 100, f"类别ID: {class_id}", color=(200, 200, 200))
                
                # 绘制置信度条
                bar_width = int(confidence * 200)
                img.draw_rectangle(10, 120, bar_width, 10, color=result_color)
                img.draw_rectangle(10, 120, 200, 10, color=(100, 100, 100), thickness=1)
                
                # 如果置信度高，绘制边框
                if confidence > 0.8:
                    img.draw_rectangle(5, 5, img.width-10, img.height-10, color=(0, 255, 0), thickness=3)
                elif confidence > 0.6:
                    img.draw_rectangle(5, 5, img.width-10, img.height-10, color=(255, 255, 0), thickness=2)
            
            # 显示帧数和FPS
            img.draw_string(10, img.height-40, f"帧数: {frame_count}", color=(255, 255, 255))
            fps = time.fps()
            img.draw_string(10, img.height-20, f"FPS: {fps:.1f}", color=(255, 255, 255))
            
            # 显示图像
            disp.show(img)
            
            # 打印信息
            if frame_count % 30 == 0 and results:
                best_result = results[0]
                print(f"第{frame_count}帧 - 分类: {best_result[2]} (置信度: {best_result[1]:.3f})")
            
            frame_count += 1
            
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    # 清理资源
    print("清理资源...")
    classifier.unload()
    cam.close()
    disp.close()
    
    print("AI分类示例结束")

if __name__ == "__main__":
    main()