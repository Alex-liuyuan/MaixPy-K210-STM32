"""SYSU Studio — FastAPI 后端：REST API + WebSocket 串口桥接 + 构建日志流。"""

from __future__ import annotations

import asyncio
import json
import os
import sys
from pathlib import Path
from typing import Any

from contextlib import asynccontextmanager

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Query, Body, HTTPException
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles

# 项目根目录
ROOT = Path(__file__).resolve().parents[2]

# 确保项目根在 sys.path 中，以便导入 tools.*
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.run import (
    list_boards,
    load_board,
    board_summary,
    detect_serial_port,
)
from tools.runtime_bundle import DEFAULT_BUNDLE_DIR, stage_runtime_bundle

from .serial_bridge import SerialBridge
from .task_runner import run_task, run_python_code, cancel_current_task, current_task

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
WORKSPACE_DIR = ROOT / "workspace"
STATIC_DIR = Path(__file__).resolve().parent / "static"

PLATFORM_TO_BOARD = {
    "rtthread": "stm32f407_nucleo",
    "k210": "k210_generic",
}

# ---------------------------------------------------------------------------
# Lifespan
# ---------------------------------------------------------------------------
@asynccontextmanager
async def lifespan(app):
    _ensure_workspace()
    yield


# ---------------------------------------------------------------------------
# FastAPI 应用
# ---------------------------------------------------------------------------
app = FastAPI(title="SYSU Studio", version="1.0.0", lifespan=lifespan)

# 静态文件（前端）
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")

# 全局串口桥接实例
_serial_bridge = SerialBridge()

# 构建日志 WebSocket 客户端集合
_build_log_clients: set[WebSocket] = set()


# ---------------------------------------------------------------------------
# 辅助
# ---------------------------------------------------------------------------
def _ensure_workspace():
    """确保 workspace 目录存在。"""
    WORKSPACE_DIR.mkdir(parents=True, exist_ok=True)
    main_py = WORKSPACE_DIR / "main.py"
    if not main_py.exists():
        main_py.write_text(
            '"""\nSYSU Studio — 默认入口\n"""\nfrom sysu import time, version\n\nprint(f"Hello SYSU_AIOTOS v{version()}")\nprint("平台:", __import__("sysu").platform())\n',
            encoding="utf-8",
        )


async def _broadcast_log(stream: str, line: str):
    """向所有 build-log WebSocket 客户端广播一行日志。"""
    msg = json.dumps({"type": "log", "stream": stream, "line": line})
    dead = set()
    for ws in _build_log_clients:
        try:
            await ws.send_text(msg)
        except Exception:
            dead.add(ws)
    _build_log_clients.difference_update(dead)


async def _broadcast_done(exit_code: int):
    msg = json.dumps({"type": "done", "exit_code": exit_code})
    dead = set()
    for ws in _build_log_clients:
        try:
            await ws.send_text(msg)
        except Exception:
            dead.add(ws)
    _build_log_clients.difference_update(dead)


def _sync_broadcast_log(stream: str, line: str):
    """从同步回调中安全地广播日志（在事件循环中调度）。"""
    loop = asyncio.get_event_loop()
    if loop.is_running():
        asyncio.ensure_future(_broadcast_log(stream, line))


def _sync_broadcast_done(exit_code: int):
    loop = asyncio.get_event_loop()
    if loop.is_running():
        asyncio.ensure_future(_broadcast_done(exit_code))


# ---------------------------------------------------------------------------
# 首页
# ---------------------------------------------------------------------------
@app.get("/", response_class=HTMLResponse)
async def index():
    index_path = STATIC_DIR / "index.html"
    if not index_path.exists():
        return HTMLResponse("<h1>SYSU Studio</h1><p>前端文件缺失</p>", status_code=500)
    return HTMLResponse(index_path.read_text(encoding="utf-8"))


# ---------------------------------------------------------------------------
# REST API — 板卡
# ---------------------------------------------------------------------------
@app.get("/api/boards")
async def api_boards():
    result = []
    for name in list_boards():
        try:
            board = load_board(name)
            result.append({
                "name": name,
                "platform": board.get("platform", "unknown"),
                "arch": board.get("arch_family", board.get("arch", "unknown")),
                "status": board.get("status", "stable"),
                "summary": board_summary(name, board),
            })
        except Exception:
            pass
    return result


