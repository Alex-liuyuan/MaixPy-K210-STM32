#!/usr/bin/env python3
"""
统一入口脚本：自动检测平台、构建、烧录、运行示例（一个命令完成）。

用法示例：
  python3 tools/run.py --board stm32f407_nucleo --build
  python3 tools/run.py --board stm32f407_nucleo --flash
  python3 tools/run.py --list-boards
"""

import argparse
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import time
from glob import glob
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARDS_DIR = ROOT / "boards"
K210_SDK_DIR = ROOT / "MaixPy-v1" / "components" / "kendryte_sdk" / "kendryte-standalone-sdk"
K210_TOOLCHAIN_BIN = ROOT / "third_party" / "toolchains" / "kendryte-toolchain" / "bin"
K210_SDK_LIB = ROOT / "build" / "k210_sdk_probe" / "lib" / "libkendryte.a"
K210_RTTHREAD_SDK_LIB = ROOT / "build" / "k210_sdk_probe" / "lib" / "libkendryte_rtthread.a"
K210_FLASH_SCRIPT = ROOT / "MaixPy-v1" / "tools" / "flash" / "flash.py"
K210_FLASH_VENDOR_DIR = ROOT / "MaixPy-v1" / "tools" / "flash"
K210_FLASH_VENDOR_MODULE = K210_FLASH_VENDOR_DIR / "kflash_py" / "kflash.py"


def load_board(board_name: str) -> dict:
    board_path = BOARDS_DIR / f"{board_name}.json"
    if not board_path.exists():
        raise FileNotFoundError(f"板卡配置不存在: {board_path}")
    return json.loads(board_path.read_text(encoding="utf-8"))


def list_boards() -> list:
    return [p.stem for p in BOARDS_DIR.glob("*.json")]


def board_status(board: dict) -> str:
    return str(board.get("status", "stable")).lower()


def board_summary(board_name: str, board: dict) -> str:
    arch = board.get("arch_family", board.get("arch", "unknown"))
    os_name = board.get("os", "unknown")
    status = board_status(board)
    return f"{board_name} [{status}] arch={arch} os={os_name} platform={board.get('platform', 'unknown')}"


def ensure_board_actionable(board: dict, action: str) -> int:
    status = board_status(board)
    if status == "stable":
        return 0

    if board.get("platform") == "k210" and action in {"build", "flash"}:
        return 0

    print(f"[ERR] 板卡 {board.get('name', '<unknown>')} 当前状态为 {status}，不能执行 {action}")
    print("[ERR] 当前项目只对已落地的 RT-Thread Nano 板级链路提供真实构建/烧录支持")
    return 1


def detect_toolchain(tool_name: str) -> str | None:
    return shutil.which(tool_name)


def k210_sdk_ready() -> bool:
    return (K210_SDK_DIR / "CMakeLists.txt").exists() and (K210_SDK_DIR / "src" / "hello_world" / "main.c").exists()


def k210_toolchain_ready() -> bool:
    return (K210_TOOLCHAIN_BIN / "riscv64-unknown-elf-gcc").exists()


def k210_sdk_library_ready() -> bool:
    return K210_SDK_LIB.exists()


def k210_rtthread_sdk_library_ready() -> bool:
    return K210_RTTHREAD_SDK_LIB.exists()


def k210_flash_script_ready() -> bool:
    return K210_FLASH_SCRIPT.exists()


def k210_kflash_module_ready() -> bool:
    if K210_FLASH_VENDOR_MODULE.exists():
        return True
    return importlib.util.find_spec("kflash_py") is not None


def detect_serial_port() -> str | None:
    candidates = sorted(glob("/dev/ttyUSB*") + glob("/dev/ttyACM*"))
    return candidates[0] if candidates else None


def stm32_artifact_basename(platform: str) -> str:
    return f"SYSU_AIOTOS_{platform}"


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


def run_cmd(cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> int:
    print("[RUN]", " ".join(cmd))
    res = subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env)
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
    if run_cmd(["cmake", "-S", str(ROOT / "platforms" / "sim"), "-B", str(build_dir)]) != 0:
        return 1
    return run_cmd(["cmake", "--build", str(build_dir)])


def build_k210_sdk_probe() -> int:
    if not k210_sdk_ready():
        print(f"[ERR] 未找到 K210 SDK: {K210_SDK_DIR}")
        print("[ERR] 请先准备 MaixPy-v1/components/kendryte_sdk/kendryte-standalone-sdk")
        return 1

    if not k210_toolchain_ready():
        print(f"[ERR] 未找到 Kendryte RISC-V 工具链: {K210_TOOLCHAIN_BIN}")
        print("[ERR] 需要 riscv64-unknown-elf-gcc 等工具")
        return 1

    build_dir = ROOT / "build" / "k210_sdk_probe"
    if run_cmd([
        "cmake",
        "-S", str(K210_SDK_DIR),
        "-B", str(build_dir),
        "-DPROJ=hello_world",
        f"-DTOOLCHAIN={K210_TOOLCHAIN_BIN}",
    ]) != 0:
        return 1

    return run_cmd(["cmake", "--build", str(build_dir)])


