# Performance Profiling & Benchmarking Guide

## Overview

This document describes the performance profiling infrastructure for gdashboard, including:
- **Coverage Analysis**: Code coverage metrics and testing gaps
- **Performance Benchmarks**: Baseline performance metrics for hot paths
- **Profiling Tools**: How to profile execution and identify bottlenecks
- **Performance Budgets**: Target performance thresholds by component

## Build Configuration

### Coverage Instrumentation
Enable code coverage with gcov/lcov:
```bash
cmake -DENABLE_COVERAGE=ON ..
make
make coverage
```

**Output:**
- `coverage/index.html` - HTML coverage report
- `coverage.json` - JSON results for CI/CD
- `coverage-summary.txt` - Terminal summary

### Profiling Support
Enable frame pointers and profiling flags:
```bash
cmake -DENABLE_PROFILING=ON ..
make
```

This enables:
- Frame pointers (`-fno-omit-frame-pointer`)
- Call graph profiling (`-p` flag for gprof)
- Debug symbols (`-g` flag)

## Code Coverage Status

### Baseline Metrics (All Source Code)
- **Lines**: 55.3% (4,393 of 7,941 lines)
- **Functions**: 53.4% (2,029 of 3,801 functions)
- **Target**: 95%+ coverage (user preference)

### Coverage by Component

| Component | Lines | Functions | Status |
|-----------|-------|-----------|--------|
| Core Layer | ~60% | ~65% | Well-tested (ThreadPool, ActiveQueue) |
| Graph Layer | ~50% | ~48% | Moderate (NodeFactory, PortTypes) |
| Application Layer | ~45% | ~50% | Lower (Policies, CapabilityBus) |
| Avionics Nodes | ~58% | ~60% | Good (DataInjection, FlightFSM, etc.) |
| UI Components | ~0% | ~5% | Not tested (MetricsTileWindow) |

### Coverage Gaps

**Not Covered:**
1. **UI Code** (`include/ui/MetricsTileWindow.hpp`)
   - FTXUI rendering logic not testable without UI framework integration
   - Recommendation: Accept as untestable or use UI testing framework

2. **Error Paths**
   - Exception handling in some error scenarios
   - Plugin loading failures with missing/corrupt files
   - Recommendation: Add explicit error scenario tests

3. **Platform-Specific Code**
   - Linux vs macOS conditional compilation
   - Recommendation: Test on both platforms

4. **Plugin Initialization**
   - Global static initializers for plugin registration
   - Recommendation: Mock plugin system or add explicit init tests

## Performance Benchmarks

### Benchmark Suite
Four comprehensive benchmark suites measure critical hot paths:

#### 1. ActiveQueue Benchmarks (`benchmark_active_queue`)
**Purpose:** Lock-free queue performance (core synchronization primitive)

**Key Benchmarks:**
- `BM_ActiveQueue_EnqueueDequeue_FastPath` - O(1) insertion at head
- `BM_ActiveQueue_EnqueueDequeue_SortedInsert` - O(n) sorted insertion with varying queue sizes
- `BM_ActiveQueue_BlockingDequeue_NoWait` - Blocking dequeue when ready
- `BM_ActiveQueue_NonBlockingDequeue_Empty` - Fast-fail on empty queue
- `BM_ActiveQueue_Metrics_Collection` - Metrics overhead
- `BM_ActiveQueue_EnqueueDequeue_Contention` - Multi-threaded lock contention

**Targets:**
- Fast path enqueue/dequeue: < 1μs per operation
- Sorted insertion (100-item queue): < 10μs
- Metrics overhead: < 5% of operation time
- Lock contention (2-4 threads): < 10% degradation

#### 2. ThreadPool Benchmarks (`benchmark_thread_pool`)
**Purpose:** Worker loop performance and task scheduling overhead

**Key Benchmarks:**
- `BM_ThreadPool_TaskQueue_Sequential` - Single worker throughput
- `BM_ThreadPool_TaskQueue_MultiWorker` - Multi-worker contention
- `BM_ThreadPool_WorkerThroughput` - Tasks/second processed
- `BM_ThreadPool_TaskLatency` - Enqueue-to-execution latency (p50/p99)
- `BM_ThreadPool_Contention_ProducerConsumer` - Multi-producer contention
- `BM_ThreadPool_SmallTask` - Overhead of small no-op tasks
- `BM_ThreadPool_MediumTask` - Realistic task workload

