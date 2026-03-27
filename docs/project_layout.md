# 项目结构说明

当前仓库按“框架实现 / 板级实现 / 主机工具 / 参考源码”四类组织，避免把示例、缓存、构建产物和正式代码混在一起。

## 顶层目录

- `runtime/`
  统一运行时入口，包含启动状态、内置脚本包、Python 运行时、模型运行时骨架。

- `boards/`
  板卡元数据注册。这里只放板卡描述，不放具体驱动实现。

- `cmake/`
  主机构建配置、工具链配置、MicroPython/TFLM 接入脚本。

- `components/`
  核心组件实现。
  当前包括：
  `hal/` 通用 HAL 抽象
  `drivers/stm32/` ARM Cortex-M 参考驱动
  `drivers/k210/` RISC-V K210 参考驱动
  `micropython_rtthread/` RT-Thread Nano 下的 MicroPython 端口

- `docs/`
  设计、结构、移植和快速开始文档。

- `examples/`
  面向上层 API 的最小示例脚本。这里保留的是“用户示例”，不是开发残留脚本。

- `maix/`
  主机侧 Python API 包与工具封装，用于接口整理和测试。

- `platforms/`
  平台专用入口目录。
  `platforms/sim/` 为 QEMU 仿真入口
  `platforms/k210/` 为 K210 实验性固件入口
  `platforms/stm32/` 为 STM32 专用启动文件与链接脚本

- `tests/`
  主机侧回归测试与 mock HAL。

- `third_party/`
  第三方依赖与离线资源。
  `rtthread-nano/` RT-Thread Nano 内核与现有 BSP 参考代码
  `toolchains/` 当前实际使用的交叉工具链
  `archives/` 仅保留归档包，不参与主线构建

- `tools/`
  主机侧命令行工具。当前只保留实际接入主链的工具：
  `run.py`
  `qemu_monitor.py`

## 顶层文件

- `project.py`
  项目统一入口。

- `run_windows.bat`
  Windows 下的最小入口包装。

- `README.md`
  仓库总览。

## 已清理的内容

本轮已移除以下不再接入主链、容易干扰理解的内容：

- `build/` 构建产物
- `__pycache__/` 与 `.pytest_cache/`
- 未接入主链的旧脚本：`tools/quick_test.py`
- 未接入主链的可视化脚本：`tools/filter_viz.py`
- 未被使用的 host shim：`bindings/_maix_hal_shim.py`
- 过时的中文批处理入口，已替换为 `run_windows.bat`

## 目录约束

后续继续整理时，遵循下面三条：

1. 构建产物不进入仓库结构说明，也不保留在工作树中。
2. 临时验证脚本如果不会进入主链，就不要放在根目录或 `tools/`。
3. 新板卡优先补 `boards/*.json` 和板级实现，不再额外开“独立小项目式”目录。
