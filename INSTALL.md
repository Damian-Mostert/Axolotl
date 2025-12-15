# Axolotl Installation Guide

## Quick Install (macOS/Linux)

```bash
chmod +x install.sh
./install.sh
```

Or using Make:

```bash
make install
```

This will:
- Build the Axolotl compiler
- Install `axolotl` command to `/usr/local/bin`
- Install standard library to `/usr/local/lib/axolotl`
- Install examples to `/usr/local/share/axolotl`
- Install VS Code extension (if VS Code is installed)

## Custom Install Location

```bash
INSTALL_PREFIX=$HOME/.local ./install.sh
```

Then add to your `~/.bashrc` or `~/.zshrc`:

```bash
export PATH="$HOME/.local/bin:$PATH"
export AXOLOTL_LIB="$HOME/.local/lib/axolotl"
```

## Manual Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

## Uninstall

```bash
sudo ./uninstall.sh
```

Or:

```bash
make uninstall
```

## Verify Installation

```bash
axolotl --version
axolotl tests/minimal.axo
```

## VS Code Extension

The installer automatically installs the VS Code extension. To manually install:

```bash
code --install-extension lang-ext/axo-syntax-*.vsix
```

Enable the icon theme:
1. Open Command Palette (Cmd+Shift+P)
2. Type "File Icon Theme"
3. Select "Axolotl Icons"

## Requirements

- CMake 3.10+
- C++17 compiler (GCC/Clang/AppleClang)
- LLVM (for JIT support)
- libcurl (for HTTP functions)
- SDL2 + SDL2_image (for canvas functions)

### macOS

```bash
brew install cmake llvm curl sdl2 sdl2_image
```

### Ubuntu/Debian

```bash
sudo apt install cmake clang llvm-dev libcurl4-openssl-dev libsdl2-dev libsdl2-image-dev
```

## Troubleshooting

**Permission denied**: Run with `sudo` or change `INSTALL_PREFIX`

**Command not found**: Ensure `/usr/local/bin` is in your PATH

**VS Code extension not working**: Manually install from `lang-ext/` folder
