@echo off
setlocal enabledelayedexpansion

set VERSION=1.0.0
set INSTALL_DIR=%ProgramFiles%\Axolotl
set TEMP_INSTALLED=
set CHOCO_INSTALLED=0

cls
echo.
echo [96m========================================[0m
echo [96m[0m  [1mAxolotl Language Installer v%VERSION%[0m  [96m[0m
echo [96m========================================[0m
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [91mError:[0m This script requires administrator privileges
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo [93m========================================[0m
echo [1mChecking dependencies...[0m
echo.

REM Check for chocolatey
where choco >nul 2>&1
if %errorLevel% neq 0 (
    echo [93m^![0m  Installing Chocolatey package manager...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    set CHOCO_INSTALLED=1
    refreshenv
)

REM Check and install CMake
where cmake >nul 2>&1
if %errorLevel% neq 0 (
    echo [93m^![0m  Installing cmake temporarily...
    choco install cmake -y --no-progress
    set TEMP_INSTALLED=!TEMP_INSTALLED! cmake
    refreshenv
)

REM Check and install LLVM
where llvm-config >nul 2>&1
if %errorLevel% neq 0 (
    echo [93m^![0m  Installing LLVM...
    choco install llvm -y --no-progress
    refreshenv
)

REM Check and install compiler
where cl >nul 2>&1
if %errorLevel% == 0 (
    set COMPILER=MSVC
) else (
    where g++ >nul 2>&1
    if %errorLevel% == 0 (
        set COMPILER=MinGW
    ) else (
        echo [93m^![0m  Installing MinGW compiler temporarily...
        choco install mingw -y --no-progress
        set TEMP_INSTALLED=!TEMP_INSTALLED! mingw
        set COMPILER=MinGW
        refreshenv
    )
)

echo [92m^>[0m Detected compiler: [1m!COMPILER![0m
echo [92m✓[0m All dependencies ready
echo.

echo [93m========================================[0m
echo [1mBuilding Axolotl...[0m
echo.
echo [96m[[0m████████[90m████████████████████████████████[96m][0m [1m 20%%[0m Configuring...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >nul 2>&1
if %errorLevel% neq 0 (
    echo [91mError:[0m CMake configuration failed
    pause
    exit /b 1
)

echo [96m[[0m████████████████[90m████████████████████████[96m][0m [1m 40%%[0m Compiling...
cmake --build build --config Release >nul 2>&1
if %errorLevel% neq 0 (
    echo [91mError:[0m Build failed
    pause
    exit /b 1
)

echo [96m[[0m████████████████████████[90m████████████████[96m][0m [1m 60%%[0m Installing binaries...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
if not exist "%INSTALL_DIR%\bin" mkdir "%INSTALL_DIR%\bin"
copy /Y build\Release\compiler.exe "%INSTALL_DIR%\bin\axolotl.exe" >nul 2>&1
if %errorLevel% neq 0 copy /Y build\compiler.exe "%INSTALL_DIR%\bin\axolotl.exe" >nul 2>&1

echo [96m[[0m████████████████████████████████[90m████████[96m][0m [1m 80%%[0m Installing source files...
if not exist "%INSTALL_DIR%\src" mkdir "%INSTALL_DIR%\src"
if not exist "%INSTALL_DIR%\include" mkdir "%INSTALL_DIR%\include"
xcopy /E /I /Y src "%INSTALL_DIR%\src" >nul 2>&1
xcopy /E /I /Y include "%INSTALL_DIR%\include" >nul 2>&1

if exist examples (
    if not exist "%INSTALL_DIR%\examples" mkdir "%INSTALL_DIR%\examples"
    xcopy /E /I /Y examples "%INSTALL_DIR%\examples" >nul 2>&1
)
if exist sample (
    if not exist "%INSTALL_DIR%\sample" mkdir "%INSTALL_DIR%\sample"
    xcopy /E /I /Y sample "%INSTALL_DIR%\sample" >nul 2>&1
)

copy /Y README.md "%INSTALL_DIR%\" >nul 2>&1

echo [96m[[0m████████████████████████████████████████[96m][0m [1m100%%[0m Finalizing...
setx /M PATH "%PATH%;%INSTALL_DIR%\bin" >nul 2>&1

REM Dependencies are now permanently installed

if !CHOCO_INSTALLED!==1 (
    echo [93mRemoving Chocolatey...[0m
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item -Recurse -Force $env:ChocolateyInstall" >nul 2>&1
)

echo.
echo.
echo [92m========================================[0m
echo [92m[0m     [1m✓ Installation Complete![0m        [92m[0m
echo [92m========================================[0m
echo.
echo [1mUsage:[0m
echo   [96maxolotl[0m [93m^<file.axo^>[0m    [90m# Run a program[0m
echo   [96maxolotl init[0m           [90m# Create new project[0m
echo   [96maxolotl examples[0m       [90m# List examples[0m
echo.
echo [1mInstalled to:[0m [96m%INSTALL_DIR%[0m
echo [1mUninstall:[0m [93mRun uninstall.bat as administrator[0m
echo.
echo [93mNote:[0m Restart your terminal for PATH changes to take effect
echo.
pause
