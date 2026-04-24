// ============================================================================
// ThreadPool Performance Benchmarks
// ============================================================================
// Measures performance of worker loop and task scheduling:
// - Task enqueueing overhead
// - Task dispatch latency
// - Worker throughput (tasks/second)
// - Contention on different thread counts

#include <benchmark/benchmark.h>
#include <graph/ThreadPool.hpp>
#include <chrono>
#include <atomic>
#include <vector>

using namespace graph;

// ============================================================================
// Task Scheduling Overhead
// ============================================================================

static void BM_ThreadPool_TaskQueue_Sequential(benchmark::State& state) {
    // Measure task queueing overhead in sequential mode
    ThreadPool pool(1);  // Single worker
    pool.Start();

    std::atomic<int> completed{0};

    for (auto _ : state) {
        pool.QueueTask([&completed]() {
            completed++;
        });
    }

    // Wait for all tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.Stop();
}
BENCHMARK(BM_ThreadPool_TaskQueue_Sequential)->Iterations(10000);

static void BM_ThreadPool_TaskQueue_MultiWorker(benchmark::State& state) {
    // Measure queueing overhead with multiple workers
    int num_workers = state.range(0);
    ThreadPool pool(num_workers);
    pool.Start();

    std::atomic<int> completed{0};

    for (auto _ : state) {
        pool.QueueTask([&completed]() {
            completed++;
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.Stop();
}
BENCHMARK(BM_ThreadPool_TaskQueue_MultiWorker)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Iterations(10000);

// ============================================================================
// Worker Loop Throughput
// ============================================================================

static void BM_ThreadPool_WorkerThroughput(benchmark::State& state) {
    // Measure tasks/second processed by worker threads
    int num_workers = state.range(0);
    ThreadPool pool(num_workers);
    pool.Start();

    std::atomic<int> count{0};

    for (auto _ : state) {
        for (int i = 0; i < 1000; i++) {
            pool.QueueTask([&count]() {
                count++;
            });
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    state.counters["tasks/sec"] = benchmark::Counter(
        static_cast<double>(count),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}
BENCHMARK(BM_ThreadPool_WorkerThroughput)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Iterations(100);

// ============================================================================
// Task Latency (enqueue to execution)
// ============================================================================

static void BM_ThreadPool_TaskLatency(benchmark::State& state) {
    // Measure latency from task enqueue to execution start
    ThreadPool pool(4);
    pool.Start();

    std::vector<std::chrono::nanoseconds> latencies;

    for (auto _ : state) {
        auto enqueue_time = std::chrono::high_resolution_clock::now();

        bool executed = false;
        pool.QueueTask([enqueue_time, &executed, &latencies]() {
            auto exec_time = std::chrono::high_resolution_clock::now();
            latencies.push_back(
                std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time - enqueue_time)
            );
            executed = true;
        });

        // Wait for execution
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    pool.Stop();

    // Report latency statistics
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        state.counters["latency_p50_us"] =
            benchmark::Counter(latencies[latencies.size() / 2].count() / 1000.0);
        state.counters["latency_p99_us"] =
            benchmark::Counter(latencies[(latencies.size() * 99) / 100].count() / 1000.0);
    }
}
BENCHMARK(BM_ThreadPool_TaskLatency)->Iterations(1000);

// ============================================================================
// Lock-free Queue Contention (internal)
// ============================================================================

static void BM_ThreadPool_Contention_ProducerConsumer(benchmark::State& state) {
    // Benchmark contention on thread pool's internal queue
    // Multiple producer threads queueing tasks, worker threads consuming
    ThreadPool pool(state.range(0));  // Number of worker threads
    pool.Start();

    std::atomic<int> task_count{0};

    if (state.thread_index() < 2) {
        // Producer threads (2 threads)
        for (auto _ : state) {
            for (int i = 0; i < 100; i++) {
                pool.QueueTask([&task_count]() {
                    task_count++;
                });
            }
        }
    } else {
        // Wait for workers to process (metrics collection thread)
        for (auto _ : state) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    pool.Stop();
}
BENCHMARK(BM_ThreadPool_Contention_ProducerConsumer)
    ->Threads(2)
    ->Threads(4)
    ->Arg(1)
    ->Arg(4)
    ->Iterations(100);

// ============================================================================
// Task Size Variations
// ============================================================================

static void BM_ThreadPool_SmallTask(benchmark::State& state) {
    // Overhead of very small tasks (lambda overhead)
    ThreadPool pool(4);
    pool.Start();

    for (auto _ : state) {
        pool.QueueTask([]() {
            // No-op task
            benchmark::DoNotOptimize(1 + 1);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.Stop();
}
BENCHMARK(BM_ThreadPool_SmallTask)->Iterations(100000);

static void BM_ThreadPool_MediumTask(benchmark::State& state) {
    // Medium-sized task with some work
    ThreadPool pool(4);
    pool.Start();

    for (auto _ : state) {
        pool.QueueTask([]() {
            volatile int sum = 0;
            for (int i = 0; i < 100; i++) {
                sum += i;
            }
            benchmark::DoNotOptimize(sum);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.Stop();
}
BENCHMARK(BM_ThreadPool_MediumTask)->Iterations(10000);

BENCHMARK_MAIN();
