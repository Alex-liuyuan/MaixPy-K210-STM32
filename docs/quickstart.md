# 快速开始

## 当前建议的验证路径

先走这条闭环：

- 板卡：`stm32f407_nucleo`
- 架构：`ARM Cortex-M`
- 系统：`RT-Thread Nano`
- 调试串口：`USART2 @ 115200`
- 烧录方式：`ST-Link + OpenOCD`

如果你要看第二条架构参考链路，可以额外验证：

- 板卡：`k210_generic`
- 架构：`RISC-V`
- 系统：`RT-Thread Nano`
- 构建命令：`python3 project.py build -p k210`
- 烧录前提：必须提供真实 `kflash_py`

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

- `build/rtthread/SYSU_AIOTOS_rtthread.elf`
- `build/rtthread/SYSU_AIOTOS_rtthread.bin`
- `build/rtthread/SYSU_AIOTOS_rtthread.hex`

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
  -kernel build/sim/SYSU_AIOTOS_sim.elf \
  -no-reboot
```

QEMU 平台入口源码位于：

- `platforms/sim/`

## 预期现象

真板和 QEMU 都会进入统一的产品运行时，并输出类似：

```text
[BOOT] SYSU_AIOTOS 0.1.0
[BOOT] Runtime: ...
[CAP] storage=... python_vm=... model_runtime=...
[FS] /boot.py ...
[PY] MicroPython VM initialized ...
[boot.py] start
[app main] hello from bundled SYSU_AIOTOS app
[MODEL] backend=...
[APP] heartbeat=...
```

## FinSH 命令

```text
maix_info
maix_state
maix_fs
maix_py
maix_model
```

## 目前还没完成

以下能力仍未完成，看到 `planned` 或 `unavailable` 属于真实状态，不是故障：

- 文件系统与模型加载
- 摄像头 / 显示 / AI 推理闭环

其中要特别区分两件事：

- 固件内已经有只读脚本包资源。
- 这些脚本现在已经能由最小 MicroPython 端口执行，但还不是完整的可写 VFS。

## 设计口径

`SYSU_AIOTOS` 当前的正确口径是：

- 兼容 `ARM Cortex-M` 与 `RISC-V`
- `STM32F407` 与 `K210` 是首批参考板
- `RT-Thread Nano` 是统一系统层
- 运行时要逐步收敛到统一 Python/AI API
