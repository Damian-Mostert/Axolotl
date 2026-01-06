#!/bin/bash
set -e

EXT_DIR="vs-code-extentions"
OUTPUT_DIR="build/extensions"

echo "🔄 Updating Axolotl VS Code Extensions"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

mkdir -p "$OUTPUT_DIR"

if ! command -v vsce &> /dev/null; then
    echo "📦 Installing vsce..."
    npm install -g @vscode/vsce
fi

echo "🔍 Extracting built-in functions..."
./scripts/extract_builtins.sh vs-code-extentions/axolotl-extension/builtins.json src/builtins/*.cpp

EXT_NAME="axolotl-extension"
echo ""
echo "📦 Building $EXT_NAME..."

cd "$EXT_DIR/$EXT_NAME"
[ -f "package-lock.json" ] && npm install --silent
rm -f "../../$OUTPUT_DIR/$EXT_NAME.vsix"
vsce package -o "../../$OUTPUT_DIR/$EXT_NAME.vsix"
cd - > /dev/null

echo "🔧 Installing $EXT_NAME..."
CODE_CMD=""
if command -v code &> /dev/null; then
    CODE_CMD="code"
elif [ -f "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code" ]; then
    CODE_CMD="/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code"
else
    echo "⚠️  'code' command not found. Install manually:"
    echo "   code --install-extension $OUTPUT_DIR/$EXT_NAME.vsix"
    exit 0
fi

"$CODE_CMD" --install-extension "$OUTPUT_DIR/$EXT_NAME.vsix" --force

echo "✅ $EXT_NAME updated"
echo ""
echo "🎉 Extension updated and installed!"
echo "💡 Reload VS Code window to see changes"
