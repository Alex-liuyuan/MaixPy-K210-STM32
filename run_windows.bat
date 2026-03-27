@echo off
setlocal
chcp 65001 >nul

python --version >nul 2>&1
if errorlevel 1 (
    echo [ERR] Python 3 未安装或未加入 PATH
    exit /b 1
)

if "%~1"=="" (
    python project.py help
    exit /b %errorlevel%
)

python project.py %*
exit /b %errorlevel%
