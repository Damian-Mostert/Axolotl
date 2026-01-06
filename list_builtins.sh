#!/bin/bash
# List all registered builtin functions

echo "📋 Axolotl Builtin Functions"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Extract function names from all builtin source files
find src/builtins -name "*.cpp" -type f | while read file; do
    grep -o 'getName() const override { return "[^"]*"' "$file" 2>/dev/null | \
    sed 's/getName() const override { return "\(.*\)"/\1/' | \
    while read func; do
        echo "  • $func"
    done
done | sort -u

echo ""
echo "Total: $(find src/builtins -name "*.cpp" -type f -exec grep -h 'getName() const override' {} \; | wc -l | tr -d ' ') functions"
