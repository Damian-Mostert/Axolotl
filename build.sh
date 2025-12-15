#!/bin/bash
set -e

LLVM_DIR="/opt/homebrew/opt/llvm/lib/cmake/llvm"

echo "🔨 Building Axolotl..."

if [ ! -d "build" ]; then
    echo "📁 Creating build directory..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR="$LLVM_DIR"
fi

cmake --build build -j$(sysctl -n hw.ncpu)

echo "✅ Build complete!"
echo "Run: ./build/compiler examples/game.axo"
