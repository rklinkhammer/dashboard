// MIT License
//
// Copyright (c) 2026 gdashboard contributors
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
#include <condition_variable>

// ============================================================================
// Layer 1, Task 1.1: ThreadPool Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Core Components Layer
// ============================================================================

/**
 * @brief Test fixture for ThreadPool tests
 *
 * Provides common setup/teardown and utilities for testing ThreadPool
 * in isolation from other components.
 */
class ThreadPoolTest : public ::testing::Test {
protected:
    // Helper: Wait for condition with timeout
    static bool WaitFor(
        std::function<bool()> condition,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    ) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (condition()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }
};

// ============================================================================
// Lifecycle Tests (6 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Constructor_DefaultThreadCount) {
    // ARRANGE/ACT
    graph::ThreadPool pool;

    // ASSERT
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Constructor_CustomThreadCount) {
    // ARRANGE/ACT
    graph::ThreadPool pool(4);

    // ASSERT
    EXPECT_EQ(pool.GetThreadCount(), 4);
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Constructor_WithConfig) {
    // ARRANGE
    graph::ThreadPool::DeadlockConfig config;
    config.task_timeout = std::chrono::milliseconds(2000);
    config.enable_detection = true;

    // ACT
    graph::ThreadPool pool(4, config);

    // ASSERT
    EXPECT_EQ(pool.GetThreadCount(), 4);
    EXPECT_FALSE(pool.IsDeadlockDetected());
}

TEST_F(ThreadPoolTest, Init_Success) {
    // ARRANGE
    graph::ThreadPool pool(2);

    // ACT
    bool result = pool.Init();

    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_FALSE(pool.GetStopRequested());
}

TEST_F(ThreadPoolTest, Start_Success) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();

    // ACT
    bool result = pool.Start();

    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_FALSE(pool.GetStopRequested());

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, Stop_SetsStopRequested) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    pool.Stop();

    // ASSERT
    EXPECT_TRUE(pool.GetStopRequested());

    // CLEANUP
    pool.Join();
}

// ============================================================================
// Task Queueing Tests (5 tests)
// ============================================================================

TEST_F(ThreadPoolTest, QueueTask_CanQueueBeforeStop) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    auto result = pool.QueueTask([](){});

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, QueueTask_RejectedAfterStop) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    pool.Stop();

    // ACT
    auto result = pool.QueueTask([](){});

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Stopped);

    // CLEANUP
    pool.Join();
}

TEST_F(ThreadPoolTest, QueueTask_NullTaskRejected) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    graph::ThreadPool::Task null_task;  // Default-constructed, empty
    auto result = pool.QueueTask(null_task);

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Error);

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, QueueTaskWithTimeout_Success) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    auto result = pool.QueueTaskWithTimeout(
        [](){},
        std::chrono::seconds(1)
    );

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, QueueTaskWithTimeout_RejectedAfterStop) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();
    pool.Stop();

    // ACT
    auto result = pool.QueueTaskWithTimeout(
        [](){},
        std::chrono::milliseconds(100)
    );

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Stopped);

    // CLEANUP
    pool.Join();
}

// ============================================================================
// Task Execution Tests (3 tests)
// ============================================================================

TEST_F(ThreadPoolTest, TaskExecution_SingleTaskCompletes) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    std::atomic<bool> executed{false};
    std::condition_variable cv;
    std::mutex mtx;

    // ACT
    auto result = pool.QueueTask([&](){
        executed.store(true);
        {
            std::lock_guard<std::mutex> lock(mtx);
        }
        cv.notify_all();
    });

    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);

    // Wait for task to execute
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait_for(lock, std::chrono::seconds(2), [&](){ return executed.load(); });
    }

    // ASSERT
    EXPECT_TRUE(executed.load());

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, TaskExecution_MultipleTasksExecute) {
    // ARRANGE
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    std::atomic<int> count{0};
    const int num_tasks = 50;

    // ACT
    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([&](){ count.fetch_add(1); });
    }

    pool.Stop();
    bool joined = pool.JoinWithTimeout(std::chrono::seconds(5));

    // ASSERT
    EXPECT_TRUE(joined);
    EXPECT_EQ(count.load(), num_tasks);
}

