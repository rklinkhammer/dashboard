#!/bin/bash
# ============================================================================
# Profiling Script - perf/gprof Analysis
# ============================================================================
# Profiles test execution using perf (Linux) or Instruments (macOS)
# and gprof for call graph analysis.
#
# Usage:
#   ./scripts/profile_tests.sh [mode] [output_dir]
#
# Modes:
#   perf        - Linux perf profiling with flame graph (default on Linux)
#   gprof       - gprof call graph profiling
#   instruments - macOS Instruments profiling (default on macOS)
#   all         - Run all available profilers
#
# Examples:
#   ./scripts/profile_tests.sh perf ./profile_results
#   ./scripts/profile_tests.sh all ./profile_results
#

set -e

MODE="${1:-auto}"
OUTPUT_DIR="${2:-./profile_results}"
BUILD_DIR="."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    DEFAULT_MODE="perf"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    DEFAULT_MODE="instruments"
else
    OS="unknown"
    DEFAULT_MODE="gprof"
fi

# Use auto-detected mode if "auto" specified
if [ "$MODE" = "auto" ]; then
    MODE="$DEFAULT_MODE"
fi

# Create output directory
mkdir -p "${OUTPUT_DIR}"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "============================================================================"
echo "Profiling Performance Tests"
echo "============================================================================"
echo "OS: $OS"
echo "Mode: $MODE"
echo "Output Directory: ${OUTPUT_DIR}"
echo ""

# ============================================================================
# Profile with perf (Linux) and FlameGraph
# ============================================================================
profile_with_perf() {
    if ! command -v perf &> /dev/null; then
        echo "ERROR: perf not found. Install with: apt-get install linux-tools"
        return 1
    fi

    echo "Running tests with perf profiling..."

    # Run unit tests with perf record
    PERF_OUT="${OUTPUT_DIR}/perf_${TIMESTAMP}.data"
    PERF_REPORT="${OUTPUT_DIR}/perf_report_${TIMESTAMP}.txt"
    PERF_STAT="${OUTPUT_DIR}/perf_stat_${TIMESTAMP}.txt"

    # Record performance data with call graph
    perf record -F 99 -g \
        --output="${PERF_OUT}" \
        "${BUILD_DIR}/bin/test_phase0_unit" 2>&1 | tee "${OUTPUT_DIR}/perf_record_${TIMESTAMP}.log"

    # Generate report
    perf report --input="${PERF_OUT}" > "${PERF_REPORT}" 2>&1

    # Generate statistics
    perf stat \
        --output="${PERF_STAT}" \
        "${BUILD_DIR}/bin/test_phase0_unit" 2>&1 | head -50

    echo "  Results:"
    echo "    Data:   ${PERF_OUT}"
    echo "    Report: ${PERF_REPORT}"
    echo "    Stats:  ${PERF_STAT}"
    echo ""

    # Try to generate FlameGraph if available
    if command -v perf2svg &> /dev/null || command -v stackcollapse-perf.pl &> /dev/null; then
        echo "  Generating FlameGraph..."
        perf script --input="${PERF_OUT}" > "${OUTPUT_DIR}/perf_script_${TIMESTAMP}.txt"
        echo "    Script: ${OUTPUT_DIR}/perf_script_${TIMESTAMP}.txt"
        echo "    (Use with FlameGraph tools for visualization)"
    fi
}

# ============================================================================
# Profile with gprof (Call Graph)
# ============================================================================
profile_with_gprof() {
    echo "Running tests with gprof profiling..."

    # Make sure profiling was enabled at build time
    if ! grep -q "fno-omit-frame-pointer" compile_commands.json 2>/dev/null; then
        echo "WARNING: Frame pointers may not be enabled. Rebuild with:"
        echo "  cmake -DENABLE_PROFILING=ON .."
        echo "  make"
    fi

    GPROF_OUT="${OUTPUT_DIR}/gprof_${TIMESTAMP}.txt"
    GMON_OUT="${OUTPUT_DIR}/gmon.out"

    # Run tests (generates gmon.out)
    "${BUILD_DIR}/bin/test_phase0_unit" > /dev/null 2>&1

    # Move gmon.out and generate gprof report
    if [ -f "gmon.out" ]; then
        mv gmon.out "${GMON_OUT}"
        gprof "${BUILD_DIR}/bin/test_phase0_unit" "${GMON_OUT}" > "${GPROF_OUT}"

        echo "  Results:"
        echo "    gmon data: ${GMON_OUT}"
        echo "    Report:    ${GPROF_OUT}"
        echo ""

        # Show top 20 functions
        echo "  Top 20 Functions by Time:"
        head -30 "${GPROF_OUT}" | grep -E "^ " | head -20
    else
        echo "  ERROR: gmon.out not generated. Ensure profiling is enabled."
        return 1
    fi
}

# ============================================================================
# Profile with Instruments (macOS)
# ============================================================================
profile_with_instruments() {
    if [ "$OS" != "macos" ]; then
        echo "ERROR: Instruments is macOS-only"
        return 1
    fi

    if ! command -v xcrun &> /dev/null; then
        echo "ERROR: xcrun not found. Install Xcode command line tools."
        return 1
    fi

    echo "Running tests with Instruments Time Profiler..."

    TRACE_FILE="${OUTPUT_DIR}/profile_${TIMESTAMP}.trace"

    # Run with Time Profiler
    xcrun xctrace record \
        --instrument "System Trace" \
        --output "${TRACE_FILE}" \
        --launch "${BUILD_DIR}/bin/test_phase0_unit" \
        2>&1 | tee "${OUTPUT_DIR}/instruments_${TIMESTAMP}.log" || true

    echo "  Results:"
    echo "    Trace: ${TRACE_FILE}"
    echo "    (Open in Xcode Instruments or Xcode)"
}

# ============================================================================
# Main Execution
# ============================================================================

case "$MODE" in
    perf)
        profile_with_perf
        ;;
    gprof)
        profile_with_gprof
        ;;
    instruments)
        profile_with_instruments
        ;;
    all)
        echo "Running all available profilers..."
        echo ""

        profile_with_gprof || echo "gprof profiling failed"
        echo ""

        if [ "$OS" = "linux" ]; then
            profile_with_perf || echo "perf profiling failed"
        elif [ "$OS" = "macos" ]; then
            profile_with_instruments || echo "Instruments profiling failed"
        fi
        ;;
    *)
        echo "ERROR: Unknown mode: $MODE"
        echo "Valid modes: perf, gprof, instruments, all"
        exit 1
        ;;
esac

echo "============================================================================"
echo "Profiling Complete"
echo "============================================================================"
echo ""
echo "Output Directory: ${OUTPUT_DIR}"
echo "Timestamp: ${TIMESTAMP}"
echo ""
echo "Next Steps:"
echo "  1. Review profiling results in ${OUTPUT_DIR}"
echo "  2. Identify hot paths and optimization opportunities"
echo "  3. Compare with baseline benchmarks"
echo "  4. Document findings in PERFORMANCE.md"
echo ""
