#!/usr/bin/env python3
"""
SYSU_AIOT_IDE 启动脚本
用法:
    python3 tools/sysu_aiot_ide/launch.py [--port 8765] [--ws-port 8766] [--no-browser]
"""

import argparse
import os
import subprocess
import sys
import time
import webbrowser
from pathlib import Path

SERVER = Path(__file__).parent / "server.py"


def main():
    parser = argparse.ArgumentParser(description="SYSU_AIOT_IDE")
    parser.add_argument("--port",       type=int, default=8765, help="HTTP 端口")
    parser.add_argument("--ws-port",    type=int, default=8766, help="WebSocket 端口")
    parser.add_argument("--no-browser", action="store_true",    help="不自动打开浏览器")
    parser.add_argument("--host",       default="localhost",    help="绑定地址")
    args = parser.parse_args()

    env = os.environ.copy()
    env["MAIXVISION_HTTP_PORT"] = str(args.port)
    env["MAIXVISION_WS_PORT"]   = str(args.ws_port)
    env["MAIXVISION_HOST"]      = args.host

    print(f"启动 SYSU_AIOT_IDE...")
    print(f"  HTTP : http://{args.host}:{args.port}")
    print(f"  WS   : ws://{args.host}:{args.ws_port}")

    proc = subprocess.Popen(
        [sys.executable, str(SERVER)],
        env=env,
        cwd=str(Path(__file__).parent.parent.parent),
    )

    if not args.no_browser:
        time.sleep(1.5)
        url = f"http://localhost:{args.port}"
        print(f"  打开浏览器: {url}")
        webbrowser.open(url)

    try:
        proc.wait()
    except KeyboardInterrupt:
        proc.terminate()
        proc.wait()
        print("\nSYSU_AIOT_IDE 已退出")


if __name__ == "__main__":
    main()
