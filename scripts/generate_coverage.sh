#!/bin/bash
# ============================================================================
# Code Coverage Report Generation
# ============================================================================
# Usage: ./scripts/generate_coverage.sh

set -e

BUILD_DIR="${1:-.}"
LCOV=$(which lcov)
GENHTML=$(which genhtml)

if [ -z "$LCOV" ] || [ -z "$GENHTML" ]; then
    echo "Error: lcov or genhtml not found"
    echo "Install with: brew install lcov (macOS) or apt-get install lcov (Linux)"
    exit 1
fi

cd "$BUILD_DIR"

echo "Generating code coverage report..."

# Zero counters
echo "  Zeroing coverage counters..."
$LCOV --directory . --zerocounters --quiet 2>/dev/null || true

# Run tests
echo "  Running tests..."
ctest --output-on-failure > /dev/null 2>&1 || true

# Capture coverage
echo "  Capturing coverage data..."
$LCOV --directory . --capture --output-file coverage.info --quiet 2>/dev/null || true

# Filter out system headers and test code
echo "  Filtering coverage data..."
$LCOV \
    --remove coverage.info \
    '/usr/*' \
    '*/test/*' \
    '*/benchmark/*' \
    --output-file coverage_filtered.info \
    --quiet 2>/dev/null || true

# Extract summary
echo ""
echo "Coverage Summary:"
$LCOV --summary coverage_filtered.info 2>/dev/null | tee coverage-summary.txt

# Generate HTML report
echo ""
echo "  Generating HTML report..."
$GENHTML \
    coverage_filtered.info \
    --output-directory coverage \
    --title "gdashboard Code Coverage Report" \
    --quiet 2>/dev/null || true

# Generate JSON report for CI/CD
echo "  Generating JSON report..."
{
    echo "{"
    $LCOV --summary coverage_filtered.info 2>/dev/null | while read line; do
        if echo "$line" | grep -q "lines"; then
            percent=$(echo "$line" | grep -o '[0-9.]*%' | head -1 | tr -d '%')
            echo "  \"lines_covered_percent\": $percent,"
        fi
        if echo "$line" | grep -q "functions"; then
            percent=$(echo "$line" | grep -o '[0-9.]*%' | head -1 | tr -d '%')
            echo "  \"functions_covered_percent\": $percent"
        fi
    done
    echo "}"
} > coverage.json

# Print summary
echo ""
echo "Coverage report generated:"
echo "  HTML:    coverage/index.html"
echo "  Summary: coverage-summary.txt"
echo "  JSON:    coverage.json"
echo ""
