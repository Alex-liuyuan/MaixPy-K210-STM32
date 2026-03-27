# SYSU_AIOTOS

`SYSU_AIOTOS` 是一个面向 `ARM Cortex-M` 与 `RISC-V` 的跨架构 AIoT 系统原型仓库。当前统一系统层为 `RT-Thread Nano`，并在其上逐步收敛 `MicroPython`、运行时资源包、模型运行时骨架和上层 `maix` API。

当前最明确的一条主线是：

- 真板：`STM32F407 Nucleo + RT-Thread Nano`
- 仿真：`QEMU olimex-stm32-h405`
- 实验链路：`K210 + RT-Thread Nano`

这个仓库已经能提供统一构建入口、板级/仿真运行入口、运行时资源打包、主机侧 API 验证与回归测试，但它还不是一个已经完成板上 AI 闭环的成品系统。

## 当前状态

| 目标 | 状态 | 说明 |
|------|------|------|
| `rtthread` | 可用 | 当前 ARM Cortex-M 参考板固件目标 |
| `sim` | 可用 | 当前 QEMU 仿真验证目标 |
| `k210` | 实验 | 当前 RISC-V 参考板实验目标 |

当前已经具备：

- 统一 CLI 入口：`project.py`
- STM32 真板构建、烧录、串口监视
- QEMU 仿真构建与监视
- K210 SDK/toolchain 探测与实验性固件构建
- 固件内置只读运行时资源包：`runtime_bundle/`
- 主机侧 `maix` API 包与 pytest 回归测试

当前仍未完成：

- 可写 VFS / 外部存储挂载
- 板上模型文件加载闭环
- 板上真实 AI 推理闭环
- K210 实板启动、串口、FinSH、Python 运行时回归

## 快速开始

建议先初始化子模块并准备 Python 依赖：

```bash
git submodule update --init --recursive
python3 -m pip install -r requirements.txt
```

查看支持的平台：

```bash
python3 project.py list-platforms
```

构建 STM32 真板固件：

```bash
python3 project.py build -p rtthread
```

烧录 STM32 真板：

```bash
python3 project.py flash -p rtthread
```

监视 STM32 串口：

```bash
python3 project.py monitor -p rtthread -d /dev/ttyACM0
```

构建 QEMU 固件：

```bash
python3 project.py build -p sim
```

启动 QEMU 监视面板：

```bash
python3 project.py monitor -p sim
```

探测 K210 SDK/toolchain：

```bash
python3 project.py k210-probe
```

构建 K210 实验固件：

```bash
python3 project.py build -p k210
```

烧录 K210 实验固件：

```bash
python3 project.py flash -p k210 -d /dev/ttyUSB0
```

更新运行时资源包中的应用与模型：

```bash
python3 project.py bundle --app-dir reference/examples/basic --model /tmp/model.tflite --labels /tmp/labels.txt
```

查看当前运行时资源包内容：

```bash
python3 project.py bundle-info
```

## 目录分区

当前仓库顶层按“系统实现 / 参考资料 / 测试回归”三块职责组织：

```text
boards/                           板卡元数据
cmake/                            构建配置与工具链脚本
components/                       核心组件实现
maix/                             主机侧 Python API
platforms/                        平台入口（STM32 / QEMU / K210）
reference/                        参考文档与示例
runtime/                          运行时骨架
runtime_bundle/                   固件内置只读资源包
tests/                            主机侧回归测试
third_party/                      第三方源码、工具链与离线归档
tools/                            构建 / 烧录 / 监视辅助工具
project.py                        仓库统一入口
```

其中：

- `reference/docs/` 放设计、结构、移植、FAQ 与快速开始文档
- `reference/examples/` 放示例脚本
- `tests/` 只放主机侧回归与 mock
- `third_party/` 与 `MaixPy-v1/` 放上游依赖和参考源码，不承载主系统入口职责

## 能力边界

QEMU 只用于它真实支持的能力验证，当前不会伪造以下结果：

- LED 可见行为
- 摄像头
- 显示
- 模型推理

K210 方向当前也只承诺：

- 已接上官方 SDK/toolchain 探测路径
- 已能生成实验性固件产物
- 已接上统一 CLI 的实验性烧录入口

它目前不代表已经完成了真实开发板上的稳定回归。

## 进一步阅读

- [reference/docs/architecture.md](reference/docs/architecture.md)
- [reference/docs/board_schema.md](reference/docs/board_schema.md)
- [reference/docs/project_layout.md](reference/docs/project_layout.md)
- [reference/docs/hardware_porting.md](reference/docs/hardware_porting.md)
- [reference/docs/quickstart.md](reference/docs/quickstart.md)
- [reference/docs/faq.md](reference/docs/faq.md)
- [reference/README.md](reference/README.md)

## 说明

- Windows 下可使用 `run_windows.bat` 作为最小入口包装。
- 构建产物、缓存目录和临时验证文件不应进入版本库。
- 顶层板卡目录 `boards/` 表示“首批参考板注册”，不代表产品边界。
