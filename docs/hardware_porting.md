# 硬件适配指南

## 当前状态

仓库当前优先维护的真实板卡路径是 `stm32f407_nucleo + RT-Thread`。

## 新增板卡时至少要处理的内容

1. 新增 `boards/*.json` 板卡描述。
2. 补充对应的时钟、启动文件、链接脚本和 CMake 平台配置。
3. 明确 LED、调试串口、烧录器连接方式。
4. 根据硬件实际连线修改外设 MSP 初始化。
5. 在文档里写清楚“上电后可见现象”和“串口查看方式”。

## STM32 侧关键位置

- 板级应用入口：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Core/Src/main.c`
- UART MSP：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Core/Src/stm32f4xx_hal_msp.c`
- RT-Thread 控制台：`rtthread-nano-master/rt-thread/bsp/stm32f407-msh/Middlewares/Third_Party/RealThread_RTOS/bsp/_template/cubemx_config/board.c`
- 板卡模板：`boards/`

## 建议流程

先让 `LED + console UART + flash` 跑通，再接入 SPI/I2C/UART/PWM/ADC，最后再做摄像头、显示和 AI。
