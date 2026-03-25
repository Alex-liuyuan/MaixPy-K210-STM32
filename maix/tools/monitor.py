from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.run import load_board, monitor_serial


def main() -> int:
    parser = argparse.ArgumentParser(description="打开 MaixPy Nano RT-Thread 串口监视器")
    parser.add_argument("--board", help="boards/*.json 中的板卡名")
    parser.add_argument("--port", help="串口设备")
    parser.add_argument("--baud", type=int, default=115200, help="串口波特率")
    args = parser.parse_args()

    baud = args.baud
    if args.board:
        board = load_board(args.board)
        baud = board.get("uart", {}).get("baud", baud)
    return monitor_serial(args.port, baud)


if __name__ == "__main__":
    raise SystemExit(main())
