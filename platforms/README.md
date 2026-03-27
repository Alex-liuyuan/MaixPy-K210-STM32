# 平台目录

- `platforms/sim/`
  QEMU 仿真入口与仿真专用启动/板级覆盖。

- `platforms/k210/`
  K210 实验性固件入口与板级实现。

顶层 `CMakeLists.txt` 继续维护 STM32/RT-Thread 主线，
其余平台由各自子目录负责独立配置。
