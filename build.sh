#!/bin/bash

# Build and Test the Compiler Engine
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║        COMPILER ENGINE - BUILD & RUN SCRIPT                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

PROJECT_DIR="/Users/damian/game-engine"
cd "$PROJECT_DIR"

# Clean and rebuild
echo "🏗️  Building the Compiler Engine..."
echo ""

mkdir -p build
cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -E "Configuring|Generating|C compiler|CXX compiler" || true
cmake --build . 2>&1 | tail -3

cd "$PROJECT_DIR"
