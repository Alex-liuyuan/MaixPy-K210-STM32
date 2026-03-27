# 硬件适配指南

## 当前状态

仓库当前优先维护的真实板卡路径是：

- `ARM Cortex-M / stm32f407_nucleo / RT-Thread Nano`
- `RISC-V / k210_generic / RT-Thread Nano`（实验）

但它们只是首批参考板，不是产品边界。

## 新板卡接入原则

新增板卡时，按下面三层思路接入：

1. 先确认它属于哪个 `arch_family`
2. 再确认它属于哪个 `soc_family`
3. 最后补具体 `board` 连线与烧录方式

不要直接把新板卡做成一个“独立小项目”。

## 新增板卡时至少要处理的内容

1. 新增 `boards/*.json` 板卡描述。
2. 补充对应的时钟、启动文件、链接脚本和 CMake 目标配置。
3. 明确 LED、调试串口、烧录器连接方式。
4. 处理脚本存储方案，至少说明固件内置脚本包、片上 Flash 分区或外部存储如何落地。
5. 根据硬件实际连线修改外设 MSP 初始化。
6. 在文档里写清楚“上电后可见现象”和“串口查看方式”。
7. 明确它是 `stable`、`experimental` 还是 `planned`。

## 建议的迁移顺序

建议按以下顺序推进：

1. `board metadata`
2. `build + linker + startup`
3. `console uart + heartbeat`
4. `flash`
5. `bundled Python scripts`
6. `可写 VFS`
7. `模型加载与 AI 推理`

## STM32 侧关键位置

- 板级应用入口：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Core/Src/main.c`
- UART MSP：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Core/Src/stm32f4xx_hal_msp.c`
- RT-Thread 控制台：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Middlewares/Third_Party/RealThread_RTOS/bsp/_template/cubemx_config/board.c`
- 板卡模板：`boards/`

## K210 侧关键位置

- 板级入口：`platforms/k210/main.c`
- 板级 profile：`platforms/k210/board.c`
- 链接脚本适配：`platforms/k210/kendryte_rtthread.ld`
- SDK 探测与统一构建：`tools/run.py`、`project.py`

## 建议流程

先让 `LED + console UART + flash` 跑通，再接入只读脚本包或外部存储，再迁移最小 MicroPython 端口，最后接模型后端、摄像头和显示。