# ---------------------------------------------------------------------------
# REST API — 串口
# ---------------------------------------------------------------------------
@app.get("/api/serial/ports")
async def api_serial_ports():
    from glob import glob as _glob
    ports = sorted(_glob("/dev/ttyUSB*") + _glob("/dev/ttyACM*"))
    return ports


# ---------------------------------------------------------------------------
# REST API — 构建
# ---------------------------------------------------------------------------
@app.post("/api/build")
async def api_build(body: dict = Body(...)):
    platform = body.get("platform", "rtthread")

    if platform == "sim":
        cmd = [sys.executable, str(ROOT / "project.py"), "build", "-p", "sim"]
    elif platform == "k210":
        cmd = [sys.executable, str(ROOT / "project.py"), "build", "-p", "k210"]
    else:
        cmd = [sys.executable, str(ROOT / "project.py"), "build", "-p", platform]

    try:
        info = await run_task(
            cmd,
            cwd=ROOT,
            on_stdout=lambda line: _sync_broadcast_log("stdout", line),
            on_stderr=lambda line: _sync_broadcast_log("stderr", line),
            on_done=lambda code: _sync_broadcast_done(code),
        )
        return {"task_id": info.task_id, "exit_code": info.exit_code}
    except RuntimeError as e:
        raise HTTPException(status_code=409, detail=str(e))


# ---------------------------------------------------------------------------
# REST API — 烧录
# ---------------------------------------------------------------------------
@app.post("/api/flash")
async def api_flash(body: dict = Body(...)):
    platform = body.get("platform", "rtthread")
    port = body.get("port")

    cmd = [sys.executable, str(ROOT / "project.py"), "flash", "-p", platform]
    if port:
        cmd.extend(["-d", port])

    try:
        info = await run_task(
            cmd,
            cwd=ROOT,
            on_stdout=lambda line: _sync_broadcast_log("stdout", line),
            on_stderr=lambda line: _sync_broadcast_log("stderr", line),
            on_done=lambda code: _sync_broadcast_done(code),
        )
        return {"task_id": info.task_id, "exit_code": info.exit_code}
    except RuntimeError as e:
        raise HTTPException(status_code=409, detail=str(e))


# ---------------------------------------------------------------------------
# REST API — 模拟运行
# ---------------------------------------------------------------------------
@app.post("/api/run-sim")
async def api_run_sim(body: dict = Body(...)):
    code = body.get("code", "")
    if not code.strip():
        raise HTTPException(status_code=400, detail="代码不能为空")

    try:
        info = await run_python_code(
            code,
            project_root=ROOT,
            on_stdout=lambda line: _sync_broadcast_log("stdout", line),
            on_stderr=lambda line: _sync_broadcast_log("stderr", line),
            on_done=lambda code: _sync_broadcast_done(code),
        )
        return {"task_id": info.task_id, "exit_code": info.exit_code}
    except RuntimeError as e:
        raise HTTPException(status_code=409, detail=str(e))


# ---------------------------------------------------------------------------
# REST API — 取消任务
# ---------------------------------------------------------------------------
@app.post("/api/cancel")
async def api_cancel():
    ok = await cancel_current_task()
    return {"cancelled": ok}


# ---------------------------------------------------------------------------
# REST API — Bundle
# ---------------------------------------------------------------------------
@app.post("/api/bundle")
async def api_bundle(body: dict = Body(...)):
    code = body.get("code", "")
    app_id = body.get("app_id", "studio_app")

    if not code.strip():
        raise HTTPException(status_code=400, detail="代码不能为空")

    # 写入临时应用目录
    app_dir = ROOT / "workspace" / ".bundle_tmp" / app_id
    app_dir.mkdir(parents=True, exist_ok=True)
    (app_dir / "main.py").write_text(code, encoding="utf-8")

    try:
        stage_runtime_bundle(app_dir=app_dir, app_id=app_id, reset_app=True)
        return {"ok": True}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


# ---------------------------------------------------------------------------
# REST API — 项目文件 CRUD
# ---------------------------------------------------------------------------
@app.get("/api/project/files")
async def api_project_files():
    _ensure_workspace()
    result = []
    for path in sorted(WORKSPACE_DIR.rglob("*")):
        if path.name.startswith("."):
            continue
        rel = path.relative_to(WORKSPACE_DIR)
        result.append({
            "path": str(rel),
            "name": path.name,
            "is_dir": path.is_dir(),
        })
    return result


