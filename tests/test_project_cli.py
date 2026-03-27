"""
project.py / sysu.tools CLI 烟雾测试
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
    assert "  - rtthread" in res.stdout
    assert "  - sim" in res.stdout
    assert "  - k210" in res.stdout


def test_project_help_mentions_monitor():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "monitor" in res.stdout


def test_tools_run_list_boards():
    res = run_cmd("tools/run.py", "--list-boards")
    assert res.returncode == 0
    assert "stm32f407_nucleo [stable]" in res.stdout
    assert "arch=arm-cortex-m" in res.stdout
    assert "k210_generic [experimental]" in res.stdout


def test_project_help_mentions_architecture_scope():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "ARM Cortex-M 与 RISC-V" in res.stdout


def test_project_help_mentions_sim():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "build -p sim" in res.stdout


def test_project_help_mentions_k210_probe():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "k210-probe" in res.stdout


def test_project_help_mentions_k210_build():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "build -p k210" in res.stdout


def test_project_help_mentions_k210_flash():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "flash -p k210" in res.stdout


def test_project_help_mentions_bundle():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "bundle --app-dir" in res.stdout
    assert "--labels" in res.stdout


def test_project_help_mentions_bundle_info():
    res = run_cmd("project.py", "help")
    assert res.returncode == 0
    assert "bundle-info" in res.stdout


def test_project_bundle_requires_app_dir():
    res = run_cmd("project.py", "bundle")
    assert res.returncode == 1
    assert "bundle 命令需要 --app-dir" in res.stdout


def test_project_bundle_info_lists_runtime_bundle():
    res = run_cmd("project.py", "bundle-info")
    assert res.returncode == 0
    assert "[BUNDLE] files=" in res.stdout
    assert "/boot.py" in res.stdout


def test_sysu_tool_modules_importable():
    res = run_cmd("-c", "import sysu.tools.flash, sysu.tools.monitor; print('ok')")
    assert res.returncode == 0
    assert "ok" in res.stdout
