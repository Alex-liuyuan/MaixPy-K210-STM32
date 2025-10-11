from setuptools import setup, find_packages, Distribution
import sys
import os
import platform
import shutil

####################################################################
# 支持的平台
platform_names = {
    "k210": "linux_riscv64",
    "stm32f407": "linux_armv7l", 
    "stm32f767": "linux_armv7l",
    "stm32h743": "linux_armv7l",
    "linux": "manylinux2014_{}".format(platform.machine().replace("-", "_").replace(".", "_").lower()),
}

python_version = {
    "k210": [3, 8],
    "stm32f407": [3, 8],
    "stm32f767": [3, 8], 
    "stm32h743": [3, 8]
}
####################################################################

def get_build_python_version():
    """获取构建Python版本"""
    version = [0, 0, 0]
    mk_file = os.path.join("build", "config", "python_version.txt")
    if os.path.exists(mk_file):
        with open(mk_file, "r", encoding="utf-8") as f:
            version_str = f.read().split(".")
            for i in range(0, 3):
                version[i] = int(version_str[i])
    else:
        # 使用当前Python版本作为默认值
        version = [sys.version_info.major, sys.version_info.minor, sys.version_info.micro]
    return version

def get_python_version():
    """获取当前Python版本"""
    return [sys.version_info.major, sys.version_info.minor, sys.version_info.micro]

# 获取平台参数
platform_target = None
supported_platforms = ["k210", "stm32f407", "stm32f767", "stm32h743", "linux"]

for platform_name in supported_platforms:
    if platform_name in sys.argv:
        platform_target = platform_name
        sys.argv.remove(platform_name)
        break

if "-p" in sys.argv:
    sys.argv.remove("-p")

if (not platform_target) and not ("-h" in sys.argv or "--help" in sys.argv or "--help-commands" in sys.argv):
    print("-- 请指定平台名称: {}, 例如: python setup.py bdist_wheel k210".format(supported_platforms))
    sys.exit(1)

# 清理旧的dist文件夹
if os.path.exists("dist"):
    os.makedirs("dist_old", exist_ok=True)
    shutil.copytree("dist", "dist_old", dirs_exist_ok=True)

# 删除临时文件
if "--not-clean" not in sys.argv and "--skip-build" not in sys.argv and os.path.exists("maix/dl_lib"):
    shutil.rmtree("maix/dl_lib")

def print_py_version_err(build_py_version):
    """打印Python版本错误"""
    print("-- Python版本不匹配构建Python版本!")
    print("   你可以使用conda创建对应Python版本的虚拟环境:")
    print("   从 https://docs.conda.io/en/latest/miniconda.html 下载miniconda")
    print("       conda create -n python{}.{} python={}.{}".format(build_py_version[0], build_py_version[1], build_py_version[0], build_py_version[1]))
    print("       conda activate python{}.{}".format(build_py_version[0], build_py_version[1]))

# 检查Python版本
py_version = get_python_version()
if platform_target in python_version and py_version[:2] != python_version[platform_target]:
    print_py_version_err(python_version[platform_target])
    sys.exit(1)

if platform_target:
    # 构建C++模块
    if "--skip-build" not in sys.argv:
        if "debug" in sys.argv:
            release_str = ""
            sys.argv.remove("debug")
        else:
            release_str = "--release"
        
        config_file = f"configs/{platform_target}_config.mk"
        cmd = f"python project.py build -p {platform_target} {release_str} --config-file {config_file}"
        
        if "--not-clean" not in sys.argv:
            cmd = "python project.py distclean && " + cmd
        else:
            sys.argv.remove("--not-clean")
        
        ret = os.system(cmd)
        if ret != 0:
            print("-- 构建C++模块失败!")
            sys.exit(1)
    else:
        sys.argv.remove("--skip-build")

# 检查Python版本
build_py_version = get_build_python_version()
print("-- 构建Python版本: {}.{}.{}".format(build_py_version[0], build_py_version[1], build_py_version[2]))
print("-- 当前Python版本: {}.{}.{}".format(py_version[0], py_version[1], py_version[2]))

