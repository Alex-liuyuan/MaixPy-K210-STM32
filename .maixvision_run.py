
import os; os.environ["MAIX_PLATFORM"]="linux"
from maix import camera, time as mt, GPIO, nn
from maix.err import check_raise, ERR_NONE
from maix import pinmap
from maix import uart, spi, adc, pwm

check_raise(ERR_NONE)
assert pinmap.set_pin_function("PA9", "USART1_TX") == ERR_NONE

cam = camera.Camera(320, 240, "RGB888")
img = cam.read()
img.draw_string(10, 10, "hello maixvision", color=(255,255,255))
blobs = img.find_blobs([(0,255,0,255,0,255)])
print(f"[WATCH] blobs = {len(blobs)}")

t0 = mt.ticks_ms()
mt.sleep_ms(10)
diff = mt.ticks_diff(t0)
print(f"[WATCH] diff_ms = {diff}")

led = GPIO(0, GPIO.Mode.OUT)
led.on(); led.off()
print("[WATCH] gpio = OK")

model = nn.NN()
model._load_mock_model("test.tflite")
import numpy as np
out = model.forward(np.zeros((224,224,3)))
det = nn.YOLOv8(model=None, labels=["cat","dog"])
print("[WATCH] nn = OK")

u = uart.UART(1, 115200)
u.write("hello")
print("[WATCH] uart = OK")

import maix.adc as madc
a = madc.ADC(1, resolution=madc.RES_BIT_12, vref=3.3)
vol = a.read_vol(madc.CH0)
print(f"[WATCH] adc_vol = {vol:.3f}")

import maix.pwm as mpwm
p = mpwm.PWM(pwm_id=1, freq=1000, duty=50.0)
print("[WATCH] pwm = OK")

print("ALL_DONE")
