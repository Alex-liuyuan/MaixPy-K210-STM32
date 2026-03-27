# 项目结构说明

跨平台边缘 AI 开发框架，支持 K210 (RISC-V) 和 STM32 (ARM Cortex-M)。

## 顶层目录

```
MaixPy-K210-STM32/
├── maix/                          # Python 包（核心 API）
├── components/
│   ├── hal/                       # C HAL 抽象层
│   ├── drivers/
│   │   ├── stm32/                 # STM32 平台驱动
│   │   └── k210/                  # K210 平台驱动
│   └── micropython_rtthread/      # MicroPython RT-Thread 端口
├── runtime/                       # 固件运行时（启动、文件系统、AI 后端）
├── runtime_bundle/                # 运行时资源包
├── platforms/
│   ├── stm32/                     # STM32 链接脚本、启动文件
│   ├── k210/                      # K210 板级代码
│   └── sim/                       # QEMU 仿真
├── cmake/                         # CMake 工具链和模块
├── tools/                         # 烧录、监控等工具脚本
├── boards/                        # 板卡 JSON 配置
├── tests/                         # pytest 测试
├── reference/
│   ├── docs/                      # 文档
│   └── examples/                  # 示例代码
├── third_party/
│   ├── rtthread-nano/             # RT-Thread Nano（直接包含）
│   └── tflite-micro/              # TFLite Micro（submodule）
├── MaixPy-v1/                     # MaixPy v1 参考（submodule）
├── CMakeLists.txt                 # 顶层构建
├── project.py                     # 构建入口脚本
├── setup.py                       # Python 打包
├── pyproject.toml                 # 现代打包配置
├── requirements.txt               # Python 依赖
├── pytest.ini                     # 测试配置
└── README.md
```

## 各目录职责

- `maix/` — 主机侧 Python API 包，包含 camera、display、nn、GPIO、SPI、I2C、UART、PWM、ADC 等模块
- `components/hal/` — C 语言 HAL 抽象层，通过 ops 结构体模式注册平台驱动
- `components/drivers/` — 平台专用驱动实现（STM32 HAL、K210 SDK）
- `runtime/` — 统一运行时入口，包含启动状态、内置脚本包、Python/AI 运行时
- `platforms/` — 平台专用入口，链接脚本和启动文件
- `boards/` — 板卡元数据（JSON 配置）
- `cmake/` — 构建配置、工具链文件、MicroPython/TFLM 集成脚本
- `tests/` — 主机侧 pytest 回归测试与 mock HAL
- `reference/` — 文档和示例代码
- `tools/` — 主机侧命令行工具（`run.py`、`qemu_monitor.py`）

## third_party 说明

- `rtthread-nano/` — RT-Thread Nano 内核与 BSP 参考代码，直接包含在仓库中
- `tflite-micro/` — TensorFlow Lite Micro，作为 git submodule 引用
- `MaixPy-v1/` — MaixPy v1 参考实现，作为 git submodule 引用
- 工具链二进制不再随仓库分发，需自行下载到 `third_party/toolchains/`（详见 [quickstart.md](quickstart.md)）

## 目录约束

1. 构建产物（`build/`）不进入版本控制
2. 工具链、压缩包等二进制文件通过 `.gitignore` 排除
3. 新板卡优先补 `boards/*.json` 和板级驱动，不另开独立目录
