/**
 * @file test_message_pool.cpp
 * @brief Comprehensive tests for MessageBufferPool
 *
 * Tests cover:
 * - Basic acquire/release operations
 * - Capacity enforcement and overflow handling
 * - Statistics accuracy under various workloads
 * - Thread safety with concurrent access
 * - Pool lifecycle (initialize/shutdown)
 * - Hit rate calculations
 */

#include <gtest/gtest.h>
#include <graph/MessagePool.hpp>
#include <thread>
#include <vector>
#include <cstring>

using namespace graph;

// ============================================================================
// Test Fixtures
// ============================================================================

/**
 * Basic pool tests with default 64-byte buffers
 */
class MessageBufferPoolTest : public ::testing::Test {
protected:
    MessageBufferPool<64> pool;

    void SetUp() override {
        // Pool starts uninitialized
        EXPECT_FALSE(pool.IsInitialized());
    }

    void TearDown() override {
        // Cleanup if needed
        if (pool.IsInitialized()) {
            pool.Shutdown();
        }
    }
};

/**
 * Tests with custom buffer sizes
 */
class MessageBufferPoolCustomSizeTest : public ::testing::Test {
};

/**
 * Thread safety tests
 */
class MessageBufferPoolThreadSafetyTest : public ::testing::Test {
};

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, InitializeAllocatesBuffers) {
    size_t capacity = 10;
    pool.Initialize(capacity);

    EXPECT_TRUE(pool.IsInitialized());
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, capacity);
    EXPECT_EQ(stats.current_available, capacity);
    EXPECT_EQ(stats.peak_capacity, capacity);
}

TEST_F(MessageBufferPoolTest, InitializeThrowsOnZeroCapacity) {
    EXPECT_THROW(pool.Initialize(0), std::invalid_argument);
}

TEST_F(MessageBufferPoolTest, InitializeThrowsIfAlreadyInitialized) {
    pool.Initialize(10);
    EXPECT_THROW(pool.Initialize(5), std::logic_error);
}

TEST_F(MessageBufferPoolTest, ShutdownFreesAllBuffers) {
    pool.Initialize(10);
    EXPECT_TRUE(pool.IsInitialized());

    pool.Shutdown();
    EXPECT_FALSE(pool.IsInitialized());
}

TEST_F(MessageBufferPoolTest, ShutdownCanBeCalledMultipleTimes) {
    pool.Initialize(10);
    pool.Shutdown();

    // Should not throw or crash
    pool.Shutdown();
    pool.Shutdown();
}

TEST_F(MessageBufferPoolTest, ShutdownOnUnInitializedPoolIsNoop) {
    // Should not throw
    EXPECT_NO_THROW(pool.Shutdown());
}

// ============================================================================
// Basic Acquire/Release Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, AcquireBufferThrowsIfNotInitialized) {
    EXPECT_THROW(pool.AcquireBuffer(), std::logic_error);
}

TEST_F(MessageBufferPoolTest, AcquireReturnsValidPointer) {
    pool.Initialize(10);

    void* buffer = pool.AcquireBuffer();
    EXPECT_NE(buffer, nullptr);
}

TEST_F(MessageBufferPoolTest, AcquireReducesAvailableCount) {
    pool.Initialize(10);
    auto stats1 = pool.GetStats();
    EXPECT_EQ(stats1.current_available, 10);

    void* buffer = pool.AcquireBuffer();
    auto stats2 = pool.GetStats();
    EXPECT_EQ(stats2.current_available, 9);
}

TEST_F(MessageBufferPoolTest, ReleaseIncreaseAvailableCount) {
    pool.Initialize(10);

    void* buffer = pool.AcquireBuffer();
    auto stats1 = pool.GetStats();
    EXPECT_EQ(stats1.current_available, 9);

    pool.ReleaseBuffer(buffer);
    auto stats2 = pool.GetStats();
    EXPECT_EQ(stats2.current_available, 10);
}

