from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.run import flash_stm32, load_board


def main() -> int:
    parser = argparse.ArgumentParser(description="烧录 MaixPy Nano RT-Thread 固件")
    parser.add_argument("--board", required=True, help="boards/*.json 中的板卡名")
    args = parser.parse_args()

    board = load_board(args.board)
    return flash_stm32(board)


if __name__ == "__main__":
    raise SystemExit(main())
