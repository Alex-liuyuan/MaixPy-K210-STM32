"""
GPIO 控制示例。

当前默认用于验证 `maix.GPIO` 的主机 mock 行为，以及统一 API 形状。
"""

from maix import GPIO, time, app

def main():
    print("=== GPIO控制示例 ===")
    print("说明: 当前板级 Python VM 未接入，这里验证的是主机 mock GPIO")

    led_pin = 0
    button_pin = 1

    led = GPIO(led_pin, GPIO.MODE_OUTPUT)
    print(f"LED引脚{led_pin}初始化为输出模式")

    button = GPIO(button_pin, GPIO.MODE_INPUT, pull=1)
    print(f"按钮引脚{button_pin}初始化为输入模式")

    led_state = False
    button_pressed = False
    last_button_state = GPIO.HIGH

    count = 0

    while not app.need_exit():
        try:
            current_button_state = button.value()

            if last_button_state == GPIO.HIGH and current_button_state == GPIO.LOW:
                button_pressed = True
                print("按钮被按下!")

            last_button_state = current_button_state

            if button_pressed:
                led_state = not led_state
                led.value(GPIO.HIGH if led_state else GPIO.LOW)
                print(f"LED状态切换为: {'ON' if led_state else 'OFF'}")
                button_pressed = False

            if count % 200 == 0:
                if not button_pressed:
                    led.toggle()
                    led_state = not led_state
                    print(f"自动闪烁: LED {'ON' if led_state else 'OFF'}")

            if count % 100 == 0:
                print(f"运行时间: {count//100}s, LED: {'ON' if led_state else 'OFF'}, 按钮: {'按下' if current_button_state == GPIO.LOW else '释放'}")

            count += 1
            time.sleep_ms(10)
        except KeyboardInterrupt:
            print("用户中断程序")
            break
        except Exception as e:
            print(f"错误: {e}")
            break
    
    led.off()
    print("GPIO示例结束")

if __name__ == "__main__":
    main()