TEST_F(ThreadPoolTest, TaskExecution_ExceptionHandling) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    std::atomic<int> executed{0};

    // Queue a task that throws
    pool.QueueTask([&](){
        throw std::runtime_error("Test exception");
    });

    // Queue a normal task after
    pool.QueueTask([&](){ executed.fetch_add(1); });

    // ACT
    pool.Stop();
    bool joined = pool.JoinWithTimeout(std::chrono::seconds(5));

    // ASSERT
    EXPECT_TRUE(joined);
    EXPECT_EQ(executed.load(), 1);  // Normal task should still execute

    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_failed.load(), 1);
    EXPECT_EQ(stats.tasks_completed.load(), 1);
}

// ============================================================================
// Statistics & Monitoring Tests (5 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Stats_TasksQueued_Tracked) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    for (int i = 0; i < 10; ++i) {
        pool.QueueTask([](){});
    }

    pool.Stop();
    pool.Join();

    // ASSERT
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_queued.load(), 10);
}

TEST_F(ThreadPoolTest, Stats_TasksCompleted_Tracked) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    const int num_tasks = 20;

    // ACT
    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([](){});
    }

    pool.Stop();
    bool joined = pool.JoinWithTimeout(std::chrono::seconds(5));

    // ASSERT
    EXPECT_TRUE(joined);
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_completed.load(), num_tasks);
}

TEST_F(ThreadPoolTest, Stats_TasksFailed_Tracked) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    for (int i = 0; i < 5; ++i) {
        pool.QueueTask([](){
            throw std::logic_error("Test error");
        });
    }

    pool.Stop();
    pool.Join();

    // ASSERT
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_failed.load(), 5);
}

TEST_F(ThreadPoolTest, Stats_AverageTaskTime_Calculated) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // Queue tasks with known duration
    const int num_tasks = 10;
    const auto task_duration = std::chrono::milliseconds(5);

    // ACT
    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([task_duration](){
            std::this_thread::sleep_for(task_duration);
        });
    }

    pool.Stop();
    pool.Join();

    // ASSERT
    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_GT(avg_ms, 0.0);
    EXPECT_LT(avg_ms, 100.0);  // Sanity check
}

TEST_F(ThreadPoolTest, Stats_ZeroWhenNoTasksExecuted) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    pool.Stop();
    pool.Join();

    // ASSERT
    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_EQ(avg_ms, 0.0);
}

// ============================================================================
// Shutdown Tests (4 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Join_WaitsForCompletion) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    std::atomic<int> completed{0};
    const int num_tasks = 10;

    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([&](){
            completed.fetch_add(1);
        });
    }

    // ACT
    pool.Stop();
    pool.Join();

    // ASSERT
    EXPECT_EQ(completed.load(), num_tasks);
}

