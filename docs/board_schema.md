# 板卡元数据规范

`boards/*.json` 是 `SYSU_AIOTOS` 的板级注册入口。新增板卡时，优先补板卡元数据，再补构建和驱动实现。

## 推荐字段

```json
{
  "name": "Human readable board name",
  "platform": "build target key",
  "status": "stable | experimental | planned",
  "arch": "arm | riscv64",
  "arch_family": "arm-cortex-m | riscv",
  "soc_family": "stm32f4 | kendryte-k210",
  "mcu": "specific part number",
  "os": "rtthread-nano",
  "build": {
    "artifact_prefix": "SYSU_AIOTOS_xxx"
  },
  "uart": {
    "baud": 115200
  }
}
```

## 字段语义

- `platform`
  当前构建目标键。必须和主机侧构建入口能够识别的目标一致。

- `status`
  板级成熟度，不允许在文档里把 `experimental` 写成“已完全支持”。

- `arch`
  当前工具链对应的架构关键字。

- `arch_family`
  对外表达用的架构家族名，避免以后把 `arm` 和 `arm-cortex-m7` 这种概念混在一起。

- `soc_family`
  SoC/MCU 家族名，用于沉淀后续驱动分层。

- `mcu`
  具体芯片型号。

- `os`
  当前统一固定为 `rtthread-nano`。

## 当前规则

当前仓库只接受：

- `RT-Thread Nano` 主线板卡
- 真构建可验证的板卡
- 烧录方式明确的板卡

不接受：

- 只有名字，没有启动文件和链接脚本的占位板卡
- 文档宣称支持，但没有构建产物或烧录路径的板卡

## 新增板卡最少交付物

1. `boards/<name>.json`
2. 对应的 CMake / toolchain / linker 配置
3. `maix_board_fill_profile()` 的板级实现
4. 烧录方式说明
5. 串口查看方式说明
6. 至少一次真实构建验证
