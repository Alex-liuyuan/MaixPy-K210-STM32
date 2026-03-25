#!/usr/bin/env python3
"""
SYSU_AIOT_IDE - Web IDE 后端服务器
基于标准库 + websockets，无需 Flask/Django
"""

import asyncio
import base64
import io
import json
import os
import queue
import signal
import subprocess
import sys
import threading
import time
import traceback
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

import websockets

# ── 路径配置 ──────────────────────────────────────────────────────────────
ROOT_DIR     = Path(__file__).parent.parent.parent.resolve()  # 项目根目录
STATIC_DIR   = Path(__file__).parent / "static"
EXAMPLES_DIR = ROOT_DIR / "examples"
HTTP_PORT    = int(os.environ.get("MAIXVISION_HTTP_PORT", 8765))
WS_PORT      = int(os.environ.get("MAIXVISION_WS_PORT",  8766))

# ── 全局状态 ──────────────────────────────────────────────────────────────
_ws_clients: set = set()          # 已连接的 WebSocket 客户端
_run_process: subprocess.Popen | None = None  # 当前运行的子进程
_run_lock = threading.Lock()

# 串口状态
_serial_port   = None   # serial.Serial 实例
_serial_lock   = threading.Lock()
_serial_rx_thread = None


# ══════════════════════════════════════════════════════════════════════════
# HTTP 静态文件服务
# ══════════════════════════════════════════════════════════════════════════

MIME = {
    ".html": "text/html; charset=utf-8",
    ".js":   "application/javascript; charset=utf-8",
    ".css":  "text/css; charset=utf-8",
    ".png":  "image/png",
    ".ico":  "image/x-icon",
    ".json": "application/json",
}


class IDEHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass  # 静默 HTTP 日志

    def do_OPTIONS(self):
        """CORS 预检请求"""
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin",  "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path   = parsed.path

        # API 路由
        if path == "/api/files":
            self._api_files(parsed.query)
            return
        if path == "/api/file":
            self._api_read_file(parsed.query)
            return
        if path == "/api/examples":
            self._api_examples()
            return
        if path == "/api/serial/ports":
            self._api_serial_ports()
            return
        if path == "/api/version":
            self._api_version()
            return

        # 静态文件
        if path == "/" or path == "":
            path = "/index.html"
        file_path = STATIC_DIR / path.lstrip("/")
        if file_path.exists() and file_path.is_file():
            ext  = file_path.suffix.lower()
            mime = MIME.get(ext, "application/octet-stream")
            data = file_path.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", mime)
            self.send_header("Content-Length", len(data))
            self.end_headers()
            self.wfile.write(data)
        else:
            self.send_error(404)

    def do_POST(self):
        parsed = urlparse(self.path)
        path   = parsed.path
        length = int(self.headers.get("Content-Length", 0))
        body   = self.rfile.read(length) if length else b""

        if path == "/api/file/save":
            self._api_save_file(body)
        elif path == "/api/file/delete":
            self._api_delete_file(body)
        elif path == "/api/file/rename":
            self._api_rename_file(body)
        elif path == "/api/run/stop":
            self._api_stop()
        elif path == "/api/run/start":
            self._api_run_start(body)
        elif path == "/api/run/tests":
            self._api_run_tests()
        else:
            self.send_error(404)

    # ── API 实现 ──────────────────────────────────────────────────────────

    def _json(self, data, status=200):
        body = json.dumps(data, ensure_ascii=False).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", len(body))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _api_files(self, query):
        """列出目录文件树"""
        params  = parse_qs(query)
        rel     = params.get("dir", ["."])[0]
        base    = (ROOT_DIR / rel).resolve()
        if not str(base).startswith(str(ROOT_DIR)):
            self._json({"error": "forbidden"}, 403); return
        entries = []
        try:
            for p in sorted(base.iterdir()):
                if p.name.startswith(".") or p.name == "__pycache__":
                    continue
                entries.append({
                    "name": p.name,
                    "path": str(p.relative_to(ROOT_DIR)),
                    "type": "dir" if p.is_dir() else "file",
                    "ext":  p.suffix.lower(),
                })
        except PermissionError:
            pass
        self._json({"entries": entries, "dir": rel})

    def _api_read_file(self, query):
        params = parse_qs(query)
        rel    = params.get("path", [""])[0]
        fpath  = (ROOT_DIR / rel).resolve()
        if not str(fpath).startswith(str(ROOT_DIR)):
            self._json({"error": "forbidden"}, 403); return
        if not fpath.exists():
            self._json({"error": "not found"}, 404); return
        try:
            content = fpath.read_text(encoding="utf-8")
            self._json({"content": content, "path": rel})
        except Exception as e:
            self._json({"error": str(e)}, 500)

    def _api_save_file(self, body):
        try:
            data  = json.loads(body)
            rel   = data["path"]
            fpath = (ROOT_DIR / rel).resolve()
            if not str(fpath).startswith(str(ROOT_DIR)):
                self._json({"error": "forbidden"}, 403); return
            fpath.parent.mkdir(parents=True, exist_ok=True)
            fpath.write_text(data["content"], encoding="utf-8")
            self._json({"ok": True})
        except Exception as e:
            self._json({"error": str(e)}, 500)

    def _api_examples(self):
        examples = []
        for p in sorted(EXAMPLES_DIR.rglob("*.py")):
            examples.append({
                "name": p.stem,
                "path": str(p.relative_to(ROOT_DIR)),
                "category": p.parent.name,
            })
        self._json({"examples": examples})

    def _api_serial_ports(self):
        ports = []
        try:
            import serial.tools.list_ports
            for p in serial.tools.list_ports.comports():
                ports.append({"device": p.device, "description": p.description})
        except Exception:
            pass
        self._json({"ports": ports})

    def _api_delete_file(self, body):
        try:
            data  = json.loads(body)
            rel   = data["path"]
            fpath = (ROOT_DIR / rel).resolve()
            if not str(fpath).startswith(str(ROOT_DIR)):
                self._json({"error": "forbidden"}, 403); return
            if not fpath.exists():
                self._json({"error": "not found"}, 404); return
            if fpath.is_dir():
                import shutil
                shutil.rmtree(fpath)
            else:
                fpath.unlink()
            self._json({"ok": True})
        except Exception as e:
            self._json({"error": str(e)}, 500)

    def _api_rename_file(self, body):
        try:
            data     = json.loads(body)
            old_rel  = data["old_path"]
            new_rel  = data["new_path"]
            old_path = (ROOT_DIR / old_rel).resolve()
            new_path = (ROOT_DIR / new_rel).resolve()
            if not str(old_path).startswith(str(ROOT_DIR)) or \
               not str(new_path).startswith(str(ROOT_DIR)):
                self._json({"error": "forbidden"}, 403); return
            if not old_path.exists():
                self._json({"error": "not found"}, 404); return
            new_path.parent.mkdir(parents=True, exist_ok=True)
            old_path.rename(new_path)
            self._json({"ok": True, "new_path": new_rel})
        except Exception as e:
            self._json({"error": str(e)}, 500)

    def _api_stop(self):
        _stop_run()
        self._json({"ok": True})

    def _api_version(self):
        import platform as _plat
        self._json({
            "ide":     "SYSU_AIOT_IDE",
            "version": "1.0.0",
            "python":  sys.version.split()[0],
            "os":      _plat.system(),
            "arch":    _plat.machine(),
        })

    def _api_run_start(self, body):
        """HTTP 触发运行（WS 广播结果）"""
        try:
            data     = json.loads(body)
            code     = data.get("code", "")
            platform = data.get("platform", "linux")
            loop     = asyncio.get_event_loop()
            asyncio.run_coroutine_threadsafe(
                _broadcast({"type": "run_start"}), loop)
            run_code(code, platform, loop)
            self._json({"ok": True})
        except Exception as e:
            self._json({"error": str(e)}, 500)

    def _api_run_tests(self):
        """HTTP 触发单元测试（WS 广播结果）"""
        try:
            loop = asyncio.get_event_loop()
            asyncio.run_coroutine_threadsafe(
                _broadcast({"type": "run_start"}), loop)
            _run_tests(loop)
            self._json({"ok": True})
        except Exception as e:
            self._json({"error": str(e)}, 500)


# ══════════════════════════════════════════════════════════════════════════
# 代码执行引擎
# ══════════════════════════════════════════════════════════════════════════

def _stop_run():
    global _run_process
    with _run_lock:
        if _run_process and _run_process.poll() is None:
            try:
                _run_process.terminate()
                _run_process.wait(timeout=2)
            except Exception:
                try:
                    _run_process.kill()
                except Exception:
                    pass
            _run_process = None


def _stream_output(proc, loop):
    """在后台线程中读取子进程输出并推送到所有 WebSocket 客户端"""
    def push(msg):
        asyncio.run_coroutine_threadsafe(_broadcast(msg), loop)

    for line in iter(proc.stdout.readline, b""):
        text = line.decode("utf-8", errors="replace")
        push({"type": "stdout", "data": text})

    for line in iter(proc.stderr.readline, b""):
        text = line.decode("utf-8", errors="replace")
        push({"type": "stderr", "data": text})

    proc.wait()
    push({"type": "run_end", "code": proc.returncode})


