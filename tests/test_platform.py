"""
maix 平台归一化测试
"""

import importlib
import sys


def reload_maix(monkeypatch, *, maix_platform=None, platform_env=None, hal_platform=None):
    if maix_platform is None:
        monkeypatch.delenv("MAIX_PLATFORM", raising=False)
    else:
        monkeypatch.setenv("MAIX_PLATFORM", maix_platform)

    if platform_env is None:
        monkeypatch.delenv("PLATFORM", raising=False)
    else:
        monkeypatch.setenv("PLATFORM", platform_env)

    if hal_platform is not None:
        import _maix_hal as hal
        monkeypatch.setattr(hal, "platform", hal_platform, raising=False)

    sys.modules.pop("maix", None)
    import maix

    return importlib.reload(maix)


def test_detect_platform_from_specific_hal_name(monkeypatch):
    maix = reload_maix(monkeypatch, hal_platform="stm32f407")
    assert maix.platform() == "stm32"
    assert maix.sys.platform() == "stm32"


def test_detect_platform_from_rtthread_env(monkeypatch):
    maix = reload_maix(monkeypatch, maix_platform="rtthread")
    assert maix.platform() == "stm32"


def test_detect_platform_from_specific_platform_env(monkeypatch):
    maix = reload_maix(monkeypatch, platform_env="stm32f407")
    assert maix.platform() == "stm32"


def test_detect_platform_linux_passthrough(monkeypatch):
    maix = reload_maix(monkeypatch, maix_platform="linux")
    assert maix.platform() == "linux"