**Targets:**
- Task enqueue latency: < 1μs
- Worker throughput: > 100K tasks/sec per worker
- Dispatch latency (p50): < 100μs (p99: < 500μs)
- Small task overhead: < 50ns per task
- Contention degradation (4 threads): < 15%

#### 3. Message Routing Benchmarks (`benchmark_message_routing`)
**Purpose:** Port and message operation overhead

**Key Benchmarks:**
- `BM_Message_Creation_Throughput` - Message allocation rate
- `BM_Message_Copy_Throughput` - Copy semantic cost
- `BM_Port_MetadataGeneration` - Compile-time machinery runtime cost
- `BM_Message_TypeBasedRouting_Lookup` - Routing table lookup
- `BM_Message_AllocationRate` - Heap allocation patterns
- `BM_MessageQueue_Throughput` - Queuing through multiple ports
- `BM_Port_IndexAccess` - Port constant access

**Targets:**
- Message creation: > 1M messages/sec
- Message copy: > 500K copies/sec
- Port metadata: Compile-time only (0 runtime cost)
- Routing lookup: < 1ns per lookup
- Port index access: < 1ns per access

#### 4. NodeFactory Benchmarks (`benchmark_node_factory`)
**Purpose:** Node creation overhead and plugin system performance

**Key Benchmarks:**
- `BM_NodeFactory_StaticNode_Creation` - Built-in node instantiation
- `BM_NodeFactory_DynamicNode_Discovery` - Plugin discovery cost
- `BM_NodeFactory_Initialization_Cold` - First-time factory setup
- `BM_NodeFactory_Initialization_Warm` - Cached factory use
- `BM_NodeFactory_Registration_Cost` - Node registration overhead
- `BM_NodeFactory_BulkCreation` - Pipeline setup with many nodes
- `BM_NodeFactory_PluginLookup` - Plugin symbol lookup
- `BM_NodeFactory_MemoryUsage` - Factory memory overhead

**Targets:**
- Static node creation: < 10μs per node
- Dynamic node creation: < 50μs per node
- Factory initialization (cold): < 100ms one-time
- Plugin discovery: < 1ms per plugin
- Bulk node creation (100 nodes): < 500μs total

### Running Benchmarks

```bash
# Run all benchmarks and save baseline results
./scripts/run_benchmarks.sh

# Run specific benchmark
./bin/benchmark_active_queue

# Run with custom parameters
./bin/benchmark_thread_pool --benchmark_filter="MultiWorker" --benchmark_repetitions=3

# Compare against baseline
diff benchmark_results/bench_active_queue_baseline.json benchmark_results/bench_active_queue_20260423.json
```

### Benchmark Results Location
- `benchmark_results/bench_active_queue_*.json` - ActiveQueue metrics
- `benchmark_results/bench_thread_pool_*.json` - ThreadPool metrics
- `benchmark_results/bench_message_routing_*.json` - Message routing metrics
- `benchmark_results/bench_node_factory_*.json` - NodeFactory metrics

## Profiling Tools

### Using perf (Linux)
```bash
# Profile with perf and generate call graph
./scripts/profile_tests.sh perf

# View results
perf report -i ./profile_results/perf_*.data

# Generate flamegraph (with FlameGraph tools installed)
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

**Metrics Captured:**
- CPU cycles
- Cache misses (L1, LL)
- Branch mispredictions
- Call graphs with time per function

### Using gprof (All Platforms)
```bash
# Profile with gprof (requires ENABLE_PROFILING=ON at build)
./scripts/profile_tests.sh gprof

# View results
cat ./profile_results/gprof_*.txt | head -100
```

**Output:**
- Flat profile (% time per function)
- Call graph (parent/child relationships)
- Index listing (function definitions)

### Using Instruments (macOS)
```bash
# Profile with Xcode Instruments
./scripts/profile_tests.sh instruments

