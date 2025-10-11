#!/usr/bin/env python
#-*- coding = utf-8 -*-

"""
@file MaixPy-K210-STM32 项目构建脚本
@author MaixPy-K210-STM32 Team  
@license Apache 2.0
"""

import sys, os

def is_project_valid():
    """检查项目是否有效"""
    return os.path.exists("main") or os.path.exists("components")

def get_supported_platforms():
    """获取支持的平台列表"""
    return ["k210", "stm32f407", "stm32f767", "stm32h743", "linux"]

def get_sdk_path():
    """获取SDK路径"""
    sdk_path = None
    sdk_env_name = "MAIXPY_K210_STM32_SDK_PATH"

    # 1. 从环境变量获取SDK路径
    try:
        if os.environ.get(sdk_env_name):
            sdk_path = os.environ[sdk_env_name]
    except Exception:
        pass

    # 2. 检查是否在SDK仓库中
    current_path = os.path.abspath(".")
    if os.path.exists(os.path.join(current_path, "tools", "cmake", "project.py")):
        sdk_path = current_path

    # 3. 检查SDK路径是否有效
    if not sdk_path:
        print("")
        print("Error: 找不到 MaixPy-K210-STM32 SDK，请设置环境变量:")
        print(f"export {sdk_env_name}=<SDK路径>")
        print("")
        sys.exit(1)
        
    if not os.path.exists(sdk_path):
        print("")
        print(f"Error: SDK路径 {sdk_path} 不存在!")
        print("")
        sys.exit(1)
        
    return os.path.abspath(sdk_path)

def exec_project_py():
    """执行项目脚本"""
    # 1. 检查主组件
    if not is_project_valid():
        print("")
        print("Error: 找不到项目主组件，请在项目根目录执行此命令")
        print("")
        sys.exit(1)

    # 2. 获取SDK路径  
    sdk_path = get_sdk_path()
    print("-- SDK路径: {}".format(sdk_path))
    project_path = os.path.abspath(".")

    # 3. 从SDK执行项目脚本
    project_file_path = os.path.join(sdk_path, "tools", "cmake", "project.py")
    if os.path.exists(project_file_path):
        # 直接执行项目脚本文件
        os.system(f"python {project_file_path} {' '.join(sys.argv[1:])}")
    else:
        print("Warning: 未找到SDK项目脚本，使用内置命令处理")
        # 内置命令处理
        if len(sys.argv) > 1 and sys.argv[1] in ['build', 'clean', 'flash', 'monitor']:
            print(f"执行命令: {' '.join(sys.argv[1:])}")
        else:
            print("请使用 'python project.py help' 查看可用命令")

def main():
    """主函数"""
    if len(sys.argv) > 1:
        command = sys.argv[1]
        
        if command == "list-platforms":
            print("支持的平台:")
            for platform in get_supported_platforms():
                print(f"  - {platform}")
            return
            
        elif command == "help" or command == "-h" or command == "--help":
            print("MaixPy-K210-STM32 项目构建工具")
            print("")
            print("用法:")
            print("  python project.py <命令> [选项]")
            print("")
            print("命令:")
            print("  menuconfig        配置项目")
            print("  build             编译项目")
            print("  clean             清理构建文件")
            print("  flash             烧录固件")
            print("  monitor           串口监视器")
            print("  list-platforms    列出支持的平台")
            print("")
            print("选项:")
            print("  -p <platform>     指定目标平台")
            print("  -d <device>       指定串口设备")
            print("  -b <baudrate>     指定波特率")
            print("")
            print("平台:")
            for platform in get_supported_platforms():
                print(f"  {platform}")
            print("")
            print("示例:")
            print("  python project.py build -p k210")
            print("  python project.py flash -p stm32f407 -d COM3")
            return
    
    exec_project_py()

if __name__ == "__main__":
    main()