@app.get("/api/project/file")
async def api_project_file_read(path: str = Query(...)):
    target = (WORKSPACE_DIR / path).resolve()
    if not str(target).startswith(str(WORKSPACE_DIR.resolve())):
        raise HTTPException(status_code=403, detail="路径越界")
    if not target.exists() or target.is_dir():
        raise HTTPException(status_code=404, detail="文件不存在")
    return {"content": target.read_text(encoding="utf-8")}


@app.put("/api/project/file")
async def api_project_file_update(body: dict = Body(...)):
    path = body.get("path", "")
    content = body.get("content", "")
    target = (WORKSPACE_DIR / path).resolve()
    if not str(target).startswith(str(WORKSPACE_DIR.resolve())):
        raise HTTPException(status_code=403, detail="路径越界")
    if not target.exists():
        raise HTTPException(status_code=404, detail="文件不存在")
    target.write_text(content, encoding="utf-8")
    return {"ok": True}


@app.post("/api/project/file")
async def api_project_file_create(body: dict = Body(...)):
    path = body.get("path", "")
    content = body.get("content", "")
    target = (WORKSPACE_DIR / path).resolve()
    if not str(target).startswith(str(WORKSPACE_DIR.resolve())):
        raise HTTPException(status_code=403, detail="路径越界")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")
    return {"ok": True}


@app.delete("/api/project/file")
async def api_project_file_delete(path: str = Query(...)):
    target = (WORKSPACE_DIR / path).resolve()
    if not str(target).startswith(str(WORKSPACE_DIR.resolve())):
        raise HTTPException(status_code=403, detail="路径越界")
    if not target.exists():
        raise HTTPException(status_code=404, detail="文件不存在")
    target.unlink()
    return {"ok": True}


# ---------------------------------------------------------------------------
# WebSocket — 串口桥接
# ---------------------------------------------------------------------------
@app.websocket("/ws/serial")
async def ws_serial(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            raw = await ws.receive_text()
            msg = json.loads(raw)
            action = msg.get("action")

            if action == "open":
                port = msg.get("port", "")
                baud = int(msg.get("baud", 115200))
                try:
                    _serial_bridge.open(port, baud, asyncio.get_event_loop())
                    await ws.send_text(json.dumps({"type": "status", "connected": True}))
                    # 启动数据转发
                    asyncio.ensure_future(_serial_forward(ws))
                except Exception as e:
                    await ws.send_text(json.dumps({"type": "status", "connected": False, "error": str(e)}))

            elif action == "send":
                import base64
                data = base64.b64decode(msg.get("data", ""))
                _serial_bridge.write(data)

            elif action == "close":
                _serial_bridge.close()
                await ws.send_text(json.dumps({"type": "status", "connected": False}))

    except WebSocketDisconnect:
        _serial_bridge.close()


async def _serial_forward(ws: WebSocket):
    """持续将串口数据转发到 WebSocket。"""
    import base64
    while _serial_bridge.connected:
        data = await _serial_bridge.read()
        if data:
            try:
                await ws.send_text(json.dumps({
                    "type": "data",
                    "data": base64.b64encode(data).decode("ascii"),
                }))
            except Exception:
                break


# ---------------------------------------------------------------------------
# WebSocket — 构建日志流
# ---------------------------------------------------------------------------
@app.websocket("/ws/build-log")
async def ws_build_log(ws: WebSocket):
    await ws.accept()
    _build_log_clients.add(ws)
    try:
        while True:
            # 保持连接，客户端不需要发送数据
            await ws.receive_text()
    except WebSocketDisconnect:
        pass
    finally:
        _build_log_clients.discard(ws)


# ---------------------------------------------------------------------------
# 启动函数
# ---------------------------------------------------------------------------
def run_server(host: str = "0.0.0.0", port: int = 8210):
    """启动 SYSU Studio 服务。"""
    import uvicorn
    print(f"[STUDIO] 启动 SYSU Studio: http://{host}:{port}")
    uvicorn.run(app, host=host, port=port, log_level="info")
