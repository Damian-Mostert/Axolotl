#!/bin/bash
set -e

VERSION="1.0.0"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="$INSTALL_PREFIX/bin"
LIB_DIR="$INSTALL_PREFIX/lib/axolotl"
SHARE_DIR="$INSTALL_PREFIX/share/axolotl"
TEMP_INSTALLED=""

# Dependencies are now permanently installed

# Colors
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

progress_bar() {
    local current=$1
    local total=$2
    local width=40
    local percent=$((current * 100 / total))
    local filled=$((current * width / total))
    printf "\r${CYAN}[${RESET}"
    printf "%${filled}s" | tr ' ' '█'
    printf "%$((width - filled))s" | tr ' ' '░'
    printf "${CYAN}]${RESET} ${BOLD}%3d%%${RESET}" $percent
}

clear
echo ""
echo "${CYAN}╔════════════════════════════════════════╗${RESET}"
echo "${CYAN}║${RESET}  ${BOLD}Axolotl Language Installer v$VERSION${RESET}  ${CYAN}║${RESET}"
echo "${CYAN}╚════════════════════════════════════════╝${RESET}"
echo ""

# Detect OS
OS="$(uname -s)"
case "$OS" in
    Linux*)     OS_TYPE=Linux;;
    Darwin*)    OS_TYPE=Mac;;
    MINGW*|MSYS*|CYGWIN*) OS_TYPE=Windows;;
    *)          OS_TYPE="Unknown";;
esac
echo "${BLUE}►${RESET} Detected OS: ${BOLD}$OS_TYPE${RESET}"
echo ""

# Check and install dependencies
echo "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
echo "${BOLD}Checking dependencies...${RESET}"
echo ""

# CMake
if ! command -v cmake >/dev/null 2>&1; then
    echo "${YELLOW}⚠${RESET}  cmake not found, installing temporarily..."
    if [ "$OS_TYPE" = "Mac" ]; then
        brew install cmake
        TEMP_INSTALLED="$TEMP_INSTALLED cmake"
    elif [ "$OS_TYPE" = "Linux" ]; then
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get update && sudo apt-get install -y cmake
            TEMP_INSTALLED="$TEMP_INSTALLED cmake"
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y cmake
            TEMP_INSTALLED="$TEMP_INSTALLED cmake"
        fi
    fi
fi

# LLVM
LLVM_FOUND=0
if command -v llvm-config >/dev/null 2>&1; then
    LLVM_FOUND=1
elif [ "$OS_TYPE" = "Mac" ] && brew list llvm >/dev/null 2>&1; then
    LLVM_FOUND=1
    export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
fi

if [ $LLVM_FOUND -eq 0 ]; then
    echo "${YELLOW}⚠${RESET}  LLVM not found, installing..."
    if [ "$OS_TYPE" = "Mac" ]; then
        brew install llvm
        export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
    elif [ "$OS_TYPE" = "Linux" ]; then
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get install -y llvm-dev
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y llvm-devel
        fi
    fi
fi

# C++ compiler
if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    echo "${YELLOW}⚠${RESET}  C++ compiler not found, installing temporarily..."
    if [ "$OS_TYPE" = "Mac" ]; then
        xcode-select --install 2>/dev/null || true
    elif [ "$OS_TYPE" = "Linux" ]; then
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get install -y g++
            TEMP_INSTALLED="$TEMP_INSTALLED g++"
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y gcc-c++
            TEMP_INSTALLED="$TEMP_INSTALLED gcc-c++"
        fi
    fi
fi

if [ "$OS_TYPE" = "Linux" ]; then
    if ! pkg-config --exists sdl2 2>/dev/null; then
        echo "${YELLOW}⚠${RESET}  SDL2 not found, installing temporarily..."
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get install -y libsdl2-dev
            TEMP_INSTALLED="$TEMP_INSTALLED libsdl2-dev"
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y SDL2-devel
            TEMP_INSTALLED="$TEMP_INSTALLED SDL2-devel"
        fi
    fi
    if ! pkg-config --exists gtk+-3.0 2>/dev/null; then
        echo "${YELLOW}⚠${RESET}  GTK3 not found, installing temporarily..."
        if command -v apt-get >/dev/null 2>&1; then
            sudo apt-get install -y libgtk-3-dev
            TEMP_INSTALLED="$TEMP_INSTALLED libgtk-3-dev"
        elif command -v dnf >/dev/null 2>&1; then
            sudo dnf install -y gtk3-devel
            TEMP_INSTALLED="$TEMP_INSTALLED gtk3-devel"
        fi
    fi
    # Optional: MySQL
    if command -v apt-get >/dev/null 2>&1; then
        if ! dpkg -l | grep -q libmysqlclient-dev 2>/dev/null; then
            echo "${BLUE}ℹ${RESET}  MySQL support available (optional)"
            read -p "  Install MySQL support? [y/N] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                sudo apt-get install -y libmysqlclient-dev
            fi
        fi
    fi
    # Optional: WebSocket
    if command -v apt-get >/dev/null 2>&1; then
        if ! dpkg -l | grep -q libwebsockets-dev 2>/dev/null; then
            echo "${BLUE}ℹ${RESET}  WebSocket support available (optional)"
            read -p "  Install WebSocket support? [y/N] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                sudo apt-get install -y libwebsockets-dev
            fi
        fi
    fi
