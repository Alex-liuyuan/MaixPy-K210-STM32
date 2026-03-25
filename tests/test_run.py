"""
tools.run 构建产物发现测试
"""

from tools.run import find_stm32_firmware, stm32_artifact_basename


def test_stm32_artifact_basename_matches_cmake_naming():
    assert stm32_artifact_basename("rtthread") == "MaixPy_rtthread"
    assert stm32_artifact_basename("stm32f407") == "MaixPy_stm32f407"


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