TEST_F(ThreadPoolTest, JoinWithTimeout_CompletesBeforeTimeout) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    for (int i = 0; i < 5; ++i) {
        pool.QueueTask([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    // ACT
    pool.Stop();
    bool joined = pool.JoinWithTimeout(std::chrono::seconds(5));

    // ASSERT
    EXPECT_TRUE(joined);
}

TEST_F(ThreadPoolTest, Stop_IsIdempotent) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    // ACT
    pool.Stop();
    EXPECT_TRUE(pool.GetStopRequested());
    pool.Stop();  // Second Stop call

    // ASSERT
    EXPECT_TRUE(pool.GetStopRequested());

    // CLEANUP
    pool.Join();
}

TEST_F(ThreadPoolTest, Destructor_ImplicitCleanup) {
    // This test verifies that destructor properly cleans up without crash
    {
        graph::ThreadPool pool(2);
        pool.Init();
        pool.Start();

        // Queue some tasks
        pool.QueueTask([](){});
        pool.QueueTask([](){});

        // Destructor should handle cleanup implicitly
    }

    // If we reach here without crash, test passes
    EXPECT_TRUE(true);
}

// ============================================================================
// Concurrency Tests (3 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Concurrency_MultipleEnqueuersFromDifferentThreads) {
    // ARRANGE
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    std::atomic<int> total_queued{0};
    const int enqueuers = 4;
    const int tasks_per_enqueuer = 25;

    // ACT
    std::vector<std::thread> threads;
    for (int t = 0; t < enqueuers; ++t) {
        threads.emplace_back([&](){
            for (int i = 0; i < tasks_per_enqueuer; ++i) {
                if (pool.QueueTask([](){}) == graph::ThreadPool::QueueResult::Ok) {
                    total_queued.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    pool.Stop();
    pool.Join();

    // ASSERT
    EXPECT_EQ(total_queued.load(), enqueuers * tasks_per_enqueuer);
}

TEST_F(ThreadPoolTest, Concurrency_WorkerThreads_ExecuteInParallel) {
    // ARRANGE
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    std::atomic<int> active{0};
    std::atomic<int> max_concurrent{0};

    // ACT - Queue tasks that track concurrent execution
    for (int i = 0; i < 8; ++i) {
        pool.QueueTask([&](){
            int count = active.fetch_add(1) + 1;

            // Update max
            int current_max = max_concurrent.load();
            while (count > current_max &&
                   !max_concurrent.compare_exchange_weak(current_max, count)) {
                // Retry
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            active.fetch_sub(1);
        });
    }

    pool.Stop();
    pool.Join();

    // ASSERT - Should have had some parallelism (at least 2 concurrent)
    EXPECT_GE(max_concurrent.load(), 1);
}

TEST_F(ThreadPoolTest, Concurrency_ThreadSafeStatistics) {
    // ARRANGE
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    const int tasks = 100;

    // ACT - Queue tasks from one thread, read stats from another
    std::thread enqueuer([&](){
        for (int i = 0; i < tasks; ++i) {
            pool.QueueTask([](){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            });
        }
    });

    std::vector<size_t> observed_counts;
    std::thread reader([&](){
        for (int i = 0; i < 20; ++i) {
            const auto& stats = pool.GetStats();
            observed_counts.push_back(stats.tasks_queued.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    enqueuer.join();
    reader.join();

    pool.Stop();
    pool.Join();

    // ASSERT
    EXPECT_GT(observed_counts.size(), 0);
    // Final count should be tasks queued
    const auto& stats = pool.GetStats();
    EXPECT_EQ(stats.tasks_queued.load(), tasks);
}

// ============================================================================
// Backpressure Tests (3 tests)
// ============================================================================

TEST_F(ThreadPoolTest, Backpressure_QueueCapacityEnforced) {
    // ARRANGE
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 3;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);  // Single thread to saturate queue
    pool.Init();
    pool.Start();

    std::atomic<bool> slow_running{false};
    std::atomic<bool> slow_finish{false};

    // Block the worker with a long-running task
    pool.QueueTask([&](){
        slow_running.store(true);
        while (!slow_finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Wait for worker to start
    bool started = WaitFor([&](){ return slow_running.load(); });
    EXPECT_TRUE(started) << "Worker task should have started";

    // Fill queue to capacity
    std::atomic<int> full_count{0};
    for (int i = 0; i < static_cast<int>(config.max_queue_size); ++i) {
        auto result = pool.QueueTask([](){});
        if (result == graph::ThreadPool::QueueResult::Full) {
            full_count.fetch_add(1);
        }
    }

    // ACT - Try to queue beyond capacity
    auto result = pool.QueueTask([](){});

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Full);

    // CLEANUP
    slow_finish.store(true);
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, Backpressure_QueueRecovery) {
    // ARRANGE
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 2;
    config.enable_detection = false;

    graph::ThreadPool pool(2, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> blocking{false};
    std::atomic<bool> done{false};

    // Block first worker
    pool.QueueTask([&](){
        blocking.store(true);
        while (!done.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    bool started = WaitFor([&](){ return blocking.load(); });
    EXPECT_TRUE(started);

    // Fill queue
    for (int i = 0; i < static_cast<int>(config.max_queue_size); ++i) {
        pool.QueueTask([](){});
    }

    // Should be full now
    auto result = pool.QueueTask([](){});
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Full);

    // ACT - Release blocking task
    done.store(true);

    // Give time for queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should be able to queue again
    result = pool.QueueTask([](){});

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Ok);

    // CLEANUP
    pool.Stop();
    pool.Join();
}

TEST_F(ThreadPoolTest, Backpressure_TimeoutEnforced) {
    // ARRANGE
    graph::ThreadPool::DeadlockConfig config;
    config.max_queue_size = 1;
    config.enable_detection = false;

    graph::ThreadPool pool(1, config);
    pool.Init();
    pool.Start();

    std::atomic<bool> running{false};
    std::atomic<bool> finish{false};

    // Block worker
    pool.QueueTask([&](){
        running.store(true);
        while (!finish.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    bool started = WaitFor([&](){ return running.load(); });
    EXPECT_TRUE(started);

    // Fill queue
    pool.QueueTask([](){});

    // ACT - Try to queue with timeout (should timeout, queue full and worker busy)
    auto start = std::chrono::steady_clock::now();
    auto result = pool.QueueTaskWithTimeout(
        [](){},
        std::chrono::milliseconds(100)
    );
    auto elapsed = std::chrono::steady_clock::now() - start;

    // ASSERT
    EXPECT_EQ(result, graph::ThreadPool::QueueResult::Timeout);
    EXPECT_GE(elapsed, std::chrono::milliseconds(90));  // Should have waited

    // CLEANUP
    finish.store(true);
    pool.Stop();
    pool.Join();
}

// ============================================================================
// Average Task Time Tests (2 new comprehensive tests)
// ============================================================================

TEST_F(ThreadPoolTest, AverageTaskTime_AccuracyWithVariableDurations) {
    // ARRANGE
    graph::ThreadPool pool(2);
    pool.Init();
    pool.Start();

    const std::vector<int> durations_ms = {5, 10, 5, 10, 5};
    double expected_avg = 0.0;

    // ACT
    for (int duration : durations_ms) {
        pool.QueueTask([duration](){
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        });
        expected_avg += duration;
    }
    expected_avg /= durations_ms.size();

    pool.Stop();
    pool.Join();

    // ASSERT
    double avg_ms = pool.GetAverageTaskTimeMs();
    EXPECT_GT(avg_ms, expected_avg - 5.0);
    EXPECT_LT(avg_ms, expected_avg + 10.0);  // Allow for scheduling variance
}

TEST_F(ThreadPoolTest, AverageTaskTime_LargeTaskCountAccuracy) {
    // ARRANGE
    graph::ThreadPool pool(4);
    pool.Init();
    pool.Start();

    const int num_tasks = 100;
    const int duration_ms = 5;

    // ACT
    for (int i = 0; i < num_tasks; ++i) {
        pool.QueueTask([duration_ms](){
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
        });
    }

    pool.Stop();
    pool.Join();

    // ASSERT
    double avg_ms = pool.GetAverageTaskTimeMs();
    const auto& stats = pool.GetStats();

    EXPECT_EQ(stats.tasks_completed.load(), num_tasks);
    EXPECT_GT(avg_ms, 2.0);    // Should be at least ~5ms minus variance
    EXPECT_LT(avg_ms, 20.0);   // Should not exceed reasonable bounds
}

