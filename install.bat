@echo off
setlocal enabledelayedexpansion

set VERSION=1.0.0
set INSTALL_DIR=%ProgramFiles%\Axolotl

echo ========================================
echo    Axolotl Language Installer v%VERSION%
echo ========================================
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: This script requires administrator privileges
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

REM Check prerequisites
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: cmake not found. Install from https://cmake.org/download/
    pause
    exit /b 1
)

where cl >nul 2>&1
if %errorLevel% == 0 (
    set COMPILER=MSVC
) else (
    where g++ >nul 2>&1
    if %errorLevel% == 0 (
        set COMPILER=MinGW
    ) else (
        echo Error: No C++ compiler found. Install Visual Studio or MinGW
        pause
        exit /b 1
    )
)

echo Detected compiler: !COMPILER!
echo.

REM Check for SDL2 and GTK (optional on Windows)
echo Note: SDL2 and GTK3 are required for graphics features
echo Install via vcpkg: vcpkg install sdl2 gtk
echo.

echo [1/5] Building Axolotl compiler...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

cmake --build build --config Release >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo [2/5] Installing binaries...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
if not exist "%INSTALL_DIR%\bin" mkdir "%INSTALL_DIR%\bin"
copy /Y build\Release\compiler.exe "%INSTALL_DIR%\bin\axolotl.exe" >nul 2>&1
if %errorLevel% neq 0 copy /Y build\compiler.exe "%INSTALL_DIR%\bin\axolotl.exe" >nul 2>&1

echo [3/5] Installing examples...
if exist examples (
    if not exist "%INSTALL_DIR%\examples" mkdir "%INSTALL_DIR%\examples"
    xcopy /E /I /Y examples "%INSTALL_DIR%\examples" >nul 2>&1
)

echo [4/5] Installing documentation...
copy /Y README.md "%INSTALL_DIR%\" >nul 2>&1

echo [5/5] Adding to PATH...
setx /M PATH "%PATH%;%INSTALL_DIR%\bin" >nul 2>&1

echo.
echo Installation complete!
echo.
echo Usage:
echo   axolotl ^<file.axo^>    # Run a program
echo   axolotl               # Start REPL
echo.
echo Installed to: %INSTALL_DIR%
echo.
echo Note: Restart your terminal for PATH changes to take effect
echo Uninstall: Run uninstall.bat as administrator
pause
