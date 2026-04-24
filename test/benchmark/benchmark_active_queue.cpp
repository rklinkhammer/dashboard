// ============================================================================
// ActiveQueue Performance Benchmarks
// ============================================================================
// Measures performance of lock-free queue operations:
// - Enqueue with sorted insertion (O(n) worst case)
// - Non-blocking dequeue (O(1) fast path)
// - Blocking dequeue with condition variable
// - Metrics collection overhead

#include <benchmark/benchmark.h>
#include <core/ActiveQueue.hpp>
#include <chrono>
#include <thread>
#include <vector>

using namespace core;

// ============================================================================
// Enqueue/Dequeue Operations
// ============================================================================

static void BM_ActiveQueue_EnqueueDequeue_FastPath(benchmark::State& state) {
    // Fast path: insert at queue head (O(1))
    ActiveQueue<int> queue;
    int counter = 0;

    for (auto _ : state) {
        queue.Enqueue(counter++);
        int value;
        benchmark::DoNotOptimize(queue.DequeueNonBlocking(value));
    }
}
BENCHMARK(BM_ActiveQueue_EnqueueDequeue_FastPath)->Iterations(100000);

static void BM_ActiveQueue_EnqueueDequeue_SortedInsert(benchmark::State& state) {
    // Sorted insertion benchmark: varying queue sizes
    int num_items = state.range(0);
    ActiveQueue<int> queue;

    // Set a custom comparator for priority-based insertion
    queue.SetComparator([](const int& a, const int& b) {
        return a < b;  // Min-heap order
    });

    // Pre-fill queue
    for (int i = 0; i < num_items; i++) {
        queue.Enqueue(100 - i);
    }

    for (auto _ : state) {
        queue.Enqueue(50);
        int value;
        benchmark::DoNotOptimize(queue.DequeueNonBlocking(value));
    }
}
BENCHMARK(BM_ActiveQueue_EnqueueDequeue_SortedInsert)
    ->RangeMultiplier(2)
    ->Range(1, 1024)
    ->Iterations(1000);

static void BM_ActiveQueue_BlockingDequeue_NoWait(benchmark::State& state) {
    // Blocking dequeue when item is available (no actual wait)
    ActiveQueue<int> queue;

    for (auto _ : state) {
        queue.Enqueue(42);
        int value;
        benchmark::DoNotOptimize(queue.Dequeue(value));
    }
}
BENCHMARK(BM_ActiveQueue_BlockingDequeue_NoWait)->Iterations(100000);

static void BM_ActiveQueue_NonBlockingDequeue_Empty(benchmark::State& state) {
    // Non-blocking dequeue on empty queue (fast fail path)
    ActiveQueue<int> queue;

    for (auto _ : state) {
        int value;
        benchmark::DoNotOptimize(queue.DequeueNonBlocking(value));
    }
}
BENCHMARK(BM_ActiveQueue_NonBlockingDequeue_Empty)->Iterations(1000000);

// ============================================================================
// Metrics Collection Overhead
// ============================================================================

static void BM_ActiveQueue_Metrics_Collection(benchmark::State& state) {
    // Measure overhead of metrics tracking on enqueue
    ActiveQueue<int> queue;
    queue.EnableMetrics(true);

    for (auto _ : state) {
        for (int i = 0; i < 10; i++) {
            queue.Enqueue(i);
        }
        // Metrics are collected during operations, read is cheap
        int value;
        queue.DequeueNonBlocking(value);
    }
}
BENCHMARK(BM_ActiveQueue_Metrics_Collection)->Iterations(10000);

// ============================================================================
// Multi-threaded Contention (lock-free semantics)
// ============================================================================

static void BM_ActiveQueue_EnqueueDequeue_Contention(benchmark::State& state) {
    // Two threads contending on queue: measure lock contention
    int num_threads = state.range(0);
    ActiveQueue<int> queue;

    if (state.thread_index() == 0) {
        for (auto _ : state) {
            for (int i = 0; i < 100; i++) {
                queue.Enqueue(i);
            }
        }
    } else {
        for (auto _ : state) {
            for (int i = 0; i < 100; i++) {
                int value;
                benchmark::DoNotOptimize(queue.DequeueNonBlocking(value));
            }
        }
    }
}
BENCHMARK(BM_ActiveQueue_EnqueueDequeue_Contention)
    ->Threads(2)
    ->Threads(4)
    ->Iterations(10000);

// ============================================================================
// Capacity and Bounding
// ============================================================================

static void BM_ActiveQueue_BoundedQueue_Enqueue(benchmark::State& state) {
    // Measure bounded queue enqueue performance
    ActiveQueue<int> queue(100);  // Bounded to 100 items

    for (auto _ : state) {
        for (int i = 0; i < 50; i++) {
            queue.Enqueue(i);
        }
        // Drain queue
        int value;
        while (queue.DequeueNonBlocking(value)) {
            benchmark::DoNotOptimize(value);
        }
    }
}
BENCHMARK(BM_ActiveQueue_BoundedQueue_Enqueue)->Iterations(1000);

BENCHMARK_MAIN();
