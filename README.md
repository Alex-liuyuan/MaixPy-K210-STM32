# SYSU_AIOTOS

面向 `ARM Cortex-M` 与 `RISC-V` 的跨架构 AIoT 操作系统框架原型，统一运行于 `RT-Thread Nano`。

当前仓库的产品边界是：

- `架构层`：兼容 `ARM Cortex-M` 与 `RISC-V`
- `系统层`：统一 `RT-Thread Nano`
- `运行时层`：统一 `MicroPython + VFS + 模型加载 + AI API`
- `板级层`：`STM32F407` 与 `K210` 只是首批参考实现

当前仓库里最成熟的一条真实可交付主线仍然是：

- 真板：`STM32F407 Nucleo + RT-Thread Nano`
- 仿真：`QEMU olimex-stm32-h405`

同时，`RISC-V / K210` 方向已经进入“真实可构建，但仍属实验”的阶段：

- 官方 `kendryte-standalone-sdk` 已纳入仓库期望路径
- 官方 RISC-V GNU toolchain 已纳入 `third_party/toolchains/`
- 可通过 `python3 project.py k210-probe` 真实验证 SDK/toolchain 构建链
- 可通过 `python3 project.py build -p k210` 真实生成 `K210 + RT-Thread Nano + SYSU_AIOTOS` 实验固件

但这还不是已经过实板验证的 `RISC-V` 产品主线，只能算首个 `RISC-V` 参考板落地样例。

这条主线已经具备：

- 固件构建
- ST-Link + OpenOCD 烧录入口
- 串口 / semihosting 日志
- 统一的产品运行时骨架
- QEMU 最小可验证链路
- 固件内置只读脚本包（`/boot.py`、`/main.py`）
- 嵌入式 MicroPython 最小端口
- MicroPython 端口构建参数化，不再把交叉前缀、架构参数、平台宏、BSP 头文件路径写死在 STM32F407
- 固件内只读脚本包已接到 MicroPython 的 `open()` / `import` 路径
- `boot.py` 与内置 app 脚本真实执行
- Python / 模型后端探测与阻塞原因输出
- 明确的能力声明，不伪造 AI / 摄像头 / 显示成功结果

当前还没有完成：

- 可写 VFS / 外部存储挂载
- 模型文件加载
- 真正的板上模型推理闭环

## 分层模型

- `Arch`：ARM Cortex-M / RISC-V
- `OS`：RT-Thread Nano
- `Board`：板级适配与烧录方式
- `Runtime`：MicroPython / VFS / 模型运行时
- `Tooling`：构建 / 烧录 / 监视 / 仿真

更正式的设计说明见：

- `reference/docs/architecture.md`
- `reference/docs/board_schema.md`
- `reference/docs/project_layout.md`

## 仓库结构

```text
boards/                           板卡元数据注册
cmake/                            CMake 与工具链配置
components/                       核心组件实现
maix/                             主机侧 API 与测试辅助
platforms/                        平台专用入口（STM32 / QEMU / K210）
reference/                        参考文档与示例
runtime/                          产品运行时骨架
tests/                            主机回归测试
third_party/                      第三方源码、工具链与离线归档
tools/                            构建 / 烧录 / 监视工具
```

仓库当前已经清理掉构建产物、缓存目录和未接入主链的旧脚本；工作树里不再保留 `build/`、`__pycache__/`、`.pytest_cache/` 这类噪音目录。

## 当前支持

| 目标 | 状态 | 说明 |
|------|------|------|
| `rtthread` | 可用 | 当前 ARM Cortex-M 参考板固件目标 |
| `sim` | 可用 | 构建并运行 QEMU 验证链路 |
| `k210` | 实验 | 当前 RISC-V 参考板固件目标；统一烧录入口依赖 `kflash_py` |

当前 `boards/` 中的板卡不应被理解为“项目边界”，而应被理解为“跨架构框架的首批参考板”。

## 快速开始

构建真板固件：

```bash
python3 project.py build -p rtthread
```

构建 QEMU 固件：

```bash
python3 project.py build -p sim
```

构建 K210 实验固件：

```bash
python3 project.py build -p k210
```

启动 QEMU 监视面板：

```bash 
python3 project.py monitor -p sim
```

烧录真板：

```bash
python3 project.py flash -p rtthread
```

烧录 K210 实验固件：

```bash
python3 project.py flash -p k210 -d /dev/ttyUSB0
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
maix_fs
maix_py
maix_model
```

它们分别输出运行时能力边界、基本状态、内置脚本包、Python 后端状态和模型后端状态。

## 真实边界

QEMU 只用于它真实支持的能力验证。当前不会在 QEMU 中伪造以下结果：

- LED
- GPIO 可见外设行为
- 摄像头
- 显示
- 模型推理

K210 当前也只承诺以下真实边界：

- 已验证官方 SDK/toolchain 构建链
- 已验证本仓库 K210 RT-Thread Nano 固件可以真实产出包含 MicroPython 端口的 `ELF/HEX/BIN`
- 已把 MicroPython 端口构建层改成平台参数驱动，且不再把指针宽度固定死在 32 位
- 已接入实验性的统一 K210 烧录 CLI，但本机仍需提供 `kflash_py`
- 尚未在真实 K210 开发板上完成启动、串口、FinSH、Python 运行时回归
- 尚未在真实 K210 开发板上完成烧录后回归

## 当前新增的真实资源

当前固件已经包含一个只读脚本包，并且已经能由最小嵌入式 MicroPython 端口真实执行：

- `/boot.py`
- `/main.py`
- `/maixapp/apps/sysu_aiotos_demo/main.py`
- `/models/README.txt`

这些资源目前仍然是固件内只读资源，不是完整可写 VFS。

## 下一步

要把这个仓库继续做成真正可跨板演进的 `SYSU_AIOTOS` 产品底座，下一阶段必须按顺序完成：

1. 接入可写 VFS 与外部模型目录。
2. 接入模型文件装载与推理后端。
3. 补齐 `maix` 运行时 API，而不是只保留最小 `maix.app`。
4. 把当前 `STM32F407/K210` 的板级经验收敛成可复制的 `ARM/RISC-V` 新板迁移规范。
