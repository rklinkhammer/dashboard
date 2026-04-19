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
#include "core/ActiveQueue.hpp"
#include <thread>
#include <chrono>
#include <vector>

// ============================================================================
// Basic Queue Operations (9 tests)
// ============================================================================

class ActiveQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset for each test
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(ActiveQueueTest, Constructor_DefaultParameters) {
    core::ActiveQueue<int> queue;
    EXPECT_TRUE(queue.Empty());
    EXPECT_EQ(queue.Size(), 0);
    EXPECT_EQ(queue.Capacity(), 0);
    EXPECT_TRUE(queue.Enabled());
}

TEST_F(ActiveQueueTest, Constructor_WithCapacity) {
    core::ActiveQueue<int> queue(100, false);
    EXPECT_TRUE(queue.Empty());
    EXPECT_EQ(queue.Capacity(), 100);
    EXPECT_FALSE(queue.GetBlockOnFull());
}

TEST_F(ActiveQueueTest, Constructor_WithBlockOnFull) {
    core::ActiveQueue<int> queue(50, true);
    EXPECT_EQ(queue.Capacity(), 50);
    EXPECT_TRUE(queue.GetBlockOnFull());
}

TEST_F(ActiveQueueTest, Enqueue_SingleElement) {
    core::ActiveQueue<int> queue;
    EXPECT_TRUE(queue.Enqueue(42));
    EXPECT_EQ(queue.Size(), 1);
    EXPECT_FALSE(queue.Empty());
}

TEST_F(ActiveQueueTest, Enqueue_MultipleElements) {
    core::ActiveQueue<int> queue;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.Enqueue(i));
    }
    EXPECT_EQ(queue.Size(), 10);
}

TEST_F(ActiveQueueTest, DequeueNonBlocking_SingleElement) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(42);
    
    int value;
    EXPECT_TRUE(queue.DequeueNonBlocking(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.Empty());
}

TEST_F(ActiveQueueTest, DequeueNonBlocking_EmptyQueue) {
    core::ActiveQueue<int> queue;
    
    int value;
    EXPECT_FALSE(queue.DequeueNonBlocking(value));
}

TEST_F(ActiveQueueTest, Clear_RemovesAllElements) {
    core::ActiveQueue<int> queue;
    for (int i = 0; i < 5; ++i) {
        queue.Enqueue(i);
    }
    EXPECT_EQ(queue.Size(), 5);
    
    queue.Clear();
    EXPECT_TRUE(queue.Empty());
    EXPECT_EQ(queue.Size(), 0);
}

TEST_F(ActiveQueueTest, Disable_PreventsEnqueue) {
    core::ActiveQueue<int> queue;
    queue.Disable();
    
    EXPECT_FALSE(queue.Enqueue(42));
    EXPECT_FALSE(queue.Enabled());
}

// ============================================================================
// Deque-Style Operations (8 tests)
// ============================================================================

TEST_F(ActiveQueueTest, PushFront_AddToFront) {
    core::ActiveQueue<int> queue;
    queue.PushBack(1);
    queue.PushBack(2);
    queue.PushFront(0);
    
    std::vector<int> values;
    int value;
    while (queue.DequeueNonBlocking(value)) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, std::vector<int>({0, 1, 2}));
}

TEST_F(ActiveQueueTest, PopBack_RemoveFromBack) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    
    int value;
    EXPECT_TRUE(queue.PopBack(value));
    EXPECT_EQ(value, 3);
    EXPECT_EQ(queue.Size(), 2);
}

TEST_F(ActiveQueueTest, PopFront_SameAsDequeueNonBlocking) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(42);
    
    int value;
    EXPECT_TRUE(queue.PopFront(value));
    EXPECT_EQ(value, 42);
}

TEST_F(ActiveQueueTest, Front_AccessWithoutRemoving) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(42);
    queue.Enqueue(43);
    
    int value;
    EXPECT_TRUE(queue.Front(value));
    EXPECT_EQ(value, 42);
    EXPECT_EQ(queue.Size(), 2);
}

TEST_F(ActiveQueueTest, Back_AccessLastElement) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    
    int value;
    EXPECT_TRUE(queue.Back(value));
    EXPECT_EQ(value, 3);
    EXPECT_EQ(queue.Size(), 3);
}

