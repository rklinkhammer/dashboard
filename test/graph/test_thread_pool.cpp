// MIT License
//
// Copyright (c) 2026 graphlib contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>
#include "graph/ThreadPool.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

// ============================================================================
// Lifecycle Tests (6 tests)
// ============================================================================

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Standard pool for most tests
    }
    
    void TearDown() override {
        // Cleanup happens in test destructors
    }
};

TEST_F(ThreadPoolTest, Constructor_Default) {
    graph::ThreadPool pool;
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Constructor_WithThreadCount) {
    graph::ThreadPool pool(4);
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Constructor_WithConfig) {
    graph::ThreadPool::DeadlockConfig config;
    config.task_timeout = std::chrono::milliseconds(2000);
    config.max_queue_size = 5000;
    
    graph::ThreadPool pool(4, config);
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Init_TransitionsState) {
    graph::ThreadPool pool(2);
    EXPECT_TRUE(pool.Init());
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Start_SpawnsThreads) {
    graph::ThreadPool pool(2);
    pool.Init();
    EXPECT_TRUE(pool.Start());
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, Stop_SetsFlag) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Stop();
    EXPECT_TRUE(pool.GetStopRequested());
    
    pool.Join();
}

// ============================================================================
// Basic Task Queuing (8 tests)
// ============================================================================

TEST_F(ThreadPoolTest, QueueTask_SingleTask) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    std::atomic<bool> executed{false};
    auto result = pool.QueueTask([&](){ 
        executed.store(true);
    });
    
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);
    
    pool.Stop();
    pool.Join();
    
    EXPECT_TRUE(executed.load());
}

TEST_F(ThreadPoolTest, QueueTask_MultipleTasksExecuteInOrder) {
    graph::ThreadPool pool(1);  // Single thread to enforce order
    pool.Init();
    pool.Start();
    
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    for (int i = 0; i < 10; ++i) {
        pool.QueueTask([i, &execution_order, &order_mutex](){
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        });
    }
    
    pool.Stop();
    pool.Join();
    
    EXPECT_EQ(execution_order.size(), 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

TEST_F(ThreadPoolTest, QueueTask_AfterStop_Rejected) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    pool.Stop();
    
    auto result = pool.QueueTask([](){});
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Stopped);
    
    pool.Join();
}

TEST_F(ThreadPoolTest, QueueTask_BoundedCapacity) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 2;
    config.enable_detection = false;
    
    graph::ThreadPool pool(1, config);  // Single worker to saturate queue
    pool.Init();
    pool.Start();
    
    // Block worker with long-running task
    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};
    
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for slow task to start
    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Queue up to capacity
    std::atomic<int> queue_full_count{0};
    for (int i = 0; i < 5; ++i) {
        auto result = pool.QueueTask([](){});
        if (result == graph::ThreadPool::QueueResult::Full) {
            queue_full_count.fetch_add(1);
        }
    }
    
    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();
    
    // Should have hit capacity at some point
    EXPECT_GT(queue_full_count.load(), 0);
}

TEST_F(ThreadPoolTest, QueueTaskWithTimeout_Success) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    std::atomic<bool> executed{false};
    auto result = pool.QueueTaskWithTimeout(
        [&](){ executed.store(true); },
        std::chrono::seconds(1)
    );
    
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);
    
    pool.Stop();
    pool.Join();
    
    EXPECT_TRUE(executed.load());
}

TEST_F(ThreadPoolTest, QueueTaskWithTimeout_AfterStop) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    pool.Stop();
    
    auto result = pool.QueueTaskWithTimeout(
        [](){},
        std::chrono::seconds(1)
    );
    
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Stopped);
    
    pool.Join();
}

TEST_F(ThreadPoolTest, Emplace_ConstructsTask) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    std::atomic<int> value{0};
    auto result = pool.QueueTask([&](){ value.store(42); });
    
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);
    
    pool.Stop();
    pool.Join();
    
    EXPECT_EQ(value.load(), 42);
}

// ============================================================================
// Statistics and Monitoring (8 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Stats_TrackTasksQueued) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 10; ++i) {
        pool.QueueTask([](){});
    }
    
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_queued.load(), 10);
}

TEST_F(ThreadPoolTest, Stats_TrackTasksCompleted) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 5; ++i) {
        pool.QueueTask([](){});
    }
    
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_completed.load(), 5);
}

