#!/bin/bash
# Auto-compile all VS Code extensions

set -e

EXT_DIR="vs-code-extentions"
OUTPUT_DIR="build/extensions"

echo "🔧 Building Axolotl VS Code Extensions"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if vsce is installed
if ! command -v vsce &> /dev/null; then
    echo "📦 Installing vsce..."
    npm install -g @vscode/vsce
fi

# Build each extension
for ext in "$EXT_DIR"/*; do
    if [ -d "$ext" ]; then
        EXT_NAME=$(basename "$ext")
        echo ""
        echo "📦 Building $EXT_NAME..."
        
        cd "$ext"
        
        # Install dependencies if package-lock.json exists
        if [ -f "package-lock.json" ]; then
            npm install --silent
        fi
        
        # Package extension
        vsce package -o "../../$OUTPUT_DIR/$EXT_NAME.vsix"
        
        cd - > /dev/null
        
        echo "✅ $EXT_NAME.vsix"
    fi
done

echo ""
echo "🎉 All extensions built successfully!"
echo "📁 Output: $OUTPUT_DIR/"
echo ""
echo "Install with: code --install-extension $OUTPUT_DIR/<extension>.vsix"
