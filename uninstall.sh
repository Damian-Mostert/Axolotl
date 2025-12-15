#!/bin/bash
set -e

INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="$INSTALL_PREFIX/bin"
LIB_DIR="$INSTALL_PREFIX/lib/axolotl"
SHARE_DIR="$INSTALL_PREFIX/share/axolotl"

echo "Uninstalling Axolotl..."

[ -f "$BIN_DIR/axolotl" ] && sudo rm "$BIN_DIR/axolotl" && echo "  ✓ Removed binary"
[ -d "$LIB_DIR" ] && sudo rm -rf "$LIB_DIR" && echo "  ✓ Removed libraries"
[ -d "$SHARE_DIR" ] && sudo rm -rf "$SHARE_DIR" && echo "  ✓ Removed documentation"

if command -v code >/dev/null 2>&1; then
    code --uninstall-extension axolotl.axo-syntax 2>/dev/null && echo "  ✓ Removed VS Code extension" || true
fi

echo ""
echo "✓ Axolotl uninstalled successfully"
