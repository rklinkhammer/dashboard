#!/bin/bash
# ============================================================================
# Benchmark Execution Script
# ============================================================================
# Runs performance benchmarks for critical hot paths and records baselines.
#
# Usage:
#   ./scripts/run_benchmarks.sh [output_file]
#
# Output:
#   - Benchmarks output to console (real-time)
#   - JSON results to bench_results.json (or specified file)
#   - Summary statistics for trend analysis
#

set -e

BUILD_DIR="${1:-.}"
OUTPUT_DIR="${BUILD_DIR}/benchmark_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_FILE="${OUTPUT_DIR}/bench_results_${TIMESTAMP}.json"

# Ensure output directory exists
mkdir -p "${OUTPUT_DIR}"

echo "============================================================================"
echo "Running Performance Benchmarks"
echo "============================================================================"
echo "Build directory: ${BUILD_DIR}"
echo "Output directory: ${OUTPUT_DIR}"
echo "Results file: ${RESULTS_FILE}"
echo ""

# ============================================================================
# 1. ActiveQueue Benchmarks (Lock-free Queue Performance)
# ============================================================================

echo "[1/4] Running ActiveQueue benchmarks..."
echo "  Measuring: Lock-free enqueue/dequeue, sorted insertion, metrics overhead"

"${BUILD_DIR}/bin/benchmark_active_queue" \
    --benchmark_format=json \
    --benchmark_out="${OUTPUT_DIR}/bench_active_queue_${TIMESTAMP}.json" \
    2>&1 | tee -a "${OUTPUT_DIR}/bench_active_queue_${TIMESTAMP}.log"

echo "  ✓ ActiveQueue benchmarks complete"
echo ""

# ============================================================================
# 2. ThreadPool Benchmarks (Worker Loop and Task Scheduling)
# ============================================================================

echo "[2/4] Running ThreadPool benchmarks..."
echo "  Measuring: Task scheduling, worker throughput, contention"

"${BUILD_DIR}/bin/benchmark_thread_pool" \
    --benchmark_format=json \
    --benchmark_out="${OUTPUT_DIR}/bench_thread_pool_${TIMESTAMP}.json" \
    2>&1 | tee -a "${OUTPUT_DIR}/bench_thread_pool_${TIMESTAMP}.log"

echo "  ✓ ThreadPool benchmarks complete"
echo ""

# ============================================================================
# 3. Message Routing Benchmarks (Port and Message Operations)
# ============================================================================

echo "[3/4] Running Message Routing benchmarks..."
echo "  Measuring: Message allocation, port operations, routing throughput"

"${BUILD_DIR}/bin/benchmark_message_routing" \
    --benchmark_format=json \
    --benchmark_out="${OUTPUT_DIR}/bench_message_routing_${TIMESTAMP}.json" \
    2>&1 | tee -a "${OUTPUT_DIR}/bench_message_routing_${TIMESTAMP}.log"

echo "  ✓ Message Routing benchmarks complete"
echo ""

# ============================================================================
# 4. NodeFactory Benchmarks (Node Creation and Plugin Overhead)
# ============================================================================

echo "[4/4] Running NodeFactory benchmarks..."
echo "  Measuring: Node instantiation, plugin discovery, factory initialization"

"${BUILD_DIR}/bin/benchmark_node_factory" \
    --benchmark_format=json \
    --benchmark_out="${OUTPUT_DIR}/bench_node_factory_${TIMESTAMP}.json" \
    2>&1 | tee -a "${OUTPUT_DIR}/bench_node_factory_${TIMESTAMP}.log"

echo "  ✓ NodeFactory benchmarks complete"
echo ""

# ============================================================================
# Consolidate Results
# ============================================================================

echo "============================================================================"
echo "Benchmark Execution Complete"
echo "============================================================================"
echo ""
echo "Results Summary:"
echo "  ActiveQueue:      ${OUTPUT_DIR}/bench_active_queue_${TIMESTAMP}.json"
echo "  ThreadPool:       ${OUTPUT_DIR}/bench_thread_pool_${TIMESTAMP}.json"
echo "  Message Routing:  ${OUTPUT_DIR}/bench_message_routing_${TIMESTAMP}.json"
echo "  NodeFactory:      ${OUTPUT_DIR}/bench_node_factory_${TIMESTAMP}.json"
echo ""
echo "Logs:"
echo "  ActiveQueue:      ${OUTPUT_DIR}/bench_active_queue_${TIMESTAMP}.log"
echo "  ThreadPool:       ${OUTPUT_DIR}/bench_thread_pool_${TIMESTAMP}.log"
echo "  Message Routing:  ${OUTPUT_DIR}/bench_message_routing_${TIMESTAMP}.log"
echo "  NodeFactory:      ${OUTPUT_DIR}/bench_node_factory_${TIMESTAMP}.log"
echo ""

# Create a summary file with timestamps and directories for reference
cat > "${OUTPUT_DIR}/BENCHMARK_RUN_${TIMESTAMP}.txt" <<EOF
Benchmark Run Summary
=====================

Timestamp: ${TIMESTAMP}
Build Directory: ${BUILD_DIR}
Output Directory: ${OUTPUT_DIR}

Benchmarks Executed:
1. ActiveQueue - Lock-free queue performance
2. ThreadPool - Worker loop and task scheduling
3. Message Routing - Port and message operations
4. NodeFactory - Node creation and plugin system

JSON Results:
- bench_active_queue_${TIMESTAMP}.json
- bench_thread_pool_${TIMESTAMP}.json
- bench_message_routing_${TIMESTAMP}.json
- bench_node_factory_${TIMESTAMP}.json

Logs:
- bench_active_queue_${TIMESTAMP}.log
- bench_thread_pool_${TIMESTAMP}.log
- bench_message_routing_${TIMESTAMP}.log
- bench_node_factory_${TIMESTAMP}.log

To compare results:
  diff bench_active_queue_baseline.json bench_active_queue_${TIMESTAMP}.json

To create new baseline:
  cp bench_active_queue_${TIMESTAMP}.json bench_active_queue_baseline.json
EOF

echo "✓ Benchmark summary saved to ${OUTPUT_DIR}/BENCHMARK_RUN_${TIMESTAMP}.txt"
