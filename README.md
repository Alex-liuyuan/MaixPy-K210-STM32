# MaixPy Nano RT-Thread

面向 RT-Thread Nano 的 MaixPy 风格产品原型。

当前仓库只保留一条真实可交付主线：

- 真板：`STM32F407 Nucleo + RT-Thread Nano`
- 仿真：`QEMU olimex-stm32-h405`

这条主线已经具备：

- 固件构建
- ST-Link + OpenOCD 烧录入口
- 串口 / semihosting 日志
- 统一的产品运行时骨架
- QEMU 最小可验证链路
- 明确的能力声明，不伪造 AI / 摄像头 / 显示成功结果

当前还没有完成：

- 嵌入式 Python VM
- `boot.py` / `main.py` 脚本加载
- VFS / 模型文件加载
- 真正的板上模型推理闭环

## 仓库结构

```text
app/                              产品运行时骨架
boards/                           当前板卡描述
cmake/                            CMake 与工具链配置
components/drivers/stm32/         STM32 驱动
components/hal/                   通用 HAL 接口
docs/                             当前产品文档
maix/                             主机侧 API 与测试辅助
rtthread-nano-master/             RT-Thread Nano 内核与 F407 BSP
sim/                              QEMU 仿真入口
tests/                            主机回归测试
tools/                            构建 / 烧录 / 监视工具
```

## 当前支持

| 目标 | 状态 | 说明 |
|------|------|------|
| `rtthread` | 可用 | 构建 STM32F407 真板固件 |
| `sim` | 可用 | 构建并运行 QEMU 验证链路 |

## 快速开始

构建真板固件：

```bash
python3 project.py build -p rtthread
```

构建 QEMU 固件：

```bash
python3 project.py build -p sim
```

启动 QEMU 监视面板：

```bash
python3 project.py monitor -p sim
```

烧录真板：

```bash
python3 project.py flash -p rtthread
```

串口监视：

```bash
python3 project.py monitor -p rtthread -d /dev/ttyACM0
```

## 运行时命令

固件启动后，FinSH 可执行：

```text
maix_info
maix_state
```

它们分别输出当前运行时能力边界和基本状态。

## 真实边界

QEMU 只用于它真实支持的能力验证。当前不会在 QEMU 中伪造以下结果：

- LED
- GPIO 可见外设行为
- 摄像头
- 显示
- Python VM
- 模型推理

## 下一步

要把这个仓库继续做成真正“类似 MaixPy”的产品，下一阶段必须按顺序完成：

1. 接入嵌入式 Python VM。
2. 接入 VFS 和脚本启动链。
3. 接入模型文件装载与推理后端。
4. 用第二种架构板卡做真实移植，再谈跨架构统一。
