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

# Check prerequisites
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake required"; exit 1; }
command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "Error: C++ compiler required"; exit 1; }

echo "[1/5] Building Axolotl compiler..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" >/dev/null 2>&1
cmake --build build -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) >/dev/null 2>&1

echo "[2/5] Installing binaries..."
sudo mkdir -p "$BIN_DIR" "$LIB_DIR" "$SHARE_DIR"
sudo cp build/compiler "$BIN_DIR/axolotl"
sudo chmod +x "$BIN_DIR/axolotl"

echo "[3/5] Installing standard library..."
sudo cp -r modules "$LIB_DIR/" 2>/dev/null || true
sudo cp -r env "$LIB_DIR/" 2>/dev/null || true

echo "[4/5] Installing documentation..."
sudo cp README.md "$SHARE_DIR/"
sudo cp -r tests "$SHARE_DIR/examples" 2>/dev/null || true

echo "[5/5] Installing VS Code extension..."
if command -v code >/dev/null 2>&1; then
    LATEST_VSIX=$(ls -t lang-ext/axo-syntax-*.vsix 2>/dev/null | head -1)
    if [ -n "$LATEST_VSIX" ]; then
        code --install-extension "$LATEST_VSIX" --force >/dev/null 2>&1 && echo "  ✓ VS Code extension installed" || echo "  ⚠ VS Code extension install failed"
    fi
else
    echo "  ⚠ VS Code not found, skipping extension"
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
