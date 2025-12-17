#!/bin/bash
set -e

INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BIN_DIR="$INSTALL_PREFIX/bin"
LIB_DIR="$INSTALL_PREFIX/lib/axolotl"
SHARE_DIR="$INSTALL_PREFIX/share/axolotl"

echo "Uninstalling Axolotl..."
echo ""

if [ -f "$BIN_DIR/axolotl" ]; then
    [ -w "$BIN_DIR/axolotl" ] && rm "$BIN_DIR/axolotl" || sudo rm "$BIN_DIR/axolotl"
    echo "  ✓ Removed binary"
fi

if [ -d "$LIB_DIR" ]; then
    [ -w "$LIB_DIR" ] && rm -rf "$LIB_DIR" || sudo rm -rf "$LIB_DIR"
    echo "  ✓ Removed libraries"
fi

if [ -d "$SHARE_DIR" ]; then
    [ -w "$SHARE_DIR" ] && rm -rf "$SHARE_DIR" || sudo rm -rf "$SHARE_DIR"
    echo "  ✓ Removed documentation"
fi

if command -v code >/dev/null 2>&1; then
    EXTENSIONS=$(code --list-extensions 2>/dev/null | grep -i axolotl || true)
    if [ -n "$EXTENSIONS" ]; then
        echo "$EXTENSIONS" | while read ext; do
            code --uninstall-extension "$ext" >/dev/null 2>&1 && echo "  ✓ Removed VS Code extension: $ext"
        done
    fi
fi

echo ""
echo "✓ Axolotl uninstalled successfully"