TEST_F(MessageBufferPoolTest, ReleaseNullPointerIsNoop) {
    pool.Initialize(10);

    auto stats1 = pool.GetStats();
    pool.ReleaseBuffer(nullptr);
    auto stats2 = pool.GetStats();

    // Should be unchanged
    EXPECT_EQ(stats1.current_available, stats2.current_available);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, StatisticsTrackRequests) {
    pool.Initialize(10);

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 0);

    pool.AcquireBuffer();
    stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 1);

    pool.AcquireBuffer();
    stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 2);
}

TEST_F(MessageBufferPoolTest, StatisticsTrackReuses) {
    pool.Initialize(10);

    // First acquire uses pooled buffer (reuse)
    void* buffer1 = pool.AcquireBuffer();
    auto stats1 = pool.GetStats();
    EXPECT_EQ(stats1.total_reuses, 1);

    pool.ReleaseBuffer(buffer1);

    // Second acquire reuses the buffer
    void* buffer2 = pool.AcquireBuffer();
    auto stats2 = pool.GetStats();
    EXPECT_EQ(stats2.total_reuses, 2);
}

TEST_F(MessageBufferPoolTest, HitRateCalculation) {
    pool.Initialize(2);

    // Requests 0, reuses 0 → 0%
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.GetHitRatePercent(), 0.0);

    // Acquire 2 buffers (2 requests, 2 reuses from pool)
    void* buf1 = pool.AcquireBuffer();
    void* buf2 = pool.AcquireBuffer();

    stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 2);
    EXPECT_EQ(stats.total_reuses, 2);
    EXPECT_EQ(stats.GetHitRatePercent(), 100.0);

    // Release both
    pool.ReleaseBuffer(buf1);
    pool.ReleaseBuffer(buf2);

    // Acquire again (2 more reuses)
    void* buf3 = pool.AcquireBuffer();
    void* buf4 = pool.AcquireBuffer();

    stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 4);
    EXPECT_EQ(stats.total_reuses, 4);
    EXPECT_EQ(stats.GetHitRatePercent(), 100.0);
}

TEST_F(MessageBufferPoolTest, AverageReusesPerBuffer) {
    pool.Initialize(2);

    // No allocations yet
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.GetAverageReusesPerBuffer(), 0.0);

    // Acquire and reuse 4 times total (4 reuses / 2 allocations = 2.0 avg)
    void* buf1 = pool.AcquireBuffer();
    void* buf2 = pool.AcquireBuffer();
    pool.ReleaseBuffer(buf1);
    void* buf3 = pool.AcquireBuffer();
    pool.ReleaseBuffer(buf2);
    void* buf4 = pool.AcquireBuffer();

    stats = pool.GetStats();
    EXPECT_EQ(stats.total_reuses, 4);  // All 4 requests got reused buffers from initial pool
    EXPECT_EQ(stats.total_allocations, 2);  // Original 2 only
    EXPECT_EQ(stats.GetAverageReusesPerBuffer(), 2.0);
}

TEST_F(MessageBufferPoolTest, PeakCapacityTracking) {
    pool.Initialize(5);

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.peak_capacity, 5);

    // Acquire 3, release all (peak stays at 5)
    std::vector<void*> buffers;
    for (int i = 0; i < 3; i++) {
        buffers.push_back(pool.AcquireBuffer());
    }
    for (auto buf : buffers) {
        pool.ReleaseBuffer(buf);
    }

    stats = pool.GetStats();
    EXPECT_EQ(stats.peak_capacity, 5);
}

// ============================================================================
// Capacity and Overflow Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, PoolEmptyAllocatesNew) {
    pool.Initialize(2);

    // Acquire 2 from pool
    void* buf1 = pool.AcquireBuffer();
    void* buf2 = pool.AcquireBuffer();

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 2);

    // Pool is empty, third acquire triggers new malloc
    void* buf3 = pool.AcquireBuffer();
    EXPECT_NE(buf3, nullptr);

    stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 3);
}

