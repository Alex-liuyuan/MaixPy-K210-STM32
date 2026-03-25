"""
project.py / maix.tools CLI 烟雾测试
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def run_cmd(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def test_project_list_platforms():
    res = run_cmd("project.py", "list-platforms")
    assert res.returncode == 0
    assert "rtthread" in res.stdout
    assert "sim" in res.stdout
    assert "k210" not in res.stdout


def test_project_help_mentions_monitor():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "monitor" in res.stdout


def test_tools_run_list_boards():
    res = run_cmd("tools/run.py", "--list-boards")
    assert res.returncode == 0
    assert "stm32f407_nucleo [stable]" in res.stdout


def test_project_help_mentions_sim():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "build -p sim" in res.stdout


def test_maix_tool_modules_importable():
    res = run_cmd("-c", "import maix.tools.flash, maix.tools.monitor; print('ok')")
    assert res.returncode == 0
    assert "ok" in res.stdout
