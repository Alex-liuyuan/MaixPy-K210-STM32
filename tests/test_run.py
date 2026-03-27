"""
tools.run 构建产物发现测试
"""

from tools.run import (
    K210_FLASH_SCRIPT,
    K210_SDK_DIR,
    K210_SDK_LIB,
    K210_RTTHREAD_SDK_LIB,
    K210_TOOLCHAIN_BIN,
    find_k210_firmware,
    find_stm32_firmware,
    k210_flash_script_ready,
    k210_sdk_library_ready,
    k210_sdk_ready,
    k210_toolchain_ready,
    stm32_artifact_basename,
)


def test_stm32_artifact_basename_matches_cmake_naming():
    assert stm32_artifact_basename("rtthread") == "SYSU_AIOTOS_rtthread"
    assert stm32_artifact_basename("stm32f407") == "SYSU_AIOTOS_stm32f407"


def test_find_stm32_firmware_prefers_platform_named_binary(tmp_path):
    build_dir = tmp_path / "rtthread"
    build_dir.mkdir()

    preferred = build_dir / f"{stm32_artifact_basename('rtthread')}.bin"
    fallback = build_dir / "maix_hal_module.bin"
    fallback.write_bytes(b"fallback")
    preferred.write_bytes(b"preferred")

    assert find_stm32_firmware(build_dir, "rtthread") == preferred


def test_find_stm32_firmware_falls_back_to_any_bin(tmp_path):
    build_dir = tmp_path / "stm32f407"
    build_dir.mkdir()

    firmware = build_dir / "custom_output.bin"
    firmware.write_bytes(b"firmware")

    assert find_stm32_firmware(build_dir, "stm32f407") == firmware


def test_find_stm32_firmware_returns_none_when_missing(tmp_path):
    build_dir = tmp_path / "stm32f407"
    build_dir.mkdir()

    assert find_stm32_firmware(build_dir, "stm32f407") is None


def test_k210_sdk_probe_paths_exist_in_repo():
    assert K210_SDK_DIR.name == "kendryte-standalone-sdk"
    assert K210_TOOLCHAIN_BIN.name == "bin"


def test_k210_sdk_probe_assets_are_detected():
    assert k210_sdk_ready() is True
    assert k210_toolchain_ready() is True


def test_k210_sdk_library_artifact_path_is_stable():
    assert K210_SDK_LIB.name == "libkendryte.a"
    assert K210_SDK_LIB.parent.name == "lib"
    assert K210_SDK_LIB.parent.parent.name == "k210_sdk_probe"


def test_k210_rtthread_sdk_library_path_is_stable():
    assert K210_RTTHREAD_SDK_LIB.name == "libkendryte_rtthread.a"


def test_find_k210_firmware_prefers_named_binary(tmp_path):
    build_dir = tmp_path / "k210"
    build_dir.mkdir()

    preferred = build_dir / "SYSU_AIOTOS_k210.bin"
    other = build_dir / "other.bin"
    other.write_bytes(b"other")
    preferred.write_bytes(b"preferred")

    assert find_k210_firmware(build_dir) == preferred


def test_k210_flash_script_is_staged():
    assert K210_FLASH_SCRIPT.name == "flash.py"
    assert k210_flash_script_ready() is True