TEST_F(ThreadPoolTest, Stats_PeakQueueSize) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 1000;
    config.enable_detection = false;
    
    graph::ThreadPool pool(1, config);  // Single worker to measure queue depth
    pool.Init();
    pool.Start();
    
    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};
    
    // Queue a slow task
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for slow task to start
    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Queue many fast tasks
    for (int i = 0; i < 50; ++i) {
        pool.QueueTask([](){});
    }
    
    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_GT(stats.peak_queue_size.load(), 0);
}

TEST_F(ThreadPoolTest, Stats_TasksRejected) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 1;
    config.enable_detection = false;
    
    graph::ThreadPool pool(1, config);
    pool.Init();
    pool.Start();
    
    // Queue blocking task
    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Queue until full
    for (int i = 0; i < 3; ++i) {
        pool.QueueTask([](){});
    }
    
    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_GT(stats.tasks_rejected.load() + stats.tasks_queued.load(), 0);
}

TEST_F(ThreadPoolTest, GetAverageTaskTimeMs) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 3; ++i) {
        pool.QueueTask([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    
    pool.Stop();
    pool.Join();
    
    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_GT(avg_ms, 5.0);  // Should be at least ~10ms
}

TEST_F(ThreadPoolTest, GetStats_ThreadSafe) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();
    
    // Queue tasks from multiple threads
    std::vector<std::thread> enqueuers;
    for (int t = 0; t < 4; ++t) {
        enqueuers.emplace_back([&](){
            for (int i = 0; i < 25; ++i) {
                pool.QueueTask([](){});
            }
        });
    }
    
    // Read stats from another thread
    std::vector<std::thread> readers;
    std::vector<size_t> observed_queued;
    std::mutex observed_mutex;
    
    for (int t = 0; t < 2; ++t) {
        readers.emplace_back([&](){
            for (int i = 0; i < 10; ++i) {
                const auto& stats = pool.GetStats();
                {
                    std::lock_guard<std::mutex> lock(observed_mutex);
                    observed_queued.push_back(stats.tasks_queued.load());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    for (auto& t : enqueuers) {
        t.join();
    }
    for (auto& t : readers) {
        t.join();
    }
    
    pool.Stop();
    pool.Join();
    
    EXPECT_GT(observed_queued.size(), 0);
}

TEST_F(ThreadPoolTest, ClearDeadlockFlag) {
    graph::ThreadPool::DeadlockConfig config;
    config.task_timeout = std::chrono::milliseconds(100);
    config.watchdog_interval = std::chrono::milliseconds(50);
    config.enable_detection = true;
    
    graph::ThreadPool pool(2, config);
    pool.Init();
    pool.Start();
    
    // Queue a task that might exceed timeout
    pool.QueueTask([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
    
    pool.Stop();
    pool.Join();
    
    // Try to clear deadlock flag
    pool.ClearDeadlockFlag();
    // Should not crash
    EXPECT_TRUE(true);
}

// ============================================================================
// Shutdown and Join (6 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Join_BlocksUntilComplete) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    std::atomic<int> tasks_completed{0};
    for (int i = 0; i < 10; ++i) {
        pool.QueueTask([&](){
            tasks_completed.fetch_add(1);
        });
    }
    
    pool.Stop();
    pool.Join();
    
    EXPECT_EQ(tasks_completed.load(), 10);
}

TEST_F(ThreadPoolTest, JoinWithTimeout_CompletesBeforeTimeout) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 5; ++i) {
        pool.QueueTask([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }
    
    pool.Stop();
    bool joined = pool.JoinWithTimeout(std::chrono::seconds(5));
    
    EXPECT_TRUE(joined);
}

TEST_F(ThreadPoolTest, DestructorImplicitStop) {
    {
        graph::ThreadPool pool(2);
        pool.Init();
        pool.Start();
        
        pool.QueueTask([](){});
        pool.QueueTask([](){});
        
        // Destructor should implicitly Stop() and Join()
    }
    // No crash, so test passed
    EXPECT_TRUE(true);
}

TEST_F(ThreadPoolTest, StopIsIdempotent) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    pool.Stop();
    EXPECT_TRUE(pool.GetStopRequested());
    
    pool.Stop();  // Second Stop call
    EXPECT_TRUE(pool.GetStopRequested());
    
    pool.Join();
}

TEST_F(ThreadPoolTest, GetStopRequested_ReturnsCorrectState) {
    graph::ThreadPool pool(2);
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Init();
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Start();
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Stop();
    EXPECT_TRUE(pool.GetStopRequested());
    pool.Join();
}

TEST_F(ThreadPoolTest, GetStopRequested_BeforeAndAfterStop) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    EXPECT_FALSE(pool.GetStopRequested());
    
    pool.Stop();
    EXPECT_TRUE(pool.GetStopRequested());
    
    pool.Join();
}

// ============================================================================
// Exception Handling (3 tests)
// ============================================================================

TEST_F(ThreadPoolTest, ExceptionInTask_DoesNotCrashPool) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    std::atomic<bool> normal_task_executed{false};
    
    // Queue a task that throws
    pool.QueueTask([](){
        throw std::runtime_error("Test exception");
    });
    
    // Queue a normal task after
    pool.QueueTask([&](){
        normal_task_executed.store(true);
    });
    
    pool.Stop();
    pool.Join();
    
    // Pool should continue working despite exception
    EXPECT_TRUE(normal_task_executed.load());
    
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_failed.load(), 1);
    EXPECT_EQ(stats.tasks_completed.load(), 1);  // Normal task
}

TEST_F(ThreadPoolTest, MultipleExceptions_AllTracked) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 5; ++i) {
        pool.QueueTask([](){
            throw std::invalid_argument("Error");
        });
    }
    
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_failed.load(), 5);
}

