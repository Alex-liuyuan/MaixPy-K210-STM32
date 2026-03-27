"""SYSU Studio 启动入口: python3 -m sysu.studio [--port PORT]"""

from __future__ import annotations

import argparse

from .server import run_server


def main():
    parser = argparse.ArgumentParser(description="SYSU Studio — Web IDE")
    parser.add_argument("--host", default="0.0.0.0", help="监听地址")
    parser.add_argument("--port", type=int, default=8210, help="监听端口")
    args = parser.parse_args()
    run_server(host=args.host, port=args.port)


if __name__ == "__main__":
    main()
