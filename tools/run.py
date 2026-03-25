#!/usr/bin/env python3
"""
统一入口脚本：自动检测平台、构建、烧录、运行示例（一个命令完成）。

用法示例：
  python3 tools/run.py --board stm32f407_nucleo --build
  python3 tools/run.py --board stm32f407_nucleo --flash
  python3 tools/run.py --list-boards
"""

import argparse
import json
import shutil
import subprocess
import sys
import time
from glob import glob
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARDS_DIR = ROOT / "boards"


def load_board(board_name: str) -> dict:
    board_path = BOARDS_DIR / f"{board_name}.json"
    if not board_path.exists():
        raise FileNotFoundError(f"板卡配置不存在: {board_path}")
    return json.loads(board_path.read_text(encoding="utf-8"))


def list_boards() -> list:
    return [p.stem for p in BOARDS_DIR.glob("*.json")]


def board_status(board: dict) -> str:
    return str(board.get("status", "stable")).lower()


def ensure_board_actionable(board: dict, action: str) -> int:
    status = board_status(board)
    if status == "stable":
        return 0

    print(f"[ERR] 板卡 {board.get('name', '<unknown>')} 当前状态为 {status}，不能执行 {action}")
    print("[ERR] 当前项目只对已落地的 RT-Thread Nano 板级链路提供真实构建/烧录支持")
    return 1


def detect_toolchain(tool_name: str) -> str | None:
    return shutil.which(tool_name)


def detect_serial_port() -> str | None:
    candidates = sorted(glob("/dev/ttyUSB*") + glob("/dev/ttyACM*"))
    return candidates[0] if candidates else None


def stm32_artifact_basename(platform: str) -> str:
    return f"MaixPy_{platform}"


def find_stm32_firmware(build_dir: Path, platform: str) -> Path | None:
    """查找可烧录的 STM32 固件。"""
    expected = [
        build_dir / f"{stm32_artifact_basename(platform)}.bin",
        build_dir / "maix_hal_module.bin",
    ]

    seen = set()
    for candidate in expected:
        if candidate.exists():
            return candidate
        seen.add(candidate)

    bin_files = sorted(
        build_dir.glob("*.bin"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for candidate in bin_files:
        if candidate not in seen:
            return candidate
    return None


def run_cmd(cmd: list[str], cwd: Path | None = None) -> int:
    print("[RUN]", " ".join(cmd))
    res = subprocess.run(cmd, cwd=str(cwd) if cwd else None)
    return res.returncode


def build_stm32(platform: str) -> int:
    toolchain = detect_toolchain("arm-none-eabi-gcc")
    if not toolchain:
        print("[ERR] 未检测到 arm-none-eabi-gcc")
        return 1

    build_dir = ROOT / "build" / platform
    cmake_args = [
        "cmake",
        f"-DPLATFORM={platform}",
        "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake",
        "-B",
        str(build_dir),
        "-S",
        str(ROOT),
    ]
    if run_cmd(cmake_args) != 0:
        return 1
    return run_cmd(["cmake", "--build", str(build_dir)])


def build_qemu() -> int:
    build_dir = ROOT / "build" / "sim"
    if run_cmd(["cmake", "-S", str(ROOT / "sim"), "-B", str(build_dir)]) != 0:
        return 1
    return run_cmd(["cmake", "--build", str(build_dir)])


def flash_stm32(board: dict) -> int:
    if not detect_toolchain("openocd"):
        print("[ERR] 未检测到 openocd，请安装后再试")
        return 1

    openocd = board.get("openocd", {})
    interface = openocd.get("interface", "interface/stlink.cfg")
    target = openocd.get("target", "target/stm32f4x.cfg")
    address = openocd.get("address", "0x08000000")
    build_dir = ROOT / "build" / board["platform"]
    bin_path = find_stm32_firmware(build_dir, board["platform"])
    if not bin_path:
        print(f"[ERR] 未在 {build_dir} 中找到可烧录固件 (*.bin)")
        return 1

    cmd = [
        "openocd",
        "-f",
        interface,
        "-f",
        target,
        "-c",
        f"program {bin_path} {address} verify reset exit",
    ]
    return run_cmd(cmd)


def monitor_serial(port: str | None, baudrate: int = 115200) -> int:
    try:
        import serial
    except ImportError:
        print("[ERR] 未安装 pyserial，请先执行: pip install pyserial")
        return 1

    if not port:
        port = detect_serial_port()
    if not port:
        print("[ERR] 未检测到串口设备 (/dev/ttyUSB* 或 /dev/ttyACM*)")
        return 1

    print(f"[MON] 打开串口 {port} @ {baudrate}bps，按 Ctrl+C 退出")
    try:
        with serial.Serial(port, baudrate=baudrate, timeout=0.2) as ser:
            while True:
                data = ser.read(256)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
                else:
                    time.sleep(0.05)
    except KeyboardInterrupt:
        print("\n[MON] 已退出")
        return 0
    except serial.SerialException as exc:
        print(f"[ERR] 串口打开失败: {exc}")
        return 1


def run_qemu_monitor(elf_path: Path | None = None) -> int:
    elf = elf_path or (ROOT / "build" / "sim" / "MaixPy_sim.elf")
    if not elf.exists():
        print(f"[ERR] 未找到 QEMU ELF: {elf}")
        print("[ERR] 请先执行 --qemu-build")
        return 1
    return run_cmd([sys.executable, str(ROOT / "tools" / "qemu_monitor.py"), "--elf", str(elf)])


def main() -> int:
    parser = argparse.ArgumentParser(description="MaixPy Nano RT-Thread 一键构建/烧录/运行入口")
    parser.add_argument("--board", help="板卡名（位于 boards/*.json）")
    parser.add_argument("--list-boards", action="store_true", help="列出可用板卡")
    parser.add_argument("--build", action="store_true", help="构建固件")
    parser.add_argument("--flash", action="store_true", help="烧录固件")
    parser.add_argument("--monitor", action="store_true", help="打开串口监视器")
    parser.add_argument("--qemu-build", action="store_true", help="构建 QEMU 仿真固件")
    parser.add_argument("--qemu-run", action="store_true", help="启动 QEMU 监控面板")
    parser.add_argument("--port", help="串口设备")
    parser.add_argument("--baud", type=int, default=115200, help="串口波特率")
    args = parser.parse_args()

    if args.list_boards:
        print("可用板卡:")
        for b in list_boards():
            board = load_board(b)
            print(f" - {b} [{board_status(board)}]")
        return 0

    if args.qemu_build:
        rc = build_qemu()
        if rc != 0 or not args.qemu_run:
            return rc

    if args.qemu_run:
        return run_qemu_monitor()

    if not args.board:
        print("[ERR] 请使用 --board 指定板卡")
        return 1

    board = load_board(args.board)
    platform = board.get("platform")

    if args.build:
        if ensure_board_actionable(board, "build") != 0:
            return 1
        if build_stm32(platform) != 0:
            return 1

    if args.flash:
        if ensure_board_actionable(board, "flash") != 0:
            return 1
        rc = flash_stm32(board)
        if rc != 0 or not args.monitor:
            return rc

    if args.monitor:
        default_baud = board.get("uart", {}).get("baud", args.baud)
        return monitor_serial(args.port, default_baud)

    if not args.build and not args.flash and not args.monitor and not args.qemu_build and not args.qemu_run:
        print("[INFO] 未指定动作，请使用 --build、--flash、--monitor、--qemu-build 或 --qemu-run")
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