# Open in Xcode
open ./profile_results/profile_*.trace
```

**Features:**
- Time Profiler (CPU time)
- System Trace (system calls)
- Memory profiler (allocations)
- Custom instrument setup

## Lock-Free Semantics Validation

The ActiveQueue implementation uses lock-free techniques to minimize latency:

### CAS-Based Operations
- **Load-acquire/store-release semantics** ensure memory ordering
- **Metrics collection** via atomic counters doesn't affect lock-freedom
- **Metrics validation**: Track `enqueue_rejections`, `dequeue_empty`, `total_wait_time_ns`

### Validation Benchmarks
1. **Enqueue under contention**: Measure CAS retry rates
2. **Dequeue on empty**: Verify constant-time fast path
3. **Sorted insertion cost**: O(n) worst case acceptable for small queues
4. **Condition variable latency**: Measure wake-up time

### Expected Results
- No degradation with increasing thread count (lock-free)
- Linear scaling up to CPU core count
- Minimal variance in latency distribution

## Performance Budgets

### Latency Budgets (per operation)
| Operation | Budget | Critical Path |
|-----------|--------|----------------|
| Message enqueue | 1μs | Graph execution |
| Task dispatch | 1μs | Worker scheduling |
| Port lookup | 10ns | Message routing |
| Node creation | 10μs | Setup (not runtime) |

### Throughput Budgets
| Metric | Budget | Rationale |
|--------|--------|-----------|
| Messages/sec | > 100K | Real-time data rate |
| Tasks/sec | > 100K | Worker throughput |
| Nodes/sec | > 100 | Setup phase only |

### Memory Budgets
| Component | Budget | Notes |
|-----------|--------|-------|
| ActiveQueue (1K items) | < 100KB | Queue + metrics |
| ThreadPool (4 workers) | < 50MB | Stack + state |
| NodeFactory | < 10MB | Plugin registry + cache |

## Regression Detection

### CI/CD Integration
```bash
# Save baseline after optimization
cp benchmark_results/bench_active_queue_20260423.json \
   benchmark_results/bench_active_queue_baseline.json

# In CI, compare current run against baseline
diff -q baseline.json current.json || {
    echo "Performance regression detected!"
    exit 1
}
```

### Acceptable Variance
- Small benchmarks (< 1μs): ±20% variance acceptable
- Medium benchmarks (1-100μs): ±10% variance acceptable
- Large benchmarks (> 100μs): ±5% variance acceptable

## Coverage Report

### View HTML Report
```bash
open coverage/index.html
```

### Generate Coverage Report with CMake
```bash
cmake -DENABLE_COVERAGE=ON ..
make
make coverage
```

### Interpretation
- **Red lines**: Not executed
- **Green lines**: Executed
- **Yellow lines**: Only partially executed (condition branches)

## Optimization Opportunities

### Hot Paths Identified
1. **ActiveQueue sorted insertion** - O(n) could use binary search
2. **NodeFactory plugin lookup** - Could cache dlsym results
3. **Message allocation** - Frequent allocations could use memory pool
4. **ThreadPool condition variable** - Spurious wakeups could be reduced

### Further Analysis
Run profiling to identify specific functions consuming CPU time:
```bash
./scripts/profile_tests.sh all
```

Review call graphs to find:
- Unexpected function calls in hot paths
- High cache miss rates
- Branch mispredictions
- Lock contention points

## Documentation

- Coverage gaps documented above
- Performance targets specified for each component
- Profiling tools described with usage examples
- Regression detection procedure documented

## Next Steps

1. **Establish Performance Baselines**
   - ✅ Run benchmarks and record JSON results
   - ✅ Identify performance outliers

2. **Profile Production Workload**
   - ✅ Set up profiling scripts
   - Run profiler on realistic workload
   - Identify actual hot paths vs theoretical

3. **Optimize Based on Profiling**
   - Focus on measured hot paths first
   - Document rationale for optimizations
   - Verify improvements with benchmarks

4. **Improve Coverage**
   - Add tests for currently uncovered paths
   - Consider UI testing framework
   - Add explicit error scenario tests

5. **CI/CD Integration**
   - Add benchmark regression detection
   - Add coverage trend tracking
   - Create performance dashboards

---

**Last Updated**: April 23, 2026
**Benchmark Suite**: 4 hot paths, 40+ individual benchmarks
**Coverage Tooling**: lcov/genhtml with 95%+ target
**Profiling Tools**: perf, gprof, Instruments support