# ══════════════════════════════════════════════════════════════════════════
# 串口通信
# ══════════════════════════════════════════════════════════════════════════

def _serial_connect(port: str, baudrate: int, loop) -> tuple[bool, str]:
    global _serial_port, _serial_rx_thread
    _serial_disconnect()
    try:
        import serial
        sp = serial.Serial(port, baudrate, timeout=0.1)
        with _serial_lock:
            _serial_port = sp
        # 启动 RX 读取线程
        _serial_rx_thread = threading.Thread(
            target=_serial_rx_loop, args=(sp, loop), daemon=True)
        _serial_rx_thread.start()
        return True, ""
    except Exception as e:
        return False, str(e)


def _serial_disconnect():
    global _serial_port
    with _serial_lock:
        if _serial_port and _serial_port.is_open:
            try:
                _serial_port.close()
            except Exception:
                pass
        _serial_port = None


def _serial_write(data: str):
    with _serial_lock:
        if _serial_port and _serial_port.is_open:
            try:
                _serial_port.write((data + '\r\n').encode('utf-8'))
            except Exception:
                pass


def _serial_rx_loop(sp, loop):
    """后台线程：持续读取串口数据并推送到 WebSocket"""
    def push(msg):
        asyncio.run_coroutine_threadsafe(_broadcast(msg), loop)

    buf = b""
    while True:
        try:
            if not sp.is_open:
                break
            chunk = sp.read(256)
            if chunk:
                buf += chunk
                # 按行分割推送
                while b'\n' in buf:
                    line, buf = buf.split(b'\n', 1)
                    text = line.decode('utf-8', errors='replace').rstrip('\r')
                    push({"type": "serial_rx", "data": text + '\n'})
        except Exception:
            break
    push({"type": "serial_status", "connected": False, "msg": "串口连接已断开"})