TEST_F(MessageBufferPoolTest, AllocationCountIncrementsOnOverflow) {
    pool.Initialize(1);

    void* buf1 = pool.AcquireBuffer();
    void* buf2 = pool.AcquireBuffer();  // Overflow - malloc
    void* buf3 = pool.AcquireBuffer();  // Another malloc

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 3);
    EXPECT_EQ(stats.total_requests, 3);
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, BufferDataIntegrity) {
    MessageBufferPool<32> small_pool;
    small_pool.Initialize(3);

    // Write pattern to buffer
    void* buffer = small_pool.AcquireBuffer();
    std::memset(buffer, 0xAB, 32);

    // Verify data
    uint8_t* data = static_cast<uint8_t*>(buffer);
    for (int i = 0; i < 32; i++) {
        EXPECT_EQ(data[i], 0xAB);
    }

    small_pool.ReleaseBuffer(buffer);
    small_pool.Shutdown();
}

TEST_F(MessageBufferPoolTest, MultipleBuffersIndependent) {
    pool.Initialize(3);

    void* buf1 = pool.AcquireBuffer();
    void* buf2 = pool.AcquireBuffer();
    void* buf3 = pool.AcquireBuffer();

    // Write different patterns
    std::memset(buf1, 0x11, 64);
    std::memset(buf2, 0x22, 64);
    std::memset(buf3, 0x33, 64);

    // Verify each maintains its pattern
    EXPECT_EQ(static_cast<uint8_t*>(buf1)[0], 0x11);
    EXPECT_EQ(static_cast<uint8_t*>(buf2)[0], 0x22);
    EXPECT_EQ(static_cast<uint8_t*>(buf3)[0], 0x33);

    pool.ReleaseBuffer(buf1);
    pool.ReleaseBuffer(buf2);
    pool.ReleaseBuffer(buf3);
}

// ============================================================================
// Custom Buffer Size Tests
// ============================================================================

TEST_F(MessageBufferPoolCustomSizeTest, SmallBufferSize) {
    MessageBufferPool<16> small_pool;
    small_pool.Initialize(5);

    void* buffer = small_pool.AcquireBuffer();
    EXPECT_NE(buffer, nullptr);

    small_pool.ReleaseBuffer(buffer);
    small_pool.Shutdown();
}

TEST_F(MessageBufferPoolCustomSizeTest, LargeBufferSize) {
    MessageBufferPool<256> large_pool;
    large_pool.Initialize(3);

    void* buffer = large_pool.AcquireBuffer();
    EXPECT_NE(buffer, nullptr);

    large_pool.ReleaseBuffer(buffer);
    large_pool.Shutdown();
}

TEST_F(MessageBufferPoolCustomSizeTest, GetBufferSizeConstexpr) {
    static_assert(MessageBufferPool<64>::GetBufferSize() == 64);
    static_assert(MessageBufferPool<128>::GetBufferSize() == 128);
    static_assert(MessageBufferPool<32>::GetBufferSize() == 32);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(MessageBufferPoolThreadSafetyTest, ConcurrentAcquireRelease) {
    MessageBufferPool<64> pool;
    pool.Initialize(10);

    std::vector<std::thread> threads;
    std::vector<void*> acquired_buffers;
    std::mutex buffer_mutex;

    // Launch 4 threads that acquire and release buffers
    auto worker = [&pool, &acquired_buffers, &buffer_mutex](int thread_id) {
        for (int i = 0; i < 10; i++) {
            void* buffer = pool.AcquireBuffer();

            // Short work simulation
            std::memset(buffer, thread_id, 64);

            {
                std::lock_guard<std::mutex> lock(buffer_mutex);
                acquired_buffers.push_back(buffer);
            }

            pool.ReleaseBuffer(buffer);
        }
    };

    for (int i = 0; i < 4; i++) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify we got all buffers
    EXPECT_EQ(acquired_buffers.size(), 40);

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 40);

    pool.Shutdown();
}