def build_k210() -> int:
    if build_k210_sdk_probe() != 0:
        return 1

    if not k210_sdk_library_ready():
        print(f"[ERR] K210 SDK 库未生成: {K210_SDK_LIB}")
        return 1

    shutil.copy2(K210_SDK_LIB, K210_RTTHREAD_SDK_LIB)
    if run_cmd([
        str(K210_TOOLCHAIN_BIN / "riscv64-unknown-elf-ar"),
        "d",
        str(K210_RTTHREAD_SDK_LIB),
        "crt.S.obj",
        "entry_user.c.obj",
    ]) != 0:
        return 1

    build_dir = ROOT / "build" / "k210"
    if run_cmd([
        "cmake",
        "-S", str(ROOT / "platforms" / "k210"),
        "-B", str(build_dir),
    ]) != 0:
        return 1

    return run_cmd(["cmake", "--build", str(build_dir)])


def find_k210_firmware(build_dir: Path) -> Path | None:
    expected = build_dir / "SYSU_AIOTOS_k210.bin"
    if expected.exists():
        return expected

    bin_files = sorted(
        build_dir.glob("*.bin"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return bin_files[0] if bin_files else None


def flash_k210(board: dict, port: str | None, baudrate: int | None = None) -> int:
    flash_cfg = board.get("kflash", {})
    build_dir = ROOT / "build" / "k210"
    firmware = find_k210_firmware(build_dir)

    if firmware is None:
        print(f"[ERR] 未在 {build_dir} 中找到 K210 固件 (*.bin)")
        print("[ERR] 请先执行: python3 project.py build -p k210")
        return 1

    if not k210_flash_script_ready():
        print(f"[ERR] 未找到 K210 烧录脚本: {K210_FLASH_SCRIPT}")
        return 1

    if not k210_kflash_module_ready():
        print("[ERR] 未检测到 kflash_py 依赖，无法执行真实 K210 烧录")
        print("[ERR] 可用来源:")
        print(f"       1. Python 环境安装 kflash_py")
        print(f"       2. 补全仓库目录 {K210_FLASH_VENDOR_DIR / 'kflash_py'}")
        return 1

    if not port:
        port = detect_serial_port()
    if not port:
        print("[ERR] 未检测到 K210 串口设备，请使用 --port 或 -d 指定")
        return 1

    if baudrate is None:
        baudrate = int(flash_cfg.get("baud", board.get("uart", {}).get("baud", 115200)))

    cmd = [
        sys.executable,
        str(K210_FLASH_SCRIPT),
        str(firmware),
        "-p",
        port,
        "-b",
        str(baudrate),
    ]

    board_name = flash_cfg.get("board", "auto")
    if board_name:
        cmd.extend(["-B", str(board_name)])
    if flash_cfg.get("slow", False):
        cmd.append("-S")
    if flash_cfg.get("sram", False):
        cmd.append("-s")

    env = os.environ.copy()
    if K210_FLASH_VENDOR_DIR.exists():
        current_pythonpath = env.get("PYTHONPATH", "")
        vendor_path = str(K210_FLASH_VENDOR_DIR)
        env["PYTHONPATH"] = vendor_path if not current_pythonpath else f"{vendor_path}:{current_pythonpath}"

    return run_cmd(cmd, cwd=ROOT, env=env)


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
    elf = elf_path or (ROOT / "build" / "sim" / "SYSU_AIOTOS_sim.elf")
    if not elf.exists():
        print(f"[ERR] 未找到 QEMU ELF: {elf}")
        print("[ERR] 请先执行 --qemu-build")
        return 1
    return run_cmd([sys.executable, str(ROOT / "tools" / "qemu_monitor.py"), "--elf", str(elf)])


def main() -> int:
    parser = argparse.ArgumentParser(description="SYSU_AIOTOS 一键构建/烧录/运行入口")
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
            print(f" - {board_summary(b, board)}")
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
        if platform == "k210":
            if build_k210() != 0:
                return 1
        elif build_stm32(platform) != 0:
            return 1

    if args.flash:
        if ensure_board_actionable(board, "flash") != 0:
            return 1
        if platform == "k210":
            rc = flash_k210(board, args.port, args.baud)
        else:
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
