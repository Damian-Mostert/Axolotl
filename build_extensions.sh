#!/bin/bash
set -e

OUTPUT_DIR="build/extensions"
mkdir -p "$OUTPUT_DIR"

echo "🔧 Building Axolotl VS Code Extension"

if ! command -v vsce &> /dev/null; then
    echo "📦 Installing vsce..."
    npm install -g @vscode/vsce
fi

echo "🔍 Extracting built-in functions..."
./scripts/extract_builtins.sh vs-code-extentions/axolotl-extension/builtins.json src/builtins/*.cpp

cd vs-code-extentions/axolotl-extension
vsce package -o "../../$OUTPUT_DIR/axolotl-extension.vsix"
cd - > /dev/null

echo "✅ axolotl-extension.vsix"
echo "📁 Output: $OUTPUT_DIR/"
echo "Install: code --install-extension $OUTPUT_DIR/axolotl-extension.vsix"
