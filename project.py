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

from tools.run import (
    build_k210,
    build_k210_sdk_probe,
    build_qemu,
    build_stm32,
    flash_k210,
    flash_stm32,
    load_board,
    monitor_serial,
    run_qemu_monitor,
)
from tools.runtime_bundle import DEFAULT_BUNDLE_DIR, stage_runtime_bundle
from tools.runtime_bundle import summarize_bundle


PLATFORM_TO_BOARD = {
    "rtthread": "stm32f407_nucleo",
    "k210": "k210_generic",
}


def get_supported_platforms():
    return ["rtthread", "sim", "k210"]


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
    if platform == "k210":
        return build_k210()
    return build_stm32(platform)


def cmd_flash(platform: str, device: str | None) -> int:
    if platform == "k210":
        board = load_board("k210_generic")
        return flash_k210(board, device)

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


def cmd_bundle(app_dir: str,
               model: str | None,
               labels: str | None,
               manifest: str | None,
               app_id: str | None,
               reset_app: bool) -> int:
    stage_runtime_bundle(
        app_dir=Path(app_dir),
        bundle_dir=DEFAULT_BUNDLE_DIR,
        model_path=Path(model) if model else None,
        labels_path=Path(labels) if labels else None,
        manifest_path=Path(manifest) if manifest else None,
        app_id=app_id,
        reset_app=reset_app,
    )
    print(f"[BUNDLE] 已更新运行时资源目录: {DEFAULT_BUNDLE_DIR}")
    return 0


def cmd_bundle_info() -> int:
    summary = summarize_bundle(DEFAULT_BUNDLE_DIR)
    print(f"[BUNDLE] files={summary['file_count']} bytes={summary['total_bytes']}")
    for path in summary["files"]:
        print(f"[BUNDLE] {path}")
    return 0


def main():
    parser = argparse.ArgumentParser(description="SYSU_AIOTOS 项目构建工具")
    parser.add_argument("command", nargs="?", default="help",
                        choices=["help", "menuconfig", "build", "clean", "distclean", "flash", "monitor", "list-platforms", "k210-probe", "bundle", "bundle-info"])
    parser.add_argument("-p", "--platform", default="rtthread", help="目标平台")
    parser.add_argument("-d", "--device", help="串口设备")
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="串口波特率")
    parser.add_argument("--app-dir", help="bundle 命令使用的 Python 应用目录")
    parser.add_argument("--model", help="bundle 命令使用的模型文件路径")
    parser.add_argument("--labels", help="bundle 命令使用的标签文件路径")
    parser.add_argument("--manifest", help="bundle 命令使用的模型 manifest 路径")
    parser.add_argument("--app-id", help="bundle 命令使用的应用 ID")
    parser.add_argument("--reset-app", action="store_true", help="bundle 命令时重置目标应用目录")
    parser.add_argument("--config-file", help="兼容旧 CLI，当前保留不使用")
    parser.add_argument("--release", action="store_true", help="兼容旧 CLI，当前保留不使用")
    args = parser.parse_args()

    if args.command == "help":
        parser.print_help()
        print("\n示例:")
        print("  python project.py build -p rtthread")
        print("  python project.py build -p sim")
        print("  python project.py build -p k210")
        print("  python project.py flash -p rtthread")
        print("  python project.py flash -p k210 -d /dev/ttyUSB0")
        print("  python project.py monitor -p rtthread -d /dev/ttyACM0")
        print("  python project.py k210-probe")
        print("  python project.py bundle --app-dir reference/examples/basic --model /tmp/model.tflite --labels /tmp/labels.txt")
        print("  python project.py bundle-info")
        print("\n说明:")
        print("  当前项目面向 ARM Cortex-M 与 RISC-V，统一使用 RT-Thread Nano。")
        print("  STM32F407 与 K210 只是首批参考板，其中 K210 仍属于实验性真构建链。")
        print("  K210 当前已接入依赖 kflash_py 的实验性统一烧录。")
        print("  bundle 命令会把 Python 应用和可选模型打包进统一运行时资源目录。")
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

    if args.command == "k210-probe":
        return build_k210_sdk_probe()

    if args.command == "bundle":
        if not args.app_dir:
            print("[ERR] bundle 命令需要 --app-dir")
            return 1
        return cmd_bundle(args.app_dir, args.model, args.labels, args.manifest, args.app_id, args.reset_app)

    if args.command == "bundle-info":
        return cmd_bundle_info()

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
