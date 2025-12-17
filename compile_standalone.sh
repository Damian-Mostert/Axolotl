#!/bin/bash
# Axolotl Standalone Compiler with Icon Support
# Works on macOS, Linux, and Windows (via WSL/MinGW)

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <source.axo> <output_name> [icon.png]"
    echo "Example: $0 sample/index.axo MyGame sample/icon.png"
    exit 1
fi

SOURCE_FILE="$1"
OUTPUT_NAME="$2"
ICON_FILE="${3:-sample/icon.png}"

echo "🚀 Axolotl Standalone Compiler"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📄 Source: $SOURCE_FILE"
echo "📦 Output: $OUTPUT_NAME"
echo "🎨 Icon: $ICON_FILE"
echo ""

# Detect platform
OS="$(uname -s)"
case "$OS" in
    Darwin*)    PLATFORM="macos";;
    Linux*)     PLATFORM="linux";;
    MINGW*|MSYS*|CYGWIN*) PLATFORM="windows";;
    *)          PLATFORM="unknown";;
esac

echo "🖥️  Platform: $PLATFORM"
echo ""

# Use the Axolotl compiler to generate the standalone executable
./build/compiler compile "$SOURCE_FILE" "$OUTPUT_NAME"

# Platform-specific icon handling
if [ -f "$ICON_FILE" ]; then
    echo ""
    echo "🎨 Embedding icon for $PLATFORM..."
    
    case "$PLATFORM" in
        macos)
            # Create macOS app bundle
            APP_BUNDLE="${OUTPUT_NAME}.app"
            mkdir -p "$APP_BUNDLE/Contents/MacOS"
            mkdir -p "$APP_BUNDLE/Contents/Resources"
            
            # Move executable into bundle
            mv "$OUTPUT_NAME" "$APP_BUNDLE/Contents/MacOS/"
            
            # Convert PNG to ICNS (requires iconutil on macOS)
            ICONSET="${OUTPUT_NAME}.iconset"
            mkdir -p "$ICONSET"
            
            # Generate different icon sizes
            sips -z 16 16     "$ICON_FILE" --out "$ICONSET/icon_16x16.png" 2>/dev/null
            sips -z 32 32     "$ICON_FILE" --out "$ICONSET/icon_16x16@2x.png" 2>/dev/null
            sips -z 32 32     "$ICON_FILE" --out "$ICONSET/icon_32x32.png" 2>/dev/null
            sips -z 64 64     "$ICON_FILE" --out "$ICONSET/icon_32x32@2x.png" 2>/dev/null
            sips -z 128 128   "$ICON_FILE" --out "$ICONSET/icon_128x128.png" 2>/dev/null
            sips -z 256 256   "$ICON_FILE" --out "$ICONSET/icon_128x128@2x.png" 2>/dev/null
            sips -z 256 256   "$ICON_FILE" --out "$ICONSET/icon_256x256.png" 2>/dev/null
            sips -z 512 512   "$ICON_FILE" --out "$ICONSET/icon_256x256@2x.png" 2>/dev/null
            sips -z 512 512   "$ICON_FILE" --out "$ICONSET/icon_512x512.png" 2>/dev/null
            sips -z 1024 1024 "$ICON_FILE" --out "$ICONSET/icon_512x512@2x.png" 2>/dev/null
            
            iconutil -c icns "$ICONSET" -o "$APP_BUNDLE/Contents/Resources/icon.icns"
            rm -rf "$ICONSET"
            
            # Create Info.plist
            cat > "$APP_BUNDLE/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$OUTPUT_NAME</string>
    <key>CFBundleIconFile</key>
    <string>icon.icns</string>
    <key>CFBundleName</key>
    <string>$OUTPUT_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>$OUTPUT_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>com.axolotl.$OUTPUT_NAME</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF
            
            echo "✅ Created macOS app bundle: $APP_BUNDLE"
            echo "   Run with: open $APP_BUNDLE"
            ;;
            
        linux)
            # Create .desktop file for Linux
            DESKTOP_FILE="${OUTPUT_NAME}.desktop"
            ICON_DEST="${OUTPUT_NAME}.png"
            cp "$ICON_FILE" "$ICON_DEST"
            
            cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=$OUTPUT_NAME
Comment=Axolotl Application
Exec=$(pwd)/$OUTPUT_NAME
Icon=$(pwd)/$ICON_DEST
Terminal=false
Categories=Application;
EOF
            
            chmod +x "$OUTPUT_NAME"
            
            echo "✅ Created Linux executable with icon"
            echo "   Icon: $ICON_DEST"
            echo "   Desktop entry: $DESKTOP_FILE"
            echo "   Run with: ./$OUTPUT_NAME"
            ;;
            
        windows)
            # For Windows, we need to create a resource file and recompile
            RC_FILE="${OUTPUT_NAME}.rc"
            ICO_FILE="${OUTPUT_NAME}.ico"
            
            # Convert PNG to ICO (requires ImageMagick)
            if command -v convert &> /dev/null; then
                convert "$ICON_FILE" -define icon:auto-resize=256,128,64,48,32,16 "$ICO_FILE"
                
                # Create resource file
                cat > "$RC_FILE" << EOF
1 ICON "$ICO_FILE"
EOF
                
                # Compile resource
                windres "$RC_FILE" -O coff -o "${OUTPUT_NAME}_res.o"
                
                # Relink with resource
                g++ -o "${OUTPUT_NAME}.exe" "${OUTPUT_NAME}.o" "${OUTPUT_NAME}_res.o" -static
                
                rm -f "$RC_FILE" "${OUTPUT_NAME}_res.o" "${OUTPUT_NAME}.o"
                
                echo "✅ Created Windows executable with icon: ${OUTPUT_NAME}.exe"
            else
                echo "⚠️  ImageMagick not found. Icon not embedded."
                echo "   Install with: apt-get install imagemagick (Linux) or brew install imagemagick (macOS)"
                echo "   Or use rcedit: rcedit ${OUTPUT_NAME}.exe --set-icon $ICON_FILE"
            fi
            ;;
    esac
else
    echo "⚠️  Icon file not found: $ICON_FILE"
    echo "   Executable created without icon"
fi

echo ""
echo "🎉 Build complete!"
