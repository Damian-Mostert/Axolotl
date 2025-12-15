# Axolotl Installer

## Quick Install

```bash
./install.sh
```

This installs Axolotl system-wide to `/usr/local/bin/axolotl`.

## What Gets Installed

- **Binary**: `/usr/local/bin/axolotl` - The Axolotl compiler/interpreter
- **Standard Library**: `/usr/local/lib/axolotl/modules/` - Standard library modules
- **Environment**: `/usr/local/lib/axolotl/env/` - Environment configuration
- **Documentation**: `/usr/local/share/axolotl/` - README and examples
- **VS Code Extension**: Automatically installed if VS Code is detected

## Usage After Installation

```bash
# Run a file
axolotl myprogram.axo

# Run index.axo in current directory (default behavior)
axolotl

# Access from anywhere
cd ~/projects/myapp
axolotl
```

## Features

### Process Global Object

Axolotl automatically loads `.env` files and provides a `process` global:

```axolotl
// Access environment variables
print(process.env.API_KEY);
print(process.env.PORT);

// Get current working directory
print(process.cwd);

// Access command-line arguments
print(process.args[0]);
```

### Auto-run index.axo

When you run `axolotl` without arguments, it automatically looks for and runs `index.axo` in the current directory.

## Custom Install Location

```bash
INSTALL_PREFIX=$HOME/.local ./install.sh
```

Then add to your shell profile:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

## Uninstall

```bash
sudo ./uninstall.sh
```

## Using Make

```bash
make install    # Build and install
make uninstall  # Remove installation
make clean      # Clean build files
```

## Building Without Installing

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/compiler myfile.axo
```

## Troubleshooting

**"Permission denied"**: Run with `sudo` or use custom install prefix

**"axolotl: command not found"**: Ensure `/usr/local/bin` is in your PATH

**VS Code extension not working**: Manually install:
```bash
code --install-extension lang-ext/axo-syntax-*.vsix
```

## System Requirements

- macOS or Linux
- CMake 3.10+
- C++17 compiler
- LLVM (for JIT)
- libcurl
- SDL2 + SDL2_image

### Install Dependencies

**macOS**:
```bash
brew install cmake llvm curl sdl2 sdl2_image
```

**Ubuntu/Debian**:
```bash
sudo apt install cmake clang llvm-dev libcurl4-openssl-dev libsdl2-dev libsdl2-image-dev
```
