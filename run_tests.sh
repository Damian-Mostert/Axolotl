#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
COMPILER="$BUILD_DIR/compiler"
TEST_DIR="$PROJECT_DIR/tests"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║           AXOLOTL TEST SUITE                                   ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

if [ ! -f "$COMPILER" ]; then
    echo "❌ ERROR: Compiler not found at $COMPILER"
    echo "Please run: ./build.sh"
    exit 1
fi

echo "✅ Compiler found"
echo "📁 Test directory: $TEST_DIR"
echo ""

PASSED=0
FAILED=0
SKIPPED=0

# Tests to skip (interactive/long-running)
SKIP_TESTS=();

should_skip() {
    local test="$1"
    for skip in "${SKIP_TESTS[@]}"; do
        if [[ "$test" == "$skip" ]]; then
            return 0
        fi
    done
    return 1
}

echo "═══════════════════════════════════════════════════════════════════"
echo "Running Tests"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

for test in "$TEST_DIR"/*.axo; do
    if [ -f "$test" ]; then
        basename=$(basename "$test")
        
        if should_skip "$basename"; then
            echo "Testing $basename ... ⏭️  SKIPPED (interactive)"
            ((SKIPPED++))
            continue
        fi
        
        printf "Testing %-40s ... " "$basename"
        
        if "$COMPILER" "$test" >/dev/null 2>&1; then
            echo "✅ PASSED"
            ((PASSED++))
        else
            echo "❌ FAILED"
            ((FAILED++))
        fi
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════════"
echo "Results: $PASSED passed, $FAILED failed, $SKIPPED skipped"
echo "═══════════════════════════════════════════════════════════════════"

[ $FAILED -eq 0 ] && echo "🎉 ALL TESTS PASSED!" || echo "⚠️  SOME TESTS FAILED"
exit $FAILED