if (py_version[0] != build_py_version[0]) or (py_version[1] != build_py_version[1]):
    print_py_version_err(build_py_version)
    sys.exit(1)

if platform_target:
    # 指定平台包名称
    sys.argv += ["--python-tag", "cp{}{}".format(build_py_version[0], build_py_version[1])]
    sys.argv += ["--plat-name", platform_names[platform_target]]

# 读取版本信息
with open("maix/version.py", "r", encoding="utf-8") as f:
    vars = {}
    exec(f.read(), vars)
    __version__ = vars["__version__"]

# 读取README
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

class BinaryDistribution(Distribution):
    """强制创建二进制包的分发类"""
    def has_ext_modules(foo):
        return True

# 查找包
pkgs = find_packages()
print("-- 找到的包: {}".format(pkgs))

print("\n=================================")
print("python包安装参数:", sys.argv)
print("=================================\n")

setup(
    # 包名
    name='MaixPy-K210-STM32',

    # 版本号
    version=__version__,

    author='MaixPy-K210-STM32 Team',
    author_email='maixpy@sipeed.com',

    description='MaixPy for K210 and STM32 platforms',
    long_description=long_description,
    long_description_content_type="text/markdown",

    # 项目主页
    url='https://github.com/sipeed/maixpy-k210-stm32',

    # 许可证
    license='Apache 2.0',

    # 分类
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Intended Audience :: Education',
        'Intended Audience :: Science/Research',
        'Topic :: Software Development :: Embedded Systems',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
    ],

    # 关键词
    keywords='K210, STM32, MaixPy, AI, Computer Vision, Edge Computing',

    # 包
    packages=pkgs,

    # 运行时依赖
    install_requires=[
        'numpy>=1.19.0',
        'pillow>=8.0.0',
    ],

    # 额外依赖
    extras_require={
        'dev': ['pytest', 'black', 'flake8'],
        'test': ['pytest-cov'],
    },

    # 包数据
    package_data={
        'maix': ['*.so', "dl_lib/*.so*", "*.pyi", "**/*.pyi", "**/**/*.pyi"]
    },

    # 控制台脚本
    entry_points={
        'console_scripts': [
            'maixpy-flash=maix.tools.flash:main',
            'maixpy-monitor=maix.tools.monitor:main',
        ],
    },

    distclass=BinaryDistribution
)

# 后处理
if platform_target:
    py_tag = "cp{}{}".format(build_py_version[0], build_py_version[1])
    files = os.listdir("dist")
    files.sort(key=lambda x: os.path.getmtime(os.path.join("dist", x)), reverse=True)
    
    if files:
        name = files[0]
        print(f"-- 生成的包: {name}")
        
        # 为某些平台创建通用包
        if platform_target in ["k210", "stm32f407"]:
            # 创建py3-none-any包
            import zipfile
            with zipfile.ZipFile(os.path.join("dist", name), "r") as zip_ref:
                zip_ref.extractall("dist/temp")
            
            wheel_path = f"dist/temp/MaixPy_K210_STM32-{__version__}.dist-info/WHEEL"
            if os.path.exists(wheel_path):
                with open(wheel_path, "r", encoding="utf-8") as f:
                    lines = f.readlines()
                
                with open(wheel_path, "w", encoding="utf-8") as f:
                    for line in lines:
                        if line.startswith("Tag:"):
                            f.write("Tag: py3-none-any\n")
                        else:
                            f.write(line)
                
                with zipfile.ZipFile(os.path.join("dist", name), "w") as zip_ref:
                    for root, dirs, files in os.walk("dist/temp"):
                        for file in files:
                            zip_ref.write(os.path.join(root, file), os.path.join(root[9:], file))
                
                shutil.rmtree("dist/temp")
                
                new_name = f"MaixPy_K210_STM32-{__version__}-py3-none-any.whl"
                os.rename(os.path.join("dist", name), os.path.join("dist", new_name))
                print(f"-- 创建通用包: {new_name}")