elif [ "$OS_TYPE" = "Mac" ]; then
    if ! brew list sdl2 >/dev/null 2>&1; then
        echo "${YELLOW}⚠${RESET}  SDL2 not found, installing temporarily..."
        brew install sdl2
        TEMP_INSTALLED="$TEMP_INSTALLED sdl2"
    fi
    if ! brew list gtk+3 >/dev/null 2>&1; then
        echo "${YELLOW}⚠${RESET}  GTK3 not found, installing temporarily..."
        brew install gtk+3
        TEMP_INSTALLED="$TEMP_INSTALLED gtk+3"
    fi
    # Optional: MySQL
    if ! brew list mysql-connector-c >/dev/null 2>&1; then
        echo "${BLUE}ℹ${RESET}  MySQL support available (optional)"
        read -p "  Install MySQL support? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            brew install mysql-connector-c
        fi
    fi
    # Optional: WebSocket
    if ! brew list libwebsockets >/dev/null 2>&1; then
        echo "${BLUE}ℹ${RESET}  WebSocket support available (optional)"
        read -p "  Install WebSocket support? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            brew install libwebsockets
        fi
    fi
fi
echo "${GREEN}✓${RESET} All dependencies ready"
echo ""

echo "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
echo "${BOLD}Building Axolotl...${RESET}"
echo ""
progress_bar 1 5
echo " Configuring build..."
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" >/dev/null 2>&1
progress_bar 2 5
echo " Compiling source..."
cmake --build build -j$NUM_CORES >/dev/null 2>&1

progress_bar 3 5
echo " Installing binaries..."
if [ -w "$INSTALL_PREFIX" ]; then
    mkdir -p "$BIN_DIR" "$LIB_DIR" "$SHARE_DIR"
    cp build/compiler "$BIN_DIR/axolotl"
    chmod +x "$BIN_DIR/axolotl"
    [ -f build/libaxolotl_static.a ] && cp build/libaxolotl_static.a "$LIB_DIR/libaxolotl.a"
else
    sudo mkdir -p "$BIN_DIR" "$LIB_DIR" "$SHARE_DIR"
    sudo cp build/compiler "$BIN_DIR/axolotl"
    sudo chmod +x "$BIN_DIR/axolotl"
    [ -f build/libaxolotl_static.a ] && sudo cp build/libaxolotl_static.a "$LIB_DIR/libaxolotl.a"
fi

progress_bar 4 5
echo " Installing source files..."
[ -w "$SHARE_DIR" ] && cp -r src "$SHARE_DIR/" || sudo cp -r src "$SHARE_DIR/"
[ -w "$SHARE_DIR" ] && cp -r include "$SHARE_DIR/" || sudo cp -r include "$SHARE_DIR/"

# Store LLVM flags for compiler
LLVM_CONFIG="llvm-config"
if [ "$OS_TYPE" = "Mac" ] && [ -f "/opt/homebrew/opt/llvm/bin/llvm-config" ]; then
    LLVM_CONFIG="/opt/homebrew/opt/llvm/bin/llvm-config"
fi

if command -v $LLVM_CONFIG >/dev/null 2>&1 || [ -f "$LLVM_CONFIG" ]; then
    LLVM_FLAGS=$($LLVM_CONFIG --cxxflags --ldflags --libs core native ExecutionEngine MCJIT RuntimeDyld AArch64 AArch64AsmParser AArch64CodeGen AArch64Desc AArch64Info 2>/dev/null || echo "")
    if [ -w "$LIB_DIR" ]; then
        echo "$LLVM_FLAGS" > "$LIB_DIR/llvm_flags.txt"
    else
        echo "$LLVM_FLAGS" | sudo tee "$LIB_DIR/llvm_flags.txt" >/dev/null
    fi
fi

if [ -d "examples" ]; then
    [ -w "$SHARE_DIR" ] && cp -r examples "$SHARE_DIR/" || sudo cp -r examples "$SHARE_DIR/"
fi
if [ -d "sample" ]; then
    [ -w "$SHARE_DIR" ] && cp -r sample "$SHARE_DIR/" || sudo cp -r sample "$SHARE_DIR/"
fi

[ -w "$SHARE_DIR" ] && cp README.md "$SHARE_DIR/" || sudo cp README.md "$SHARE_DIR/"

progress_bar 5 5
echo " Installing VS Code extension..."
if command -v code >/dev/null 2>&1 && [ -d "lang-ext" ]; then
    LATEST_VSIX=$(ls -t lang-ext/*.vsix 2>/dev/null | head -1)
    if [ -n "$LATEST_VSIX" ]; then
        code --install-extension "$LATEST_VSIX" --force >/dev/null 2>&1 && echo "  ✓ VS Code extension installed" || echo "  ⚠ VS Code extension install failed"
    else
        echo "  ⚠ No .vsix file found, skipping extension"
    fi
else
    echo "  ⚠ VS Code not found or lang-ext missing, skipping extension"
fi

echo ""
echo ""
echo "${GREEN}╔════════════════════════════════════════╗${RESET}"
echo "${GREEN}║${RESET}     ${BOLD}✓ Installation Complete!${RESET}        ${GREEN}║${RESET}"
echo "${GREEN}╚════════════════════════════════════════╝${RESET}"
echo ""
echo "${BOLD}Usage:${RESET}"
echo "  ${CYAN}axolotl${RESET} ${YELLOW}<file.axo>${RESET}    ${BLUE}# Run a program${RESET}"
echo "  ${CYAN}axolotl init${RESET}           ${BLUE}# Create new project${RESET}"
echo "  ${CYAN}axolotl examples${RESET}       ${BLUE}# List examples${RESET}"
echo ""
echo "${BOLD}Installed to:${RESET} ${CYAN}$INSTALL_PREFIX${RESET}"
echo "${BOLD}Uninstall:${RESET} ${YELLOW}sudo ./uninstall.sh${RESET}"
echo ""
