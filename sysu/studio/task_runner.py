"""异步任务执行器：编译、烧录、模拟运行的统一管理。"""

from __future__ import annotations

import asyncio
import tempfile
import uuid
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Callable


class TaskState(str, Enum):
    PENDING = "pending"
    RUNNING = "running"
    DONE = "done"
    CANCELLED = "cancelled"


@dataclass
class TaskInfo:
    task_id: str
    state: TaskState = TaskState.PENDING
    exit_code: int | None = None
    _process: asyncio.subprocess.Process | None = field(default=None, repr=False)


# 全局：同一时间只允许一个构建/烧录任务
_current_task: TaskInfo | None = None
_lock = asyncio.Lock()


def current_task() -> TaskInfo | None:
    return _current_task


async def run_task(
    cmd: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    on_stdout: Callable[[str], None] | None = None,
    on_stderr: Callable[[str], None] | None = None,
    on_done: Callable[[int], None] | None = None,
) -> TaskInfo:
    """启动异步子进程任务，日志通过回调推送。"""
    global _current_task

    async with _lock:
        if _current_task and _current_task.state == TaskState.RUNNING:
            raise RuntimeError("已有任务正在运行，请等待完成或取消")

        task_id = uuid.uuid4().hex[:8]
        info = TaskInfo(task_id=task_id, state=TaskState.RUNNING)
        _current_task = info

    proc = await asyncio.create_subprocess_exec(
        *cmd,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
        cwd=str(cwd) if cwd else None,
        env=env,
    )
    info._process = proc

    async def _read_stream(stream, callback, stream_name):
        while True:
            line = await stream.readline()
            if not line:
                break
            text = line.decode("utf-8", errors="replace").rstrip("\n")
            if callback:
                callback(text)

    await asyncio.gather(
        _read_stream(proc.stdout, on_stdout, "stdout"),
        _read_stream(proc.stderr, on_stderr, "stderr"),
    )
    await proc.wait()

    info.exit_code = proc.returncode
    info.state = TaskState.DONE if info.state != TaskState.CANCELLED else TaskState.CANCELLED

    if on_done:
        on_done(proc.returncode)

    return info


async def cancel_current_task() -> bool:
    """取消当前运行中的任务。"""
    global _current_task
    if _current_task and _current_task.state == TaskState.RUNNING and _current_task._process:
        _current_task.state = TaskState.CANCELLED
        _current_task._process.terminate()
        try:
            await asyncio.wait_for(_current_task._process.wait(), timeout=5)
        except asyncio.TimeoutError:
            _current_task._process.kill()
        return True
    return False


async def run_python_code(
    code: str,
    *,
    project_root: Path,
    on_stdout: Callable[[str], None] | None = None,
    on_stderr: Callable[[str], None] | None = None,
    on_done: Callable[[int], None] | None = None,
) -> TaskInfo:
    """在主机上模拟运行用户 Python 代码。"""
    import os
    import sys

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".py", dir=str(project_root), delete=False
    ) as f:
        f.write(code)
        tmp_path = f.name

    env = os.environ.copy()
    # 确保项目根目录和 mock 目录在 PYTHONPATH 中
    extra_paths = [str(project_root), str(project_root / "tests" / "mock")]
    existing = env.get("PYTHONPATH", "")
    env["PYTHONPATH"] = ":".join(extra_paths + ([existing] if existing else []))

    try:
        return await run_task(
            [sys.executable, tmp_path],
            cwd=project_root,
            env=env,
            on_stdout=on_stdout,
            on_stderr=on_stderr,
            on_done=on_done,
        )
    finally:
        Path(tmp_path).unlink(missing_ok=True)
