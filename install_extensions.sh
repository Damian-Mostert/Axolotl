#!/bin/bash
# Install Axolotl VS Code extensions in correct order

set -e

OUTPUT_DIR="build/extensions"

echo "🔧 Installing Axolotl VS Code Extensions"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if ! command -v code &> /dev/null; then
    echo "❌ VS Code CLI not found"
    echo "Please install VS Code and ensure 'code' command is in PATH"
    exit 1
fi

# Install in specific order to ensure dependencies
EXTENSIONS=(
    "axolotl-highlighter"
    "axolotl-intelsense"
    "axolotl-formater"
)

for ext in "${EXTENSIONS[@]}"; do
    VSIX_FILE="$OUTPUT_DIR/$ext.vsix"
    if [ -f "$VSIX_FILE" ]; then
        echo ""
        echo "📦 Installing $ext..."
        code --install-extension "$VSIX_FILE" --force
        echo "✅ $ext installed"
    else
        echo "⚠️  $VSIX_FILE not found, skipping"
    fi
done

echo ""
echo "🎉 Extensions installed!"
echo ""
echo "Please reload VS Code window for changes to take effect"
echo "Command: Developer: Reload Window"
