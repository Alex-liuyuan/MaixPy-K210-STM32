"""
MaixPy Nano RT-Thread 统一错误处理模块
对齐官方 MaixPy v4 err 模块设计
"""


class MaixError(Exception):
    """MaixPy 基础异常"""
    pass


class HardwareError(MaixError):
    """硬件操作失败"""
    pass


class NotSupportedError(MaixError):
    """当前平台不支持该功能"""
    pass


class InvalidArgError(MaixError):
    """参数无效"""
    pass


# 错误码常量
ERR_NONE    =  0
ERR_TIMEOUT = -1
ERR_BUSY    = -2
ERR_INVAL   = -3
ERR_NOMEM   = -4
ERR_NODEV   = -5


def check_raise(ret: int, msg: str = "") -> int:
    """非零时抛出 HardwareError，返回 ret 支持链式调用"""
    if ret != ERR_NONE:
        raise HardwareError(f"{msg} (ret={ret})" if msg else f"Hardware error (ret={ret})")
    return ret


def check_bool(condition: bool, msg: str = "") -> None:
    """条件为 False 时抛出 MaixError"""
    if not condition:
        raise MaixError(msg or "Assertion failed")
