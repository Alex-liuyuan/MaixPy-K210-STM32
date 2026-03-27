# SYSU_AIOTOS 架构基线

## 产品定义

`SYSU_AIOTOS` 的目标不是做 “STM32 专用项目” 或 “K210 专用项目”，而是做一套：

- 面向 `ARM Cortex-M` 与 `RISC-V` 的跨架构嵌入式框架
- 统一使用 `RT-Thread Nano`
- 上层统一提供 `MicroPython + VFS + 模型加载 + AI API`
- 支持真板构建、烧录、串口回归与可验证仿真

当前仓库中的 `STM32F407 Nucleo` 与 `K210 Generic Board` 只是首批参考板级实现。

## 设计约束

必须长期坚持以下约束：

1. 一次构建只允许一个目标架构和一个板卡，禁止混用 ARM/RISC-V 编译参数。
2. 操作系统主线固定为 `RT-Thread Nano`，不再把“裸机无 OS”作为主产品分支。
3. Python API 和 AI API 不暴露芯片私有语义，板级差异只留在底层驱动和能力声明中。
4. 构建、烧录、串口监视、仿真入口必须统一到同一套主机工具链中。
5. 所有“已支持”能力都必须可验证；不能靠文档或日志伪造成功闭环。

## 分层模型

推荐按五层组织实现：

### 1. `Arch` 层

负责 CPU 架构相关差异：

- `arm-cortex-m`
- `riscv`

典型内容：

- 交叉工具链前缀
- 启动文件
- 链接脚本入口
- 中断/上下文切换相关代码
- 字长、ABI、编译参数

### 2. `OS` 层

当前只维护：

- `rtthread-nano`

典型内容：

- 调度与线程模型
- 控制台与 FinSH
- Tick/延时/同步原语
- 设备注册抽象

### 3. `Board` 层

板级是“某架构 + 某 SoC/MCU + 某连线方案”的具体实例，例如：

- `stm32f407_nucleo`
- `k210_generic`

板级只负责：

- 时钟和启动初始化
- LED/UART/Flash/外设引脚映射
- 烧录方式
- 串口默认波特率
- 外部存储与模型介质落点
- 运行时 profile 填充

### 4. `Runtime` 层

这是产品真正要统一的部分：

- `maix_runtime_app`
- `MicroPython RT-Thread port`
- 只读/可写 VFS
- 应用脚本加载
- 模型文件发现与装载
- 统一能力声明

要求：

- 同一套 Python 应用尽量不感知底层芯片
- 允许能力缺失，但必须明确返回 `planned` / `unavailable`

### 5. `Tooling` 层

主机侧统一入口负责：

- 板卡元数据读取
- 工具链探测
- 固件构建
- 产物命名与归档
- 烧录
- 串口监视
- 仿真启动

## 当前仓库与目标结构的关系

当前仓库已经具备部分分层基础，但仍有明显“首批板卡写死”的痕迹：

- `boards/` 已经承担板卡元数据入口
- `components/micropython_rtthread/` 已经开始变成跨平台运行时端口
- `components/drivers/stm32/` 与 `components/drivers/k210/` 仍然偏向 SoC 特化目录
- `components/hal/hal_common.h` 仍把平台枚举直接写成 `STM32F407/K210`

因此后续整改方向应该是：

1. 保留现有可用构建链，不为了“抽象漂亮”破坏现有真构建结果。
2. 新增板卡时优先扩 `boards/` 和构建元数据，不再新增“独立小项目式”目录。
3. 逐步把 `HAL_PLATFORM_STM32F407/HAL_PLATFORM_K210` 迁移为更通用的 `board/soc/arch` 描述。
4. 上层 runtime 只识别统一能力，不识别具体芯片名字。

## 首批参考实现

### `ARM Cortex-M` 参考板

- 板卡：`stm32f407_nucleo`
- 架构：`arm`
- 系统：`rtthread-nano`
- 当前状态：真板主线

### `RISC-V` 参考板

- 板卡：`k210_generic`
- 架构：`riscv64`
- 系统：`rtthread-nano`
- 当前状态：实验性真构建链

## 后续扩展原则

以后新增板卡时，遵循以下优先顺序：

1. 同架构新板优先
2. 同一运行时接口复用优先
3. 真构建和真烧录优先
4. AI/摄像头/显示等高阶能力后补

例如：

- `ARM Cortex-M`: STM32F411、STM32H743、GD32、ATSAMD
- `RISC-V`: K210 之外的其他 MCU/SoC

## 当前还缺的关键能力

这套“跨架构 Python AIoT 框架”距离完整产品还差：

1. 可写 VFS 与外部存储挂载
2. 统一模型目录与模型装载器
3. 至少一种真实可运行的板上 AI 推理后端
4. 更完整的 `sysu` Python API
5. 真板回归脚本，而不仅是主机侧构建测试

在这些能力落地之前，仓库可以叫“跨架构框架原型”，但还不能叫“完整可量产底座”。