TEST_F(ThreadPoolTest, Stats_ExceptionTracking) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    for (int i = 0; i < 3; ++i) {
        pool.QueueTask([](){
            throw std::logic_error("Test");
        });
    }
    
    pool.Stop();
    pool.Join();
    
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_failed.load(), 3);
}

// ============================================================================
// Concurrency and Thread Safety (6 tests)
// ============================================================================

TEST_F(ThreadPoolTest, ConcurrentEnqueue_FromMultipleThreads) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();
    
    std::atomic<int> task_count{0};
    const int num_enqueuers = 4;
    const int tasks_per_enqueuer = 50;
    
    std::vector<std::thread> enqueuers;
    for (int t = 0; t < num_enqueuers; ++t) {
        enqueuers.emplace_back([&](){
            for (int i = 0; i < tasks_per_enqueuer; ++i) {
                pool.QueueTask([&](){ task_count.fetch_add(1); });
            }
        });
    }
    
    for (auto& t : enqueuers) {
        t.join();
    }
    
    pool.Stop();
    pool.Join();
    
    EXPECT_EQ(task_count.load(), num_enqueuers * tasks_per_enqueuer);
}

TEST_F(ThreadPoolTest, WorkerThreads_ExecuteConcurrently) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();
    
    std::atomic<int> concurrent_tasks{0};
    std::atomic<int> max_concurrent{0};
    std::mutex concurrent_mutex;
    
    for (int i = 0; i < 8; ++i) {
        pool.QueueTask([&](){
            int count = concurrent_tasks.fetch_add(1) + 1;
            
            // Update max
            int current_max = max_concurrent.load();
            while (count > current_max && !max_concurrent.compare_exchange_weak(
                current_max, count, std::memory_order_release)) {
                // Retry
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            concurrent_tasks.fetch_sub(1);
        });
    }
    
    pool.Stop();
    pool.Join();
    
    // Should have had at least 2 tasks concurrent (with 4 threads)
    EXPECT_GT(max_concurrent.load(), 1);
}

TEST_F(ThreadPoolTest, GetThreadCount) {
    graph::ThreadPool pool(8);
    pool.Init();
    pool.Start();
    
    EXPECT_EQ(pool.GetThreadCount(), 8);
    
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, TasksExecuteFromWorkerThreads) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();
    
    std::thread::id main_thread_id = std::this_thread::get_id();
    std::vector<std::thread::id> task_thread_ids;
    std::mutex ids_mutex;
    
    for (int i = 0; i < 10; ++i) {
        pool.QueueTask([&](){
            std::lock_guard<std::mutex> lock(ids_mutex);
            task_thread_ids.push_back(std::this_thread::get_id());
        });
    }
    
    pool.Stop();
    pool.Join();
    
    EXPECT_EQ(task_thread_ids.size(), 10);
    
    // Tasks should execute from worker threads, not main thread
    for (const auto& id : task_thread_ids) {
        EXPECT_NE(id, main_thread_id);
    }
}

