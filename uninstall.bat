@echo off
setlocal

set INSTALL_DIR=%ProgramFiles%\Axolotl

echo Uninstalling Axolotl...
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: This script requires administrator privileges
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

if exist "%INSTALL_DIR%" (
    echo Removing installation directory...
    rmdir /S /Q "%INSTALL_DIR%"
    echo   [OK] Removed %INSTALL_DIR%
)

REM Remove from PATH
echo Removing from PATH...
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path') do set CURRENT_PATH=%%b
set NEW_PATH=!CURRENT_PATH:%INSTALL_DIR%\bin;=!
setx /M PATH "!NEW_PATH!" >nul 2>&1
echo   [OK] Removed from PATH

echo.
echo Axolotl uninstalled successfully
echo.
echo Note: Restart your terminal for PATH changes to take effect
pause
