#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""项目统一构建入口。"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent

if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.run import build_qemu, build_stm32, flash_stm32, load_board, monitor_serial, run_qemu_monitor


PLATFORM_TO_BOARD = {
    "rtthread": "stm32f407_nucleo",
}


def get_supported_platforms():
    return ["rtthread", "sim"]


def default_board_for_platform(platform: str) -> str | None:
    return PLATFORM_TO_BOARD.get(platform)


def clean_build_dir(platform: str) -> int:
    target = ROOT / "build" / platform
    if target.exists():
        shutil.rmtree(target)
        print(f"[CLEAN] 已删除 {target}")
    else:
        print(f"[CLEAN] 无需清理，目录不存在: {target}")
    return 0


def cmd_build(platform: str) -> int:
    if platform == "sim":
        return build_qemu()
    return build_stm32(platform)


def cmd_flash(platform: str, device: str | None) -> int:
    board_name = default_board_for_platform(platform)
    if not board_name:
        print(f"[ERR] 平台 {platform} 没有默认板卡配置，请使用 tools/run.py --board ...")
        return 1

    board = load_board(board_name)
    return flash_stm32(board)


def cmd_monitor(platform: str, device: str | None, baudrate: int) -> int:
    if platform == "sim":
        return run_qemu_monitor()
    board_name = default_board_for_platform(platform)
    if board_name:
        board = load_board(board_name)
        baudrate = board.get("uart", {}).get("baud", baudrate)
    return monitor_serial(device, baudrate)


def main():
    parser = argparse.ArgumentParser(description="MaixPy Nano RT-Thread 项目构建工具")
    parser.add_argument("command", nargs="?", default="help",
                        choices=["help", "menuconfig", "build", "clean", "distclean", "flash", "monitor", "list-platforms"])
    parser.add_argument("-p", "--platform", default="rtthread", help="目标平台")
    parser.add_argument("-d", "--device", help="串口设备")
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="串口波特率")
    parser.add_argument("--config-file", help="兼容旧 CLI，当前保留不使用")
    parser.add_argument("--release", action="store_true", help="兼容旧 CLI，当前保留不使用")
    args = parser.parse_args()

    if args.command == "help":
        parser.print_help()
        print("\n示例:")
        print("  python project.py build -p rtthread")
        print("  python project.py build -p sim")
        print("  python project.py flash -p rtthread")
        print("  python project.py monitor -p rtthread -d /dev/ttyACM0")
        print("\n说明:")
        print("  当前主线只对 RT-Thread Nano STM32 路径和 QEMU 验证路径提供真实支持。")
        return 0

    if args.command == "list-platforms":
        print("支持的平台:")
        for platform in get_supported_platforms():
            print(f"  - {platform}")
        return 0

    if args.command == "menuconfig":
        print("[INFO] 当前项目使用 CMake/boards 配置，无独立 menuconfig。")
        print("[INFO] 如需切换目标，请使用 -p/--platform 或 tools/run.py --board。")
        return 0

    if args.command in {"clean", "distclean"}:
        return clean_build_dir(args.platform)

    if args.command == "build":
        return cmd_build(args.platform)

    if args.command == "flash":
        return cmd_flash(args.platform, args.device)

    if args.command == "monitor":
        return cmd_monitor(args.platform, args.device, args.baudrate)

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