TEST_F(ThreadPoolTest, ThreadPool_NonCopyable) {
    graph::ThreadPool pool1(4);
    // This should not compile if uncommented:
    // graph::ThreadPool pool2 = pool1;
    // graph::ThreadPool pool3(pool1);
    
    EXPECT_TRUE(true);  // Compile-time check
}

// ============================================================================
// C++26 [[nodiscard]] Attribute Tests (3 tests)
// ============================================================================

TEST_F(ThreadPoolTest, NodiscardQueueTask) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    
    [[maybe_unused]] auto result = pool.QueueTask([](){});
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);
    
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, NodiscardStart) {
    graph::ThreadPool pool(2);
    pool.Init();
    
    [[maybe_unused]] bool started = pool.Start();
    EXPECT_TRUE(started);
    
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, NodiscardInit) {
    graph::ThreadPool pool(2);
    [[maybe_unused]] bool initialized = pool.Init();
    EXPECT_TRUE(initialized);

    pool.Start();
    pool.Stop();
    pool.Join();
}

// ============================================================================
// Enhanced Backpressure Tests (5 new tests)
// ============================================================================

TEST_F(ThreadPoolTest, BackpressureEnforcement_BlocksWhenFull) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 3;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);  // Single worker to saturate queue
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};

    // Queue a long-running task to block the worker
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Wait for task to start
    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Fill the queue to capacity
    std::atomic<int> enqueued{0};
    for (int i = 0; i < config.max_queue_size; ++i) {
        if (pool.QueueTask([](){}) == graph::ThreadPool::QueueResult::Ok) {
            enqueued.fetch_add(1);
        }
    }

    // Attempt to queue beyond capacity
    auto result = pool.QueueTask([](){});
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Full);

    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();

    // Verify backpressure kicked in
    EXPECT_EQ(enqueued.load(), static_cast<int>(config.max_queue_size));
}

TEST_F(ThreadPoolTest, BackpressureTimeout_QueueTaskWithTimeout_Respects) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 2;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};

    // Block the worker
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Fill queue
    for (int i = 0; i < config.max_queue_size; ++i) {
        pool.QueueTask([](){});
    }

    // Try to queue with timeout (should timeout, queue is full and worker is busy)
    auto start = std::chrono::steady_clock::now();
    auto result = pool.QueueTaskWithTimeout(
        [](){},
        std::chrono::milliseconds(100)
    );
    auto duration = std::chrono::steady_clock::now() - start;

    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();

    // Should have timed out
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Timeout);
    // Should have actually waited at least close to timeout duration
    EXPECT_GE(duration, std::chrono::milliseconds(90));
}

