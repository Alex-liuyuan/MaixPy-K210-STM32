"""
GPIO控制示例
演示K210和STM32的GPIO统一接口
"""

from maix import GPIO, time, app

def main():
    """主函数"""
    print("=== GPIO控制示例 ===")
    print("按Ctrl+C退出程序")
    
    # 创建GPIO对象
    # K210: 使用GPIOHS_0 (引脚编号0)
    # STM32: 使用GPIOA_0 (端口A引脚0，编码为(0<<16)|0)
    led_pin = 0  # 根据平台自动适配
    button_pin = 1
    
    # 初始化LED引脚为输出模式
    led = GPIO(led_pin, GPIO.MODE_OUTPUT)
    print(f"LED引脚{led_pin}初始化为输出模式")
    
    # 初始化按钮引脚为输入模式，带上拉
    button = GPIO(button_pin, GPIO.MODE_INPUT, pull=1)  # 1表示上拉
    print(f"按钮引脚{button_pin}初始化为输入模式")
    
    led_state = False
    button_pressed = False
    last_button_state = GPIO.HIGH
    
    print("开始GPIO控制循环...")
    print("- LED每秒闪烁一次")
    print("- 按下按钮可以切换LED状态")
    
    count = 0
    
    while not app.need_exit():
        try:
            # 读取按钮状态
            current_button_state = button.value()
            
            # 检测按钮按下事件（下降沿）
            if last_button_state == GPIO.HIGH and current_button_state == GPIO.LOW:
                button_pressed = True
                print("按钮被按下!")
            
            last_button_state = current_button_state
            
            # 如果按钮被按下，切换LED状态
            if button_pressed:
                led_state = not led_state
                led.value(GPIO.HIGH if led_state else GPIO.LOW)
                print(f"LED状态切换为: {'ON' if led_state else 'OFF'}")
                button_pressed = False
            
            # 自动闪烁模式（每2秒切换一次）
            if count % 200 == 0:  # 约2秒（假设每次循环10ms）
                if not button_pressed:  # 如果没有手动控制
                    led.toggle()
                    led_state = not led_state
                    print(f"自动闪烁: LED {'ON' if led_state else 'OFF'}")
            
            # 显示状态信息
            if count % 100 == 0:  # 每秒显示一次
                print(f"运行时间: {count//100}s, LED: {'ON' if led_state else 'OFF'}, 按钮: {'按下' if current_button_state == GPIO.LOW else '释放'}")
            
            count += 1
            
            # 短暂延时
            time.sleep_ms(10)
            
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    # 关闭LED
    led.off()
    print("GPIO示例结束")

if __name__ == "__main__":
    main()