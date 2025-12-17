# Axolotl Installation Guide

## Quick Install

### macOS / Linux
```bash
chmod +x install.sh
./install.sh
```

### Windows
Right-click `install.bat` → **Run as administrator**

---

## Prerequisites

### macOS
```bash
# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake sdl2 gtk+3
```

### Ubuntu / Debian
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libsdl2-dev libgtk-3-dev
```

### Fedora / RHEL
```bash
sudo dnf install gcc-c++ cmake SDL2-devel gtk3-devel
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake sdl2 gtk3
```

### Windows
1. Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with C++ tools, OR
2. Install [MinGW-w64](https://www.mingw-w64.org/)
3. Install [CMake](https://cmake.org/download/)
4. (Optional) Install SDL2 and GTK3 via [vcpkg](https://vcpkg.io/):
   ```cmd
   vcpkg install sdl2:x64-windows gtk:x64-windows
   ```

---

## Manual Build

If the install script doesn't work, build manually:

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run
./build/compiler examples/test.axo
```

---

## Installation Locations

### Unix-like (macOS/Linux)
- Binary: `/usr/local/bin/axolotl`
- Examples: `/usr/local/share/axolotl/examples/`
- Docs: `/usr/local/share/axolotl/README.md`

### Windows
- Binary: `C:\Program Files\Axolotl\bin\axolotl.exe`
- Examples: `C:\Program Files\Axolotl\examples\`
- Docs: `C:\Program Files\Axolotl\README.md`

---

## VS Code Extension

The installer automatically installs the Axolotl VS Code extension if VS Code is detected.

### Manual Installation
1. Open VS Code
2. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
3. Type "Extensions: Install from VSIX"
4. Select `lang-ext/*.vsix`

### Features
- Syntax highlighting for `.axo` files
- Custom file icons
- Code snippets

---

## Uninstall

### macOS / Linux
```bash
chmod +x uninstall.sh
./uninstall.sh
```

### Windows
Right-click `uninstall.bat` → **Run as administrator**

---

## Troubleshooting

### "cmake not found"
Install CMake from https://cmake.org/download/

### "SDL2 not found" (Linux)
```bash
sudo apt-get install libsdl2-dev  # Ubuntu/Debian
sudo dnf install SDL2-devel       # Fedora
```

### "GTK3 not found" (Linux)
```bash
sudo apt-get install libgtk-3-dev  # Ubuntu/Debian
sudo dnf install gtk3-devel        # Fedora
```

### Build fails on Windows
- Ensure Visual Studio C++ tools are installed
- Try running from "Developer Command Prompt for VS"
- Or use MinGW and ensure `g++` is in PATH

### Permission denied (macOS/Linux)
The installer will prompt for `sudo` when needed. If it fails:
```bash
sudo ./install.sh
```

### PATH not updated (Windows)
Restart your terminal or computer after installation.

---

## Custom Installation Directory

### Unix-like
```bash
INSTALL_PREFIX=$HOME/.local ./install.sh
```

### Verify Installation
```bash
axolotl --version  # Should show version info
axolotl            # Start REPL
```

---

## Getting Started

After installation:

```bash
# Run an example
axolotl /usr/local/share/axolotl/examples/test.axo

# Start REPL
axolotl

# Create your first program
echo 'print("Hello, Axolotl!");' > hello.axo
axolotl hello.axo
```

See [README.md](README.md) for language documentation.