TEST_F(MessageBufferPoolThreadSafetyTest, ProducerConsumerPattern) {
    MessageBufferPool<64> pool;
    pool.Initialize(5);

    std::vector<void*> active_buffers;
    std::mutex active_mutex;
    volatile bool stop_producers = false;

    auto producer = [&pool, &active_buffers, &active_mutex, &stop_producers]() {
        for (int i = 0; i < 20 && !stop_producers; i++) {
            void* buffer = pool.AcquireBuffer();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_buffers.push_back(buffer);
            }
            std::this_thread::yield();
        }
    };

    auto consumer = [&pool, &active_buffers, &active_mutex]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        for (int i = 0; i < 20; i++) {
            void* buffer = nullptr;
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                if (!active_buffers.empty()) {
                    buffer = active_buffers.back();
                    active_buffers.pop_back();
                }
            }
            if (buffer) {
                pool.ReleaseBuffer(buffer);
            }
            std::this_thread::yield();
        }
    };

    std::thread prod(producer);
    std::thread cons(consumer);

    prod.join();
    stop_producers = true;
    cons.join();

    auto stats = pool.GetStats();
    EXPECT_GT(stats.total_requests, 0);

    pool.Shutdown();
}

TEST_F(MessageBufferPoolThreadSafetyTest, RaceConditionDetection) {
    MessageBufferPool<64> pool;
    pool.Initialize(3);

    std::vector<void*> buffers;
    std::mutex buffers_mutex;

    // Rapid acquire/release from multiple threads
    auto stress_worker = [&pool, &buffers, &buffers_mutex]() {
        for (int i = 0; i < 100; i++) {
            void* buf = pool.AcquireBuffer();
            {
                std::lock_guard<std::mutex> lock(buffers_mutex);
                buffers.push_back(buf);
            }

            {
                std::lock_guard<std::mutex> lock(buffers_mutex);
                if (!buffers.empty()) {
                    void* released = buffers.back();
                    buffers.pop_back();
                    pool.ReleaseBuffer(released);
                }
            }
        }
    };

    std::vector<std::thread> workers;
    for (int i = 0; i < 8; i++) {
        workers.emplace_back(stress_worker);
    }

    for (auto& t : workers) {
        t.join();
    }

    // If we get here without crashes/hangs, thread safety works
    SUCCEED();

    pool.Shutdown();
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(MessageBufferPoolTest, HighFrequencyAcquireRelease) {
    pool.Initialize(10);

    for (int i = 0; i < 1000; i++) {
        void* buffer = pool.AcquireBuffer();
        EXPECT_NE(buffer, nullptr);
        pool.ReleaseBuffer(buffer);
    }

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 1000);
    EXPECT_GE(stats.total_reuses, 10);
    EXPECT_EQ(stats.current_available, 10);
}

TEST_F(MessageBufferPoolTest, ExhaustionAndRecovery) {
    pool.Initialize(5);

    std::vector<void*> buffers;

    // Exhaust pool
    for (int i = 0; i < 10; i++) {
        buffers.push_back(pool.AcquireBuffer());
    }

    auto stats1 = pool.GetStats();
    EXPECT_EQ(stats1.total_allocations, 10);
    EXPECT_EQ(stats1.current_available, 0);

    // Release all
    for (auto buf : buffers) {
        pool.ReleaseBuffer(buf);
    }

    auto stats2 = pool.GetStats();
    // Some buffers are back in pool
    EXPECT_GT(stats2.current_available, 0);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MessageBufferPoolTest, SingleBufferPool) {
    MessageBufferPool<64> single_pool;
    single_pool.Initialize(1);

    void* buf1 = single_pool.AcquireBuffer();
    void* buf2 = single_pool.AcquireBuffer();  // Overflow

    EXPECT_NE(buf1, buf2);

    single_pool.ReleaseBuffer(buf1);
    void* buf3 = single_pool.AcquireBuffer();  // Reuse buf1
    EXPECT_EQ(buf1, buf3);

    single_pool.ReleaseBuffer(buf2);
    single_pool.ReleaseBuffer(buf3);
    single_pool.Shutdown();
}

TEST_F(MessageBufferPoolTest, LargeCapacityPool) {
    MessageBufferPool<64> large_pool;
    large_pool.Initialize(1000);

    auto stats = large_pool.GetStats();
    EXPECT_EQ(stats.peak_capacity, 1000);
    EXPECT_EQ(stats.current_available, 1000);

    large_pool.Shutdown();
}