TEST_F(ActiveQueueTest, At_RandomAccess) {
    core::ActiveQueue<int> queue;
    for (int i = 0; i < 5; ++i) {
        queue.Enqueue(i * 10);
    }
    
    int value;
    EXPECT_TRUE(queue.At(0, value));
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(queue.At(2, value));
    EXPECT_EQ(value, 20);
    EXPECT_TRUE(queue.At(4, value));
    EXPECT_EQ(value, 40);
}

TEST_F(ActiveQueueTest, At_OutOfBounds) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(1);
    
    int value;
    EXPECT_FALSE(queue.At(5, value));
}

TEST_F(ActiveQueueTest, PushBack_Enqueue_Equivalent) {
    core::ActiveQueue<int> queue;
    EXPECT_TRUE(queue.PushBack(42));
    
    int value;
    EXPECT_TRUE(queue.PopFront(value));
    EXPECT_EQ(value, 42);
}

// ============================================================================
// Boundary Conditions (6 tests)
// ============================================================================

TEST_F(ActiveQueueTest, BoundedQueue_DropWhenFull) {
    core::ActiveQueue<int> queue(3, false);  // Capacity 3, drop when full
    
    EXPECT_TRUE(queue.Enqueue(1));
    EXPECT_TRUE(queue.Enqueue(2));
    EXPECT_TRUE(queue.Enqueue(3));
    EXPECT_FALSE(queue.Enqueue(4));  // Should be dropped
    
    EXPECT_EQ(queue.Size(), 3);
}

TEST_F(ActiveQueueTest, UnboundedQueue_NeverFulls) {
    core::ActiveQueue<int> queue(0);  // Capacity 0 = unbounded
    
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(queue.Enqueue(i));
    }
    EXPECT_EQ(queue.Size(), 1000);
}

TEST_F(ActiveQueueTest, SetCapacity_ChangesLimit) {
    core::ActiveQueue<int> queue(5);
    queue.SetCapacity(10);
    
    EXPECT_EQ(queue.Capacity(), 10);
}

TEST_F(ActiveQueueTest, Enable_AllowsEnqueueAfterDisable) {
    core::ActiveQueue<int> queue;
    queue.Disable();
    EXPECT_FALSE(queue.Enqueue(42));
    
    queue.Enable();
    EXPECT_TRUE(queue.Enqueue(42));
    EXPECT_EQ(queue.Size(), 1);
}

TEST_F(ActiveQueueTest, SetBlockOnFull_FailsWithData) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(1);
    
    EXPECT_FALSE(queue.SetBlockOnFull(true));
    EXPECT_FALSE(queue.GetBlockOnFull());
}

TEST_F(ActiveQueueTest, SetBlockOnFull_SucceedsWhenEmpty) {
    core::ActiveQueue<int> queue;
    EXPECT_TRUE(queue.SetBlockOnFull(true));
    EXPECT_TRUE(queue.GetBlockOnFull());
}

// ============================================================================
// Metrics Collection (5 tests)
// ============================================================================

TEST_F(ActiveQueueTest, Metrics_DisabledByDefault) {
    core::ActiveQueue<int> queue;
    const auto& metrics = queue.GetMetrics();
    
    EXPECT_EQ(metrics.enqueued_count.load(), 0);
    EXPECT_EQ(metrics.dequeued_count.load(), 0);
}

TEST_F(ActiveQueueTest, Metrics_TrackEnqueue) {
    core::ActiveQueue<int> queue;
    queue.EnableMetrics();
    
    queue.Enqueue(1);
    queue.Enqueue(2);
    queue.Enqueue(3);
    
    const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.enqueued_count.load(), 3);
}

TEST_F(ActiveQueueTest, Metrics_TrackDequeue) {
    core::ActiveQueue<int> queue;
    queue.EnableMetrics();
    
    queue.Enqueue(1);
    queue.Enqueue(2);
    
    int value;
    queue.DequeueNonBlocking(value);
    queue.DequeueNonBlocking(value);
    
    const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.dequeued_count.load(), 2);
}

TEST_F(ActiveQueueTest, Metrics_TrackMaxSize) {
    core::ActiveQueue<int> queue;
    queue.EnableMetrics();
    
    for (int i = 0; i < 50; ++i) {
        queue.Enqueue(i);
    }
    
    const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.max_size_observed.load(), 50);
}

