"""SYSU Studio API 测试。"""

from __future__ import annotations

import asyncio
import json
import os
import sys
from pathlib import Path
from unittest.mock import patch, MagicMock

import pytest

ROOT = Path(__file__).resolve().parents[1]

# 确保项目根在 sys.path
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))


@pytest.fixture(scope="module")
def anyio_backend():
    return "asyncio"


@pytest.fixture
def client():
    """创建 FastAPI TestClient。"""
    from fastapi.testclient import TestClient
    from sysu.studio.server import app, WORKSPACE_DIR

    # 确保 workspace 存在
    WORKSPACE_DIR.mkdir(parents=True, exist_ok=True)
    main_py = WORKSPACE_DIR / "main.py"
    if not main_py.exists():
        main_py.write_text('print("hello")\n', encoding="utf-8")

    with TestClient(app) as c:
        yield c


@pytest.fixture
def workspace_dir():
    from sysu.studio.server import WORKSPACE_DIR
    WORKSPACE_DIR.mkdir(parents=True, exist_ok=True)
    return WORKSPACE_DIR


# ---------------------------------------------------------------------------
# 板卡 API
# ---------------------------------------------------------------------------
class TestBoardsAPI:
    def test_boards_returns_list(self, client):
        resp = client.get("/api/boards")
        assert resp.status_code == 200
        data = resp.json()
        assert isinstance(data, list)
        # 至少有板卡配置
        if len(data) > 0:
            assert "name" in data[0]
            assert "platform" in data[0]

    def test_boards_have_required_fields(self, client):
        resp = client.get("/api/boards")
        for board in resp.json():
            assert "name" in board
            assert "platform" in board
            assert "arch" in board
            assert "status" in board


# ---------------------------------------------------------------------------
# 串口 API
# ---------------------------------------------------------------------------
class TestSerialPortsAPI:
    def test_serial_ports_returns_list(self, client):
        resp = client.get("/api/serial/ports")
        assert resp.status_code == 200
        data = resp.json()
        assert isinstance(data, list)


# ---------------------------------------------------------------------------
# 模拟运行 API
# ---------------------------------------------------------------------------
class TestRunSimAPI:
    def test_run_sim_empty_code_rejected(self, client):
        resp = client.post("/api/run-sim", json={"code": ""})
        assert resp.status_code == 400

    def test_run_sim_whitespace_code_rejected(self, client):
        resp = client.post("/api/run-sim", json={"code": "   "})
        assert resp.status_code == 400

    def test_run_sim_simple_code(self, client):
        resp = client.post("/api/run-sim", json={"code": "print('hello studio')"})
        assert resp.status_code == 200
        data = resp.json()
        assert "task_id" in data
        assert "exit_code" in data
        assert data["exit_code"] == 0

    def test_run_sim_syntax_error(self, client):
        resp = client.post("/api/run-sim", json={"code": "def ("})
        assert resp.status_code == 200
        data = resp.json()
        assert data["exit_code"] != 0


# ---------------------------------------------------------------------------
# 文件 CRUD API
# ---------------------------------------------------------------------------
class TestFileCRUD:
    def test_list_files(self, client):
        resp = client.get("/api/project/files")
        assert resp.status_code == 200
        data = resp.json()
        assert isinstance(data, list)

    def test_create_read_update_delete(self, client, workspace_dir):
        test_path = "test_crud_tmp.py"
        target = workspace_dir / test_path

        try:
            # 创建
            resp = client.post("/api/project/file", json={"path": test_path, "content": "# test"})
            assert resp.status_code == 200

            # 读取
            resp = client.get(f"/api/project/file?path={test_path}")
            assert resp.status_code == 200
            assert resp.json()["content"] == "# test"

            # 更新
            resp = client.put("/api/project/file", json={"path": test_path, "content": "# updated"})
            assert resp.status_code == 200

            resp = client.get(f"/api/project/file?path={test_path}")
            assert resp.json()["content"] == "# updated"

            # 删除
            resp = client.delete(f"/api/project/file?path={test_path}")
            assert resp.status_code == 200

            resp = client.get(f"/api/project/file?path={test_path}")
            assert resp.status_code == 404
        finally:
            target.unlink(missing_ok=True)

    def test_path_traversal_blocked(self, client):
        resp = client.get("/api/project/file?path=../../etc/passwd")
        assert resp.status_code in (403, 404)

    def test_update_nonexistent_file(self, client):
        resp = client.put("/api/project/file", json={"path": "nonexistent_xyz.py", "content": "x"})
        assert resp.status_code == 404

    def test_delete_nonexistent_file(self, client):
        resp = client.delete("/api/project/file?path=nonexistent_xyz.py")
        assert resp.status_code == 404


# ---------------------------------------------------------------------------
# 构建 API（mock 验证参数）
# ---------------------------------------------------------------------------
class TestBuildAPI:
    def test_build_triggers_task(self, client):
        """构建 API 应该接受请求（即使工具链不存在也会返回 task_id）。"""
        resp = client.post("/api/build", json={"platform": "rtthread"})
        # 可能成功或因工具链缺失而 exit_code != 0，但不应 500
        assert resp.status_code in (200, 409)
        if resp.status_code == 200:
            data = resp.json()
            assert "task_id" in data


# ---------------------------------------------------------------------------
# WebSocket 串口（连接/断开）
# ---------------------------------------------------------------------------
class TestWebSocketSerial:
    def test_serial_ws_connect_disconnect(self, client):
        with client.websocket_connect("/ws/serial") as ws:
            # 发送 close（无串口打开时也不应崩溃）
            ws.send_text(json.dumps({"action": "close"}))
            resp = ws.receive_text()
            msg = json.loads(resp)
            assert msg["type"] == "status"
            assert msg["connected"] is False


# ---------------------------------------------------------------------------
# 静态文件
# ---------------------------------------------------------------------------
class TestStaticServed:
    def test_index_html(self, client):
        resp = client.get("/")
        assert resp.status_code == 200
        assert "SYSU Studio" in resp.text

    def test_static_css(self, client):
        resp = client.get("/static/style.css")
        assert resp.status_code == 200

    def test_static_js(self, client):
        resp = client.get("/static/app.js")
        assert resp.status_code == 200


# ---------------------------------------------------------------------------
# 取消任务 API
# ---------------------------------------------------------------------------
class TestCancelAPI:
    def test_cancel_no_task(self, client):
        resp = client.post("/api/cancel")
        assert resp.status_code == 200
        assert resp.json()["cancelled"] is False


# ---------------------------------------------------------------------------
# Bundle API
# ---------------------------------------------------------------------------
class TestBundleAPI:
    def test_bundle_empty_code_rejected(self, client):
        resp = client.post("/api/bundle", json={"code": ""})
        assert resp.status_code == 400

    def test_bundle_with_code(self, client):
        resp = client.post("/api/bundle", json={"code": "print('bundled')", "app_id": "test_bundle"})
        assert resp.status_code == 200
        assert resp.json()["ok"] is True