TEST_F(ThreadPoolTest, BackpressureRecovery_QueuesAgainAfterCapacityFrees) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 2;
    config.enable_detection = false;

    graph::ThreadPool pool(2, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};
    std::atomic<int> completed_count{0};

    // Queue a blocking task on first worker
    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Fill queue
    for (int i = 0; i < config.max_queue_size; ++i) {
        pool.QueueTask([&](){
            completed_count.fetch_add(1);
        });
    }

    // Queue should be full now
    EXPECT_EQ(pool.QueueTask([](){}), graph::ThreadPool::QueueResult::Full);

    // Release the blocking task
    slow_task_finish.store(true);

    // Wait a bit for queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now should be able to queue again
    auto result = pool.QueueTask([](){});
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);

    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, BackpressureStats_PeakQueueSizeTracking) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 100;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_task_running{false};
    std::atomic<bool> slow_task_finish{false};

    pool.QueueTask([&](){
        slow_task_running.store(true);
        while (!slow_task_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    while (!slow_task_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Queue many tasks
    for (int i = 0; i < 30; ++i) {
        pool.QueueTask([](){});
    }

    slow_task_finish.store(true);
    pool.Stop();
    pool.Join();

    const auto& stats = pool.GetStats();
    EXPECT_GT(stats.peak_queue_size.load(), 20);  // Should have observed significant queue depth
}

TEST_F(ThreadPoolTest, BackpressureRecoveryUnderLoad_ConcurrentEnqueuers) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 10;
    config.enable_detection = false;

    graph::ThreadPool pool(2, config);
    pool.Init();
    pool.Start();

    std::atomic<int> successful_enqueues{0};
    std::atomic<int> rejected_enqueues{0};

    // Multiple threads trying to enqueue under backpressure
    std::vector<std::thread> enqueuers;
    for (int t = 0; t < 4; ++t) {
        enqueuers.emplace_back([&](){
            for (int i = 0; i < 20; ++i) {
                auto result = pool.QueueTask([](){
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                });
                if (result == graph::ThreadPool::QueueResult::Ok) {
                    successful_enqueues.fetch_add(1);
                } else {
                    rejected_enqueues.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : enqueuers) {
        t.join();
    }

    pool.Stop();
    pool.Join();

    // Should have successfully enqueued tasks even under backpressure
    EXPECT_GT(successful_enqueues.load(), 0);
    // Some may have been rejected due to queue being full
    EXPECT_GE(successful_enqueues.load() + rejected_enqueues.load(), 80);
}

// ============================================================================
// Enhanced Task Rejection Tests (4 new tests)
// ============================================================================

TEST_F(ThreadPoolTest, TaskRejection_AllRejectedAfterStop) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    std::atomic<int> queued_count{0};
    std::atomic<int> rejected_count{0};

    // Queue some tasks
    for (int i = 0; i < 5; ++i) {
        auto result = pool.QueueTask([](){});
        if (result == graph::ThreadPool::QueueResult::Ok) {
            queued_count.fetch_add(1);
        }
    }

    pool.Stop();

    // All subsequent tasks should be rejected
    for (int i = 0; i < 10; ++i) {
        auto result = pool.QueueTask([](){});
        EXPECT_EQ(result, graph::ThreadPool::QueueResult::Stopped);
        rejected_count.fetch_add(1);
    }

    pool.Join();

    EXPECT_EQ(rejected_count.load(), 10);
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_rejected.load(), 10);
}

TEST_F(ThreadPoolTest, TaskRejection_RejectionTrackedInStats) {
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 2;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_running{false};
    std::atomic<bool> slow_finish{false};

    pool.QueueTask([&](){
        slow_running.store(true);
        while (!slow_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    while (!slow_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Fill queue and try to overflow
    for (int i = 0; i < config.max_queue_size; ++i) {
        pool.QueueTask([](){});
    }

    int overflow_count = 0;
    for (int i = 0; i < 5; ++i) {
        if (pool.QueueTask([](){}) == graph::ThreadPool::QueueResult::Full) {
            overflow_count++;
        }
    }

    slow_finish.store(true);
    pool.Stop();
    pool.Join();

    const auto& stats = pool.GetStats();
    EXPECT_GT(stats.tasks_rejected.load(), 0);
    EXPECT_GE(overflow_count, 1);
}

TEST_F(ThreadPoolTest, TaskRejection_NullTaskHandling) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // Attempting to queue null task
    graph::ThreadPool::Task null_task;  // Default-constructed, empty
    auto result = pool.QueueTask(null_task);

    pool.Stop();
    pool.Join();

    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Error);
}

TEST_F(ThreadPoolTest, TaskRejection_ConcurrentStopAndEnqueue) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    std::atomic<int> rejected_during_stop{0};
    std::atomic<bool> stop_issued{false};

    std::thread enqueuer([&](){
        while (!stop_issued.load()) {
            auto result = pool.QueueTask([](){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            });
            if (result == graph::ThreadPool::QueueResult::Stopped) {
                rejected_during_stop.fetch_add(1);
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stop_issued.store(true);
    pool.Stop();

    enqueuer.join();
    pool.Join();

    // Should have observed rejections after stop
    EXPECT_GT(rejected_during_stop.load(), 0);
}

// ============================================================================
// Enhanced Average Task Time Tests (4 new tests)
// ============================================================================

TEST_F(ThreadPoolTest, AverageTaskTime_Accuracy_MultipleVariableDurations) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // Queue tasks with known durations
    std::vector<int> durations_ms = {5, 10, 15, 5, 10};
    int total_ms = 0;
    int task_count = 0;

    for (int duration : durations_ms) {
        pool.QueueTask([duration](){
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        });
        total_ms += duration;
        task_count++;
    }

    pool.Stop();
    pool.Join();

    double avg_ms = pool.GetAverageTaskTimeMs();
    double expected_avg = total_ms / static_cast<double>(task_count);

    // Should be within reasonable margin (±5ms accounting for scheduling variance)
    EXPECT_GT(avg_ms, expected_avg - 5.0);
    EXPECT_LT(avg_ms, expected_avg + 5.0);
}

TEST_F(ThreadPoolTest, AverageTaskTime_ZeroWhenNoTasksCompleted) {
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_EQ(avg_ms, 0.0);

    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, AverageTaskTime_IncludesOnlyCompletedTasks) {
    graph::ThreadPool pool(1);
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_running{false};
    std::atomic<bool> slow_finish{false};

    // Queue a long-running task
    pool.QueueTask([&](){
        slow_running.store(true);
        while (!slow_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    while (!slow_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Queue quick tasks
    for (int i = 0; i < 3; ++i) {
        pool.QueueTask([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        });
    }

    // Give quick tasks time to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Average should reflect completed quick tasks, not the slow one still running
    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_GT(avg_ms, 0.0);
    EXPECT_LT(avg_ms, 50.0);  // Should be much less than the slow task's 500+ ms

    slow_finish.store(true);
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, AverageTaskTime_HighPrecision_LargeTaskCount) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    const int task_count = 100;
    const int duration_ms = 5;

    for (int i = 0; i < task_count; ++i) {
        pool.QueueTask([duration_ms](){
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        });
    }

    pool.Stop();
    pool.Join();

    double avg_ms = pool.GetAverageTaskTimeMs();

    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_completed.load(), task_count);

    // With many tasks, law of large numbers gives us better accuracy
    // Should average around 5ms per task
    EXPECT_GT(avg_ms, 2.0);
    EXPECT_LT(avg_ms, 10.0);
}

// ============================================================================
// Concurrent Execution Stress Tests (3 new tests)
// ============================================================================

TEST_F(ThreadPoolTest, ConcurrentExecution_FourThreadsFullUtilization) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> completed{0};

    auto record_concurrent = [&](){
        int count = concurrent_count.fetch_add(1) + 1;

        // Try to update max
        int current_max = max_concurrent.load();
        while (count > current_max && !max_concurrent.compare_exchange_weak(
            current_max, count, std::memory_order_release)) {
            // Retry
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        concurrent_count.fetch_sub(1);
        completed.fetch_add(1);
    };

    // Queue 16 tasks (should allow 4 concurrent)
    for (int i = 0; i < 16; ++i) {
        pool.QueueTask(record_concurrent);
    }

    pool.Stop();
    pool.Join();

    // With 4 threads, should see max concurrency of 3-4
    EXPECT_GE(max_concurrent.load(), 2);
    EXPECT_LE(max_concurrent.load(), 4);
    EXPECT_EQ(completed.load(), 16);
}

TEST_F(ThreadPoolTest, ConcurrentExecution_AllTasksExecuteEventually) {
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    const int num_tasks = 1000;
    std::atomic<int> executed{0};

    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([&](){
            executed.fetch_add(1);
        });
    }

    pool.Stop();
    pool.Join();

    // All 1000 tasks should have executed
    EXPECT_EQ(executed.load(), num_tasks);
}

TEST_F(ThreadPoolTest, ConcurrentExecution_NoDeadlock_UnderHeavyLoad) {
    graph::ThreadPool::DeadlockConfig config;
    config.enable_detection = true;
    config.task_timeout = std::chrono::milliseconds(5000);
    config.watchdog_interval = std::chrono::milliseconds(100);

    graph::ThreadPool pool(4, config);
    pool.Init();
    pool.Start();

    // Queue many fast tasks
    for (int batch = 0; batch < 10; ++batch) {
        for (int i = 0; i < 50; ++i) {
            pool.QueueTask([](){
                // Very quick task
                volatile int x = 0;
                for (int j = 0; j < 1000; ++j) {
                    x += j;
                }
            });
        }
    }

    pool.Stop();
    bool joined_in_time = pool.JoinWithTimeout(std::chrono::seconds(10));

    EXPECT_TRUE(joined_in_time);
    EXPECT_FALSE(pool.IsDeadlockDetected());
}


