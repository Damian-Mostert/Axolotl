#!/bin/bash
# Quick install script for Axolotl
# Usage: curl -fsSL https://raw.githubusercontent.com/YOUR_USERNAME/Axolotl/main/quick-install.sh | bash

set -e

REPO="Damian-Mostert/Axolotl"
VERSION="v1.0.0"
TEMP_DIR=$(mktemp -d)

echo "🦎 Installing Axolotl Programming Language..."

cd "$TEMP_DIR"
curl -fsSL "https://github.com/$REPO/archive/$VERSION.tar.gz" | tar -xz
cd "Axolotl-${VERSION#v}"

bash install.sh

cd /
rm -rf "$TEMP_DIR"

echo "✓ Axolotl installed successfully!"
echo "Run: axolotl --help"
