#!/bin/bash
# Quick update and reinstall VS Code extensions for development

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

for ext in "$EXT_DIR"/*; do
    if [ -d "$ext" ]; then
        EXT_NAME=$(basename "$ext")
        echo ""
        echo "📦 Building $EXT_NAME..."
        
        cd "$ext"
        [ -f "package-lock.json" ] && npm install --silent
        rm -f "../../$OUTPUT_DIR/$EXT_NAME.vsix"
        vsce package -o "../../$OUTPUT_DIR/$EXT_NAME.vsix"
        cd - > /dev/null
        
        echo "🔧 Installing $EXT_NAME..."
        if command -v code &> /dev/null; then
            code --install-extension "$OUTPUT_DIR/$EXT_NAME.vsix" --force
        elif [ -f "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code" ]; then
            "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code" --install-extension "$OUTPUT_DIR/$EXT_NAME.vsix" --force
        else
            echo "⚠️  'code' command not found. Install manually:"
            echo "   code --install-extension $OUTPUT_DIR/$EXT_NAME.vsix"
        fi
        
        echo "✅ $EXT_NAME updated"
    fi
done

echo ""
echo "🎉 All extensions updated and installed!"
echo "💡 Reload VS Code window to see changes"
