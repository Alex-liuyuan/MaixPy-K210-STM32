"""
SYSU Studio — 默认入口
"""
from sysu import time, version

print(f"Hello SYSU_AIOTOS v{version()}")
print("平台:", __import__("sysu").platform())
