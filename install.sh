#!/bin/bash
set -e

VERSION="1.0.0"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="$INSTALL_PREFIX/bin"
LIB_DIR="$INSTALL_PREFIX/lib/axolotl"
SHARE_DIR="$INSTALL_PREFIX/share/axolotl"

echo "╔════════════════════════════════════════╗"
echo "║   Axolotl Language Installer v$VERSION   ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Detect OS
OS="$(uname -s)"
case "$OS" in
    Linux*)     OS_TYPE=Linux;;
    Darwin*)    OS_TYPE=Mac;;
    MINGW*|MSYS*|CYGWIN*) OS_TYPE=Windows;;
    *)          OS_TYPE="Unknown";;
esac
echo "Detected OS: $OS_TYPE"
echo ""

# Check prerequisites
echo "Checking dependencies..."
command -v cmake >/dev/null 2>&1 || { echo "✗ cmake not found. Install: https://cmake.org/download/"; exit 1; }
command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "✗ C++ compiler not found"; exit 1; }

if [ "$OS_TYPE" = "Linux" ]; then
    if ! pkg-config --exists sdl2 2>/dev/null; then
        echo "✗ SDL2 not found. Install: sudo apt-get install libsdl2-dev (Ubuntu/Debian) or sudo dnf install SDL2-devel (Fedora)"
        exit 1
    fi
    if ! pkg-config --exists gtk+-3.0 2>/dev/null; then
        echo "✗ GTK3 not found. Install: sudo apt-get install libgtk-3-dev (Ubuntu/Debian) or sudo dnf install gtk3-devel (Fedora)"
        exit 1
    fi
elif [ "$OS_TYPE" = "Mac" ]; then
    if ! brew list sdl2 >/dev/null 2>&1; then
        echo "✗ SDL2 not found. Install: brew install sdl2"
        exit 1
    fi
    if ! brew list gtk+3 >/dev/null 2>&1; then
        echo "✗ GTK3 not found. Install: brew install gtk+3"
        exit 1
    fi
fi
echo "✓ All dependencies found"
echo ""

echo "[1/5] Building Axolotl compiler..."
NUM_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" >/dev/null 2>&1
cmake --build build -j$NUM_CORES >/dev/null 2>&1

echo "[2/5] Installing binaries..."
if [ -w "$INSTALL_PREFIX" ]; then
    mkdir -p "$BIN_DIR" "$LIB_DIR" "$SHARE_DIR"
    cp build/compiler "$BIN_DIR/axolotl"
    chmod +x "$BIN_DIR/axolotl"
else
    sudo mkdir -p "$BIN_DIR" "$LIB_DIR" "$SHARE_DIR"
    sudo cp build/compiler "$BIN_DIR/axolotl"
    sudo chmod +x "$BIN_DIR/axolotl"
fi

echo "[3/5] Installing examples..."
if [ -d "examples" ]; then
    [ -w "$SHARE_DIR" ] && cp -r examples "$SHARE_DIR/" || sudo cp -r examples "$SHARE_DIR/"
fi

echo "[4/5] Installing documentation..."
[ -w "$SHARE_DIR" ] && cp README.md "$SHARE_DIR/" || sudo cp README.md "$SHARE_DIR/"

echo "[5/5] Installing VS Code extension..."
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
echo "✓ Installation complete!"
echo ""
echo "Usage:"
echo "  axolotl <file.axo>    # Run a program"
echo "  axolotl               # Start REPL"
echo ""
echo "Installed to: $INSTALL_PREFIX"
echo "Uninstall: sudo ./uninstall.sh"
