"""MaixPy Nano RT-Thread Python包安装脚本。"""

from setuptools import setup, find_packages
from pathlib import Path

here = Path(__file__).parent

# 从 maix/version.py 读取版本号（避免 import maix 触发副作用）
version = {}
exec((here / "maix" / "version.py").read_text(), version)

setup(
    name="maixpy-nano-rtthread",
    version=version["__version__"],
    description="跨平台边缘AI开发框架，支持K210和STM32",
    long_description=(here / "README.md").read_text(encoding="utf-8")
    if (here / "README.md").exists()
    else "",
    long_description_content_type="text/markdown",
    author="MaixPy Nano RT-Thread Team",
    license="Apache-2.0",
    python_requires=">=3.8",
    packages=find_packages(exclude=["tests", "tests.*", "examples", "examples.*"]),
    install_requires=[
        "numpy>=1.24.0",
        "pyserial>=3.4",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0",
            "websockets>=12.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "maixpy-project=maix.tools:main",
        ],
    },
    classifiers=[
        "Development Status :: 3 - Alpha",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 3",
        "Topic :: Software Development :: Embedded Systems",
    ],
)
