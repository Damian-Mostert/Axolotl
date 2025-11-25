#!/bin/bash

# Compiler Engine Test Suite
# This script verifies that all components work correctly

set -e

PROJECT_DIR="/Users/damian/game-engine"
BUILD_DIR="$PROJECT_DIR/build"
COMPILER="$BUILD_DIR/compiler"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║           COMPILER ENGINE TEST SUITE                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if compiler exists
if [ ! -f "$COMPILER" ]; then
    echo "❌ ERROR: Compiler not found at $COMPILER"
    echo "Please run: cd $PROJECT_DIR && cmake -S . -B build && cmake --build build"
    exit 1
fi

echo "✅ Compiler found: $COMPILER"
echo "📊 Compiler size: $(ls -lh $COMPILER | awk '{print $5}')"
echo ""

# Test each example
TESTS=(
    "examples/minimal.lang"
    "examples/simple.lang"
    "examples/functions.lang"
    "examples/logic.lang"
    "examples/fibonacci.lang"
    "examples/test.lang"
    "examples/showcase.lang"
)

PASSED=0
FAILED=0

echo "═══════════════════════════════════════════════════════════════════"
echo "Running Test Suite"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

for test in "${TESTS[@]}"; do
    if [ -f "$PROJECT_DIR/$test" ]; then
        echo -n "Testing $test ... "
        
        # Run with timeout (use sleep to avoid infinite loops)
        if (sleep 1; pkill -9 -f "build/compiler" 2>/dev/null) & "$COMPILER" "$PROJECT_DIR/$test" >/dev/null 2>&1; then
            echo "✅ PASSED"
            ((PASSED++))
        else
            echo "❌ FAILED"
            ((FAILED++))
        fi
    fi
done

wait 2>/dev/null || true
pkill -9 -f "build/compiler" 2>/dev/null || true

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo "Test Results: $PASSED passed, $FAILED failed"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# Summary
if [ $FAILED -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    exit 0
else
    echo "⚠️  SOME TESTS FAILED"
    exit 1
fi
