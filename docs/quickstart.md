# 快速开始

## 当前建议的验证路径

先走这条闭环：

- 板卡：`stm32f407_nucleo`
- 系统：`RT-Thread Nano`
- 调试串口：`USART2 @ 115200`
- 烧录方式：`ST-Link + OpenOCD`

## 依赖

- `cmake >= 3.20`
- `arm-none-eabi-gcc`
- `openocd`
- `python3`

## 构建真板固件

```bash
python3 project.py build -p rtthread
```

产物位于：

- `build/rtthread/MaixPy_rtthread.elf`
- `build/rtthread/MaixPy_rtthread.bin`
- `build/rtthread/MaixPy_rtthread.hex`

## 烧录真板

```bash
python3 project.py flash -p rtthread
```

## 串口监视

```bash
python3 project.py monitor -p rtthread -d /dev/ttyACM0
```

## 构建 QEMU 固件

```bash
python3 project.py build -p sim
```

## 运行 QEMU

```bash
python3 project.py monitor -p sim
```

或者直接运行：

```bash
qemu-system-arm \
  -machine olimex-stm32-h405 \
  -nographic \
  -semihosting \
  -semihosting-config enable=on,target=native \
  -kernel build/sim/MaixPy_sim.elf \
  -no-reboot
```

## 预期现象

真板和 QEMU 都会进入统一的产品运行时，并输出类似：

```text
[BOOT] MaixPy Nano 0.1.0
[BOOT] Runtime: ...
[CAP] storage=... python_vm=... model_runtime=...
[APP] heartbeat=...
```

## FinSH 命令

```text
maix_info
maix_state
```

## 目前还没完成

以下能力仍未完成，看到 `planned` 或 `unavailable` 属于真实状态，不是故障：

- Python VM
- `boot.py` / `main.py`
- 文件系统与模型加载
- 摄像头 / 显示 / AI 推理闭环
