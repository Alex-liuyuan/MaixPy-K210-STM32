from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.run import flash_k210, flash_stm32, load_board


def main() -> int:
    parser = argparse.ArgumentParser(description="烧录 SYSU_AIOTOS 固件")
    parser.add_argument("--board", required=True, help="boards/*.json 中的板卡名")
    parser.add_argument("--port", help="串口设备")
    parser.add_argument("--baud", type=int, help="K210 烧录波特率")
    args = parser.parse_args()

    board = load_board(args.board)
    if board.get("platform") == "k210":
        return flash_k210(board, args.port, args.baud)
    return flash_stm32(board)


if __name__ == "__main__":
    raise SystemExit(main())
