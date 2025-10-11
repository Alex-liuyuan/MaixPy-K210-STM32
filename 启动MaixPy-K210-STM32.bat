@echo off
chcp 65001 >nul
echo ======================================
echo    MaixPy-K210-STM32 启动脚本
echo ======================================
echo.

:: 检查Python环境
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到Python环境，请先安装Python 3.8+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

echo [信息] Python环境检查通过

:: 检查依赖包
echo [信息] 检查Python依赖包...
python -c "import numpy; print('[检查] NumPy:', numpy.__version__)" 2>nul
if errorlevel 1 (
    echo [警告] NumPy未安装，正在安装...
    pip install numpy
)

python -c "import serial; print('[检查] PySerial 已安装')" 2>nul
if errorlevel 1 (
    echo [警告] PySerial未安装，正在安装...
    pip install pyserial
)

echo.
echo ======================================
echo           选择目标平台
echo ======================================
echo 1. K210 平台开发
echo 2. STM32F407 平台开发  
echo 3. STM32F767 平台开发
echo 4. STM32H743 平台开发
echo 5. Linux 仿真模式
echo 6. 查看帮助
echo 7. 退出
echo.
set /p choice="请选择 (1-7): "

if "%choice%"=="1" (
    set PLATFORM=k210
    echo [选择] K210 平台
) else if "%choice%"=="2" (
    set PLATFORM=stm32f407
    echo [选择] STM32F407 平台
) else if "%choice%"=="3" (
    set PLATFORM=stm32f767
    echo [选择] STM32F767 平台
) else if "%choice%"=="4" (
    set PLATFORM=stm32h743
    echo [选择] STM32H743 平台
) else if "%choice%"=="5" (
    set PLATFORM=linux
    echo [选择] Linux 仿真模式
) else if "%choice%"=="6" (
    echo.
    echo ======================================
    echo             帮助信息
    echo ======================================
    echo.
    echo 项目结构:
    echo   examples/          - 示例代码
    echo   components/        - 核心组件
    echo   configs/           - 平台配置
    echo   docs/              - 文档
    echo.
    echo 常用命令:
    echo   python project.py build -p k210
    echo   python project.py flash -p k210 -d COM3
    echo   python examples/basic/hello_maix.py
    echo.
    pause
    goto :eof
) else if "%choice%"=="7" (
    echo 再见！
    exit /b 0
) else (
    echo [错误] 无效选择
    pause
    goto :eof
)

echo.
echo ======================================
echo           选择操作
echo ======================================
echo 1. 编译固件
echo 2. 烧录固件
echo 3. 编译并烧录
echo 4. 运行示例程序
echo 5. 配置平台
echo 6. 清理构建文件
echo 7. 返回主菜单
echo.
set /p action="请选择操作 (1-7): "

if "%action%"=="1" (
    echo [执行] 编译 %PLATFORM% 平台固件...
    python project.py build -p %PLATFORM%
) else if "%action%"=="2" (
    set /p port="请输入串口 (如 COM3 或 /dev/ttyUSB0): "
    echo [执行] 烧录到 %port%...
    python project.py flash -p %PLATFORM% -d %port%
) else if "%action%"=="3" (
    set /p port="请输入串口 (如 COM3 或 /dev/ttyUSB0): "
    echo [执行] 编译并烧录...
    python project.py build -p %PLATFORM%
    if not errorlevel 1 (
        python project.py flash -p %PLATFORM% -d %port%
    )
) else if "%action%"=="4" (
    echo.
    echo 可用示例:
    echo   1. examples/basic/hello_maix.py - 基础摄像头显示
    echo   2. examples/basic/gpio_demo.py - GPIO控制
    echo   3. examples/vision/image_classification.py - AI图像分类
    echo.
    set /p example="请选择示例 (1-3): "
    if "%example%"=="1" (
        python examples/basic/hello_maix.py
    ) else if "%example%"=="2" (
        python examples/basic/gpio_demo.py
    ) else if "%example%"=="3" (
        python examples/vision/image_classification.py
    ) else (
        echo [错误] 无效选择
    )
) else if "%action%"=="5" (
    echo [执行] 配置 %PLATFORM% 平台...
    python project.py menuconfig -p %PLATFORM%
) else if "%action%"=="6" (
    echo [执行] 清理构建文件...
    python project.py clean
) else if "%action%"=="7" (
    echo 返回主菜单...
    goto :eof
) else (
    echo [错误] 无效操作
)

echo.
echo ======================================
echo 操作完成！按任意键继续...
pause >nul