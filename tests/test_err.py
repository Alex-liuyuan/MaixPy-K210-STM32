"""
sysu.err 模块测试
"""
import pytest
import os
os.environ["MAIX_PLATFORM"] = "linux"

from sysu.err import (
    MaixError, HardwareError, NotSupportedError, InvalidArgError,
    ERR_NONE, ERR_TIMEOUT, ERR_BUSY, ERR_INVAL, ERR_NOMEM, ERR_NODEV,
    check_raise, check_bool,
)


def test_error_codes():
    assert ERR_NONE    ==  0
    assert ERR_TIMEOUT == -1
    assert ERR_BUSY    == -2
    assert ERR_INVAL   == -3
    assert ERR_NOMEM   == -4
    assert ERR_NODEV   == -5


def test_check_raise_ok():
    ret = check_raise(ERR_NONE, "should pass")
    assert ret == ERR_NONE


def test_check_raise_fail():
    with pytest.raises(HardwareError) as exc_info:
        check_raise(ERR_TIMEOUT, "timeout test")
    assert "ret=-1" in str(exc_info.value)


def test_check_raise_no_msg():
    with pytest.raises(HardwareError):
        check_raise(ERR_BUSY)


def test_check_bool_ok():
    check_bool(True, "should pass")  # no exception


def test_check_bool_fail():
    with pytest.raises(MaixError) as exc_info:
        check_bool(False, "condition failed")
    assert "condition failed" in str(exc_info.value)


def test_exception_hierarchy():
    assert issubclass(HardwareError, MaixError)
    assert issubclass(NotSupportedError, MaixError)
    assert issubclass(InvalidArgError, MaixError)
