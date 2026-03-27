"""
tests/conftest.py
根据平台自动选择真实或模拟HAL，并将 sysu 包加入 sys.path
"""

import sys
import os
import importlib
import pytest

# 将项目根目录加入 sys.path
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

# 将 mock 目录加入 sys.path（用于 _maix_hal 模拟）
MOCK_DIR = os.path.join(os.path.dirname(__file__), "mock")
if MOCK_DIR not in sys.path:
    sys.path.insert(0, MOCK_DIR)


def _install_mock_hal():
    """将 _maix_hal_mock 注册为 _maix_hal"""
    if "_maix_hal" not in sys.modules:
        try:
            mock = importlib.import_module("_maix_hal_mock")
            sys.modules["_maix_hal"] = mock
        except ImportError as e:
            pytest.skip(f"无法加载模拟HAL: {e}")


def pytest_configure(config):
    """pytest启动时自动安装模拟HAL（Linux x86/amd64 或 PLATFORM=rtthread 时）"""
    import platform
    machine = platform.machine().lower()
    is_linux = "x86" in machine or "amd64" in machine or "aarch64" in machine
    is_rtthread = os.environ.get("PLATFORM", "").lower() == "rtthread"

    if is_linux or is_rtthread:
        _install_mock_hal()


@pytest.fixture(autouse=True)
def reset_mock_hal():
    """每个测试前重置模拟HAL状态"""
    import _maix_hal as hal
    # 重置GPIO状态
    if hasattr(hal, "_gpio_state"):
        hal._gpio_state.clear()
    # 重置摄像头
    if hasattr(hal, "_cam_open"):
        hal._cam_open = False
    # 重置显示
    if hasattr(hal, "_disp_open"):
        hal._disp_open = False
        hal._disp_last_frame = None
    yield


@pytest.fixture
def hal():
    """返回 _maix_hal 模块"""
    import _maix_hal
    return _maix_hal