def run_code(code: str, platform: str, loop):
    """在子进程中执行用户代码"""
    global _run_process
    _stop_run()

    tmp = ROOT_DIR / ".sysu_aiot_ide_run.py"
    tmp.write_text(code, encoding="utf-8")

    env = os.environ.copy()
    env["MAIX_PLATFORM"]    = platform
    env["PYTHONPATH"]       = str(ROOT_DIR)
    env["PYTHONUNBUFFERED"] = "1"

    with _run_lock:
        _run_process = subprocess.Popen(
            [sys.executable, str(tmp)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.PIPE,   # 支持 stdin 交互
            env=env,
            cwd=str(ROOT_DIR),
        )

    t = threading.Thread(
        target=_stream_output, args=(_run_process, loop), daemon=True)
    t.start()


def _run_tests(loop):
    """运行项目单元测试（pytest），结果通过 WS 推送"""
    def push(msg):
        asyncio.run_coroutine_threadsafe(_broadcast(msg), loop)

    def worker():
        env = os.environ.copy()
        env["MAIX_PLATFORM"]    = "linux"
        env["PYTHONPATH"]       = str(ROOT_DIR)
        env["PYTHONUNBUFFERED"] = "1"
        proc = subprocess.Popen(
            [sys.executable, "-m", "pytest", "tests/", "-v", "--tb=short",
             "--no-header"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
            cwd=str(ROOT_DIR),
        )
        for line in iter(proc.stdout.readline, b""):
            text = line.decode("utf-8", errors="replace")
            push({"type": "stdout", "data": text})
        proc.wait()
        push({"type": "run_end", "code": proc.returncode})

    threading.Thread(target=worker, daemon=True).start()


# ══════════════════════════════════════════════════════════════════════════
# 图像捕获（mock 摄像头 → base64 JPEG）
# ══════════════════════════════════════════════════════════════════════════

# 共享帧缓冲：运行中的代码可通过环境变量 MAIXVISION_FRAME_PIPE 写入帧
_last_frame_b64: str = ""
_frame_lock = threading.Lock()


def _capture_frame(width=320, height=240) -> str:
    """返回最新帧（优先使用运行代码推送的帧，否则生成动态演示帧）"""
    with _frame_lock:
        if _last_frame_b64:
            return _last_frame_b64

    try:
        import numpy as np
        from PIL import Image as PILImage

        t = time.time()
        img = np.zeros((height, width, 3), dtype=np.uint8)
        xs = np.linspace(0, 255, width, dtype=np.uint8)
        ys = np.linspace(0, 255, height, dtype=np.uint8)
        img[:, :, 0] = xs
        img[:, :, 1] = ys.reshape(height, 1)
        img[:, :, 2] = int((np.sin(t * 2) * 0.5 + 0.5) * 255)
        # 叠加时间戳文字（简单像素块）
        pil = PILImage.fromarray(img, "RGB")
        buf = io.BytesIO()
        pil.save(buf, format="JPEG", quality=70)
        return base64.b64encode(buf.getvalue()).decode()
    except Exception:
        return ""


def _push_frame_from_pipe(pipe_path: str, loop):
    """后台线程：从命名管道读取帧数据（base64 JPEG）并广播"""
    global _last_frame_b64
    try:
        import os
        if not os.path.exists(pipe_path):
            os.mkfifo(pipe_path)
        while True:
            with open(pipe_path, 'rb') as f:
                data = f.read()
            if data:
                b64 = base64.b64encode(data).decode()
                with _frame_lock:
                    _last_frame_b64 = b64
                asyncio.run_coroutine_threadsafe(
                    _broadcast({"type": "frame", "data": b64}), loop)
    except Exception:
        pass


# ══════════════════════════════════════════════════════════════════════════
# WebSocket 服务
# ══════════════════════════════════════════════════════════════════════════

async def _broadcast(msg: dict):
    if not _ws_clients:
        return
    data = json.dumps(msg, ensure_ascii=False)
    dead = set()
    for ws in list(_ws_clients):
        try:
            await ws.send(data)
        except Exception:
            dead.add(ws)
    _ws_clients.difference_update(dead)


async def _ws_handler(ws):
    _ws_clients.add(ws)
    loop = asyncio.get_event_loop()
    try:
        async for raw in ws:
            try:
                msg = json.loads(raw)
            except json.JSONDecodeError:
                continue

            action = msg.get("action", "")

            if action == "run":
                code     = msg.get("code", "")
                platform = msg.get("platform", "linux")
                await _broadcast({"type": "run_start"})
                run_code(code, platform, loop)

            elif action == "stop":
                _stop_run()
                await _broadcast({"type": "run_end", "code": -1})

            elif action == "capture":
                frame = _capture_frame(
                    msg.get("width", 320), msg.get("height", 240))
                await ws.send(json.dumps(
                    {"type": "frame", "data": frame}))

            elif action == "ping":
                await ws.send(json.dumps({"type": "pong"}))

            elif action == "serial_connect":
                port     = msg.get("port", "")
                baudrate = msg.get("baudrate", 115200)
                ok, err  = _serial_connect(port, baudrate, loop)
                await ws.send(json.dumps({
                    "type": "serial_status",
                    "connected": ok,
                    "port": port,
                    "msg": "串口已连接" if ok else f"连接失败: {err}",
                }))

            elif action == "serial_disconnect":
                _serial_disconnect()
                await ws.send(json.dumps(
                    {"type": "serial_status", "connected": False,
                     "msg": "串口已断开"}))

            elif action == "serial_write":
                data = msg.get("data", "")
                _serial_write(data)

            elif action == "stdin":
                # 向运行中的子进程写入 stdin
                text = msg.get("data", "")
                with _run_lock:
                    if _run_process and _run_process.poll() is None and \
                       _run_process.stdin:
                        try:
                            _run_process.stdin.write(
                                (text + "\n").encode("utf-8"))
                            _run_process.stdin.flush()
                        except Exception:
                            pass

            elif action == "run_tests":
                # 运行项目单元测试
                await _broadcast({"type": "run_start"})
                _run_tests(loop)

    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        _ws_clients.discard(ws)


# ══════════════════════════════════════════════════════════════════════════
# 启动入口
# ══════════════════════════════════════════════════════════════════════════

def _start_http():
    HTTPServer.allow_reuse_address = True
    server = HTTPServer(("0.0.0.0", HTTP_PORT), IDEHandler)
    server.serve_forever()


async def _main():
    # HTTP 服务在独立线程
    t = threading.Thread(target=_start_http, daemon=True)
    t.start()

    print(f"╔══════════════════════════════════════════╗")
    print(f"║   SYSU_AIOT_IDE                 ║")
    print(f"╠══════════════════════════════════════════╣")
    print(f"║  打开浏览器访问:                          ║")
    print(f"║  http://localhost:{HTTP_PORT}                  ║")
    print(f"║                                          ║")
    print(f"║  按 Ctrl+C 退出                          ║")
    print(f"╚══════════════════════════════════════════╝")

    async with websockets.serve(_ws_handler, "0.0.0.0", WS_PORT):
        await asyncio.Future()  # 永久运行


if __name__ == "__main__":
    try:
        asyncio.run(_main())
    except KeyboardInterrupt:
        _stop_run()
        print("\n已退出 SYSU_AIOT_IDE")