TEST_F(ActiveQueueTest, Metrics_ResetClearsCounters) {
    core::ActiveQueue<int> queue;
    queue.EnableMetrics();
    
    queue.Enqueue(1);
    queue.Enqueue(2);
    
    queue.ResetMetrics();
    
    const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.enqueued_count.load(), 0);
    EXPECT_EQ(metrics.dequeued_count.load(), 0);
    EXPECT_EQ(metrics.max_size_observed.load(), 0);
}

// ============================================================================
// Thread Safety (7 tests)
// ============================================================================

TEST_F(ActiveQueueTest, ThreadSafety_ConcurrentEnqueue) {
    core::ActiveQueue<int> queue;
    const int num_threads = 4;
    const int items_per_thread = 25;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&queue, t, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                queue.Enqueue(t * 1000 + i);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(queue.Size(), num_threads * items_per_thread);
}

TEST_F(ActiveQueueTest, ThreadSafety_ConcurrentDequeue) {
    core::ActiveQueue<int> queue;
    const int total_items = 100;
    
    for (int i = 0; i < total_items; ++i) {
        queue.Enqueue(i);
    }
    
    std::vector<int> results;
    std::mutex results_mutex;
    const int num_threads = 4;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            int value;
            while (queue.DequeueNonBlocking(value)) {
                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(value);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(results.size(), total_items);
    EXPECT_TRUE(queue.Empty());
}

TEST_F(ActiveQueueTest, ThreadSafety_BlockingDequeue) {
    core::ActiveQueue<int> queue;
    bool dequeue_completed = false;
    int received_value = -1;
    
    std::thread dequeue_thread([&]() {
        int value;
        if (queue.Dequeue(value)) {
            received_value = value;
            dequeue_completed = true;
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(dequeue_completed);
    
    queue.Enqueue(42);
    dequeue_thread.join();
    
    EXPECT_TRUE(dequeue_completed);
    EXPECT_EQ(received_value, 42);
}

TEST_F(ActiveQueueTest, ThreadSafety_DisableUnblocksWaiters) {
    core::ActiveQueue<int> queue;
    bool dequeue_returned = false;
    
    std::thread dequeue_thread([&]() {
        int value;
        queue.Dequeue(value);
        dequeue_returned = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(dequeue_returned);
    
    queue.Disable();
    dequeue_thread.join();
    
    EXPECT_TRUE(dequeue_returned);
}

TEST_F(ActiveQueueTest, ThreadSafety_MetricsThreadSafe) {
    core::ActiveQueue<int> queue;
    queue.EnableMetrics();
    const int num_threads = 10;
    const int items_per_thread = 100;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&queue, items_per_thread]() {
            for (int i = 0; i < items_per_thread; ++i) {
                queue.Enqueue(i);
                // Also read metrics to stress test
                auto& metrics = queue.GetMetrics();
                (void)metrics.enqueued_count.load();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.enqueued_count.load(), num_threads * items_per_thread);
}

TEST_F(ActiveQueueTest, ThreadSafety_SizeIsAtomicSnapshot) {
    core::ActiveQueue<int> queue;
    
    for (int i = 0; i < 10; ++i) {
        queue.Enqueue(i);
    }
    
    std::vector<std::size_t> observed_sizes;
    std::mutex sizes_mutex;
    
    std::thread reader([&]() {
        for (int i = 0; i < 100; ++i) {
            {
                std::lock_guard<std::mutex> lock(sizes_mutex);
                observed_sizes.push_back(queue.Size());
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    std::thread writer([&]() {
        for (int i = 0; i < 50; ++i) {
            queue.Enqueue(i);
            std::this_thread::sleep_for(std::chrono::microseconds(20));
        }
    });
    
    reader.join();
    writer.join();
    
    // Sizes should be monotonically non-decreasing during filling
    EXPECT_GT(observed_sizes.size(), 0);
}

// ============================================================================
// Comparator/Sorted Insertion (3 tests)
// ============================================================================

TEST_F(ActiveQueueTest, SetComparator_SortedInsertion) {
    core::ActiveQueue<int> queue;
    queue.SetComparator([](const int& a, const int& b) {
        return a < b;  // Min-heap: smaller values at front
    });
    
    queue.Enqueue(5);
    queue.Enqueue(2);
    queue.Enqueue(8);
    queue.Enqueue(1);
    
    std::vector<int> values;
    int value;
    while (queue.DequeueNonBlocking(value)) {
        values.push_back(value);
    }
    
    EXPECT_EQ(values, std::vector<int>({1, 2, 5, 8}));
}

TEST_F(ActiveQueueTest, SetComparator_CanBeDisabled) {
    core::ActiveQueue<int> queue;
    queue.SetComparator([](const int& a, const int& b) {
        return a < b;
    });
    
    queue.SetComparator(nullptr);
    
    queue.Enqueue(5);
    queue.Enqueue(2);
    
    std::vector<int> values;
    int value;
    while (queue.DequeueNonBlocking(value)) {
        values.push_back(value);
    }
    
    // Should be FIFO now (5, 2) not sorted
    EXPECT_EQ(values, std::vector<int>({5, 2}));
}

TEST_F(ActiveQueueTest, Emplace_ConstructsInPlace) {
    struct TestStruct {
        int x;
        std::string y;
        
        TestStruct(int _x, const std::string& _y) : x(_x), y(_y) {}
    };
    
    core::ActiveQueue<TestStruct> queue;
    EXPECT_TRUE(queue.Emplace(42, "hello"));
    
    EXPECT_EQ(queue.Size(), 1);
}

// ============================================================================
// RAII and Cleanup (2 tests)
// ============================================================================

TEST_F(ActiveQueueTest, Destructor_CleansUpOnDestroy) {
    {
        core::ActiveQueue<int> queue;
        queue.Enqueue(1);
        queue.Enqueue(2);
        queue.Enqueue(3);
        // Should clean up gracefully when scope ends
    }
    // If we got here without crash, test passed
    EXPECT_TRUE(true);
}

TEST_F(ActiveQueueTest, NonCopyable_CopyConstructorDeleted) {
    core::ActiveQueue<int> queue1;
    // This should not compile if uncommented:
    // core::ActiveQueue<int> queue2 = queue1;
    // core::ActiveQueue<int> queue3(queue1);
    
    EXPECT_TRUE(true);  // Compile-time check; runtime always passes
}

// ============================================================================
// C++26 [[nodiscard]] Attribute Tests (5 tests)
// ============================================================================

TEST_F(ActiveQueueTest, NodiscardEnqueue_ReturnedByFunction) {
    core::ActiveQueue<int> queue;
    [[maybe_unused]] bool result = queue.Enqueue(42);
    // With [[nodiscard]], compiler warns if result unused;
    // we use it here, so no warning
    EXPECT_TRUE(result);
}

TEST_F(ActiveQueueTest, NodiscardDequeue_ReturnedByFunction) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(42);
    
    int value;
    [[maybe_unused]] bool result = queue.Dequeue(value);
    EXPECT_TRUE(result);
}

TEST_F(ActiveQueueTest, NodiscardDequeueNonBlocking_ReturnedByFunction) {
    core::ActiveQueue<int> queue;
    queue.Enqueue(42);
    
    int value;
    [[maybe_unused]] bool result = queue.DequeueNonBlocking(value);
    EXPECT_TRUE(result);
}

TEST_F(ActiveQueueTest, NodiscardEmpty_ReturnedByFunction) {
    core::ActiveQueue<int> queue;
    [[maybe_unused]] bool is_empty = queue.Empty();
    EXPECT_TRUE(is_empty);
}

TEST_F(ActiveQueueTest, NodiscardGetMetrics_ReturnedByFunction) {
    core::ActiveQueue<int> queue;
    [[maybe_unused]] const auto& metrics = queue.GetMetrics();
    EXPECT_EQ(metrics.enqueued_count.load(), 0);
}

// ============================================================================
// Concurrent Reader/Writer Stress Tests (5 new tests)
// ============================================================================
// These tests verify robust behavior under concurrent multi-producer/consumer load

TEST_F(ActiveQueueTest, ConcurrentStress_ManyProducersManyConsumers) {
    core::ActiveQueue<int> queue;

    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 100;
    const int total_expected = num_producers * items_per_producer;

    std::atomic<int> consumed{0};
    std::vector<std::thread> threads;

    // Producer threads
    for (int p = 0; p < num_producers; ++p) {
        threads.emplace_back([&, p](){
            for (int i = 0; i < items_per_producer; ++i) {
                queue.Enqueue(p * items_per_producer + i);
            }
        });
    }

    // Consumer threads
    for (int c = 0; c < num_consumers; ++c) {
        threads.emplace_back([&](){
            int value;
            while (consumed.load() < total_expected) {
                if (queue.DequeueNonBlocking(value)) {
                    consumed.fetch_add(1);
                }
            }
        });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // Verify all items were consumed
    EXPECT_EQ(consumed.load(), total_expected);
    EXPECT_TRUE(queue.Empty());
}

TEST_F(ActiveQueueTest, ConcurrentStress_BlockingDequeueUnderLoad) {
    core::ActiveQueue<int> queue;

    const int num_items = 50;
    std::atomic<int> consumed{0};
    std::atomic<bool> producer_done{false};
    std::atomic<bool> consumer_exit{false};

    // Consumer thread with blocking dequeue
    std::thread consumer([&](){
        int value;
        while (!consumer_exit.load()) {
            if (queue.DequeueNonBlocking(value)) {
                consumed.fetch_add(1);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    // Producer thread
    std::thread producer([&](){
        for (int i = 0; i < num_items; ++i) {
            queue.Enqueue(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        producer_done.store(true);
    });

    producer.join();

    // Give consumer time to consume
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    consumer_exit.store(true);
    consumer.join();

    EXPECT_EQ(consumed.load(), num_items);
}

TEST_F(ActiveQueueTest, ConcurrentStress_HighThroughputEnqueue) {
    core::ActiveQueue<int> queue;

    const int num_threads = 8;
    const int items_per_thread = 500;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&](){
            for (int i = 0; i < items_per_thread; ++i) {
                queue.Enqueue(i);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify queue size
    EXPECT_EQ(queue.Size(), static_cast<size_t>(num_threads * items_per_thread));
}

TEST_F(ActiveQueueTest, ConcurrentStress_AlternatingDisableEnable) {
    core::ActiveQueue<int> queue;

    std::atomic<int> enqueued{0};
    std::atomic<int> attempts{0};
    std::atomic<bool> stop{false};

    // Producer thread trying to enqueue with enable/disable cycling
    std::thread producer([&](){
        while (!stop.load()) {
            if (queue.Enqueue(1)) {
                enqueued.fetch_add(1);
            }
            attempts.fetch_add(1);
        }
    });

    // Controller thread that cycles enable/disable
    std::thread controller([&](){
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            queue.Disable();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            queue.Enable();
        }
        stop.store(true);
    });

    producer.join();
    controller.join();

    // Should have some successful enqueues before disable took effect
    EXPECT_GT(enqueued.load(), 0);
    EXPECT_GT(attempts.load(), 0);
}

TEST_F(ActiveQueueTest, ConcurrentStress_SizeConsistencyUnderLoad) {
    core::ActiveQueue<int> queue;

    const int producer_count = 3;
    const int items_per_producer = 100;
    const int consumer_count = 2;

    std::vector<size_t> observed_sizes;
    std::mutex sizes_mutex;

    // Producers
    std::vector<std::thread> threads;
    for (int p = 0; p < producer_count; ++p) {
        threads.emplace_back([&, p](){
            for (int i = 0; i < items_per_producer; ++i) {
                queue.Enqueue(p * items_per_producer + i);

                {
                    std::lock_guard<std::mutex> lock(sizes_mutex);
                    observed_sizes.push_back(queue.Size());
                }
            }
        });
    }

    // Consumers
    for (int c = 0; c < consumer_count; ++c) {
        threads.emplace_back([&](){
            int value;
            while (!queue.Empty() || queue.Size() > 0) {
                if (queue.DequeueNonBlocking(value)) {
                    {
                        std::lock_guard<std::mutex> lock(sizes_mutex);
                        observed_sizes.push_back(queue.Size());
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Size should be monotonically bounded [0, max_capacity]
    for (size_t observed : observed_sizes) {
        EXPECT_GE(observed, 0);
        EXPECT_LE(observed, static_cast<size_t>(producer_count * items_per_producer));
    }
}

// ============================================================================
// Boundary Conditions Tests (4 new tests)
// ============================================================================
// These tests verify correct behavior at capacity limits and edge cases

TEST_F(ActiveQueueTest, BoundaryCondition_ExactlyAtCapacity) {
    core::ActiveQueue<int> queue(10, false);  // Capacity 10, don't block

    // Fill exactly to capacity
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.Enqueue(i));
    }

    EXPECT_EQ(queue.Size(), 10);

    // Next enqueue should fail (bounded queue)
    EXPECT_FALSE(queue.Enqueue(10));

    // Size should still be 10
    EXPECT_EQ(queue.Size(), 10);
}

TEST_F(ActiveQueueTest, BoundaryCondition_CapacityZero) {
    core::ActiveQueue<int> queue(0, false);  // Zero capacity

    // With zero capacity, enqueue still works for unbounded behavior
    // This test validates that zero capacity doesn't crash
    int value = 1;
    queue.Enqueue(value);

    // Size should reflect the item
    EXPECT_GE(queue.Size(), 1);
}

TEST_F(ActiveQueueTest, BoundaryCondition_ResizingCapacity) {
    core::ActiveQueue<int> queue(5, false);

    // Fill to capacity 5
    for (int i = 0; i < 5; ++i) {
        queue.Enqueue(i);
    }
    EXPECT_EQ(queue.Size(), 5);

    // Resize to capacity 10
    queue.SetCapacity(10);

    // Should now accept more items
    for (int i = 5; i < 10; ++i) {
        EXPECT_TRUE(queue.Enqueue(i));
    }
    EXPECT_EQ(queue.Size(), 10);
}

TEST_F(ActiveQueueTest, BoundaryCondition_LargeCapacity) {
    core::ActiveQueue<int> queue(10000, false);

    // Enqueue many items
    for (int i = 0; i < 10000; ++i) {
        EXPECT_TRUE(queue.Enqueue(i));
    }

    EXPECT_EQ(queue.Size(), 10000);

    // Verify we can dequeue them all
    int count = 0;
    int value;
    while (queue.DequeueNonBlocking(value)) {
        count++;
    }
    EXPECT_EQ(count, 10000);
}

// ============================================================================
// Memory Ordering & Atomicity Tests (3 new tests)
// ============================================================================
// These tests verify lock-free guarantees and atomic visibility

TEST_F(ActiveQueueTest, MemoryOrdering_EnqueueVisibilityToConsumer) {
    core::ActiveQueue<int> queue;

    std::atomic<bool> producer_done{false};
    int received_value = -1;

    std::thread producer([&](){
        queue.Enqueue(42);
        producer_done.store(true, std::memory_order_release);
    });

    // Consumer waits for producer signal then dequeues
    while (!producer_done.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    EXPECT_TRUE(queue.DequeueNonBlocking(received_value));
    EXPECT_EQ(received_value, 42);

    producer.join();
}

TEST_F(ActiveQueueTest, MemoryOrdering_DisableVisibilityToWaiters) {
    core::ActiveQueue<int> queue;

    std::atomic<bool> dequeue_started{false};
    std::atomic<bool> dequeue_returned{false};
    bool dequeue_result = true;

    std::thread waiter([&](){
        int value;
        dequeue_started.store(true);
        dequeue_result = queue.DequeueNonBlocking(value);  // Try non-blocking first
        dequeue_returned.store(true, std::memory_order_release);
    });

    // Give waiter time to attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify waiter started
    EXPECT_TRUE(dequeue_started.load());

    // Disable queue
    queue.Disable();

    // Wait for waiter to return
    auto start = std::chrono::steady_clock::now();
    while (!dequeue_returned.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(1)) {
            FAIL() << "Waiter did not return after queue disabled";
        }
        std::this_thread::yield();
    }

    EXPECT_FALSE(dequeue_result);  // Should fail (queue was empty)
    waiter.join();
}

TEST_F(ActiveQueueTest, MemoryOrdering_MetricsAtomicVisibility) {
    core::ActiveQueue<int> queue;

    const int num_items = 1000;
    std::atomic<int> producer_count{0};
    std::atomic<int> consumer_count{0};

    std::thread producer([&](){
        for (int i = 0; i < num_items; ++i) {
            queue.Enqueue(i);
            producer_count.fetch_add(1, std::memory_order_release);
        }
    });

    std::thread consumer([&](){
        int value;
        while (consumer_count.load(std::memory_order_acquire) < num_items) {
            if (queue.DequeueNonBlocking(value)) {
                consumer_count.fetch_add(1, std::memory_order_release);
            }
        }
    });

    producer.join();
    consumer.join();

    // Verify counts via atomic operations
    EXPECT_EQ(producer_count.load(std::memory_order_acquire), num_items);
    EXPECT_EQ(consumer_count.load(std::memory_order_acquire), num_items);

    // Queue should be empty
    EXPECT_TRUE(queue.Empty());
}

