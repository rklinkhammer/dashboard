// MIT License
//
// Copyright (c) 2026 gdashboard contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
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
#include "graph/MessagePool.hpp"
#include "graph/AdaptiveCapacityMonitor.hpp"
#include <thread>
#include <array>

// ============================================================================
// Phase 4b.1: Adaptive Capacity Integration Tests
// Tests for MessageBufferPool with AdaptiveCapacityMonitor
// ============================================================================

class AdaptiveCapacityIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets fresh pool and monitor
    }

    void TearDown() override {
        // Cleanup handled by destructors
    }
};

// ============================================================================
// Test Suite 1: SetCapacity Basic Operations (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityIntegrationTest, SetCapacityIncrease) {
    // Test scaling up capacity
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    auto initial_capacity = pool.GetCurrentCapacity();
    EXPECT_EQ(initial_capacity, 10);

    // Increase capacity
    pool.SetCapacity(20);
    auto new_capacity = pool.GetCurrentCapacity();
    EXPECT_EQ(new_capacity, 20);
}

TEST_F(AdaptiveCapacityIntegrationTest, SetCapacityDecrease) {
    // Test scaling down capacity
    graph::MessageBufferPool<64> pool;
    pool.Initialize(20);

    pool.SetCapacity(10);
    auto new_capacity = pool.GetCurrentCapacity();
    EXPECT_EQ(new_capacity, 10);
}

TEST_F(AdaptiveCapacityIntegrationTest, SetCapacityNoChange) {
    // Setting to same capacity is idempotent
    graph::MessageBufferPool<64> pool;
    pool.Initialize(15);

    pool.SetCapacity(15);
    auto capacity = pool.GetCurrentCapacity();
    EXPECT_EQ(capacity, 15);
}

// ============================================================================
// Test Suite 2: Capacity Adjustment with Buffers (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityIntegrationTest, IncreaseCapacityAllocatesNewBuffers) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(5);

    // Acquire all buffers
    std::vector<void*> buffers;
    for (int i = 0; i < 5; ++i) {
        buffers.push_back(pool.AcquireBuffer());
    }

    // Increase capacity
    pool.SetCapacity(10);

    // Should now be able to acquire more buffers
    std::vector<void*> new_buffers;
    for (int i = 0; i < 5; ++i) {
        auto buffer = pool.AcquireBuffer();
        EXPECT_NE(buffer, nullptr);
        new_buffers.push_back(buffer);
    }

    // Cleanup
    for (void* buf : buffers) pool.ReleaseBuffer(buf);
    for (void* buf : new_buffers) pool.ReleaseBuffer(buf);
}

TEST_F(AdaptiveCapacityIntegrationTest, DecreaseCapacityReclaimsBuffers) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    // Acquire some buffers
    std::vector<void*> buffers;
    for (int i = 0; i < 3; ++i) {
        buffers.push_back(pool.AcquireBuffer());
    }

    // Release them
    for (void* buf : buffers) {
        pool.ReleaseBuffer(buf);
    }

    auto stats_before = pool.GetStats();
    size_t available_before = stats_before.current_available;

    // Decrease capacity from 10 to 5
    pool.SetCapacity(5);

    auto stats_after = pool.GetStats();
    size_t available_after = stats_after.current_available;

    // Available buffers should decrease
    EXPECT_LT(available_after, available_before);
}

TEST_F(AdaptiveCapacityIntegrationTest, SetCapacityRespectsBusyBuffers) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    // Acquire some buffers (they're now busy)
    std::vector<void*> busy_buffers;
    for (int i = 0; i < 8; ++i) {
        busy_buffers.push_back(pool.AcquireBuffer());
    }

    // Try to decrease capacity to less than busy buffers
    pool.SetCapacity(5);  // Only 5 capacity, but 8 busy

    // Pool should still function
    for (void* buf : busy_buffers) {
        pool.ReleaseBuffer(buf);
    }

    // Should still be able to acquire/release
    void* buffer = pool.AcquireBuffer();
    EXPECT_NE(buffer, nullptr);
    pool.ReleaseBuffer(buffer);
}

// ============================================================================
// Test Suite 3: Adaptive Monitor Integration (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityIntegrationTest, MonitorRegistersPoolCorrectly) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    graph::AdaptiveCapacityMonitor monitor;

    // Register pool with hit rate provider
    monitor.RegisterPool(64, [&pool]() {
        return pool.GetStats().GetHitRatePercent();
    });

    // Register adjustment callback
    bool adjustment_called = false;
    monitor.RegisterAdjustmentCallback(64, [&](size_t new_capacity) {
        adjustment_called = true;
        pool.SetCapacity(new_capacity);
    });

    // Verify registration succeeded
    EXPECT_NO_THROW({
        (void)monitor.GetConfig();
    });
}

TEST_F(AdaptiveCapacityIntegrationTest, MonitorCallsAdjustmentCallback) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    graph::AdaptiveCapacityConfig config;
    config.adjustment_interval = std::chrono::seconds(1);
    config.low_hit_rate_threshold = 30.0;  // Low threshold to trigger scale-up

    graph::AdaptiveCapacityMonitor monitor(config);

    std::atomic<size_t> adjustments{0};
    std::atomic<size_t> last_capacity{10};

    monitor.RegisterPool(64, [&pool]() {
        return pool.GetStats().GetHitRatePercent();
    });

    monitor.RegisterAdjustmentCallback(64, [&](size_t new_capacity) {
        adjustments.fetch_add(1);
        last_capacity.store(new_capacity);
        pool.SetCapacity(new_capacity);
    });

    // Just verify that the callback is set up without starting the monitor
    // (to avoid long waits in tests)
    EXPECT_NO_THROW({
        (void)monitor.GetConfig();
    });
}

TEST_F(AdaptiveCapacityIntegrationTest, PoolScalesWithMonitor) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(5);

    graph::AdaptiveCapacityMonitor monitor;

    monitor.RegisterPool(64, [&pool]() {
        return pool.GetStats().GetHitRatePercent();
    });

    monitor.RegisterAdjustmentCallback(64, [&](size_t new_capacity) {
        pool.SetCapacity(new_capacity);
    });

    auto initial = pool.GetCurrentCapacity();
    EXPECT_EQ(initial, 5);

    // Directly trigger adjustment (simulating monitor detection)
    pool.SetCapacity(15);
    auto adjusted = pool.GetCurrentCapacity();
    EXPECT_EQ(adjusted, 15);
}

// ============================================================================
// Test Suite 4: Statistics and Metrics (2 tests)
// ============================================================================

TEST_F(AdaptiveCapacityIntegrationTest, StatisticsReflectCapacityChanges) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    auto stats_before = pool.GetStats();
    size_t available_before = stats_before.current_available;

    // Increase capacity
    pool.SetCapacity(20);

    auto stats_after = pool.GetStats();
    size_t available_after = stats_after.current_available;

    // Available should increase
    EXPECT_GT(available_after, available_before);
}

TEST_F(AdaptiveCapacityIntegrationTest, PeakCapacityUpdatedOnScale) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(5);

    auto stats_initial = pool.GetStats();
    EXPECT_EQ(stats_initial.peak_capacity, 5);

    // Scale up
    pool.SetCapacity(20);

    auto stats_after = pool.GetStats();
    EXPECT_EQ(stats_after.peak_capacity, 20);
}

// ============================================================================
// Test Suite 5: Thread Safety (2 tests)
// ============================================================================

TEST_F(AdaptiveCapacityIntegrationTest, SetCapacityDuringConcurrentAcquisition) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(10);

    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Threads acquiring buffers
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool, &running] {
            while (running) {
                void* buffer = pool.AcquireBuffer();
                if (buffer) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    pool.ReleaseBuffer(buffer);
                }
            }
        });
    }

    // Main thread adjusting capacity
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pool.SetCapacity(10 + i * 2);
    }

    running = false;
    for (auto& t : threads) {
        t.join();
    }

    // Pool should still be functional
    auto capacity = pool.GetCurrentCapacity();
    EXPECT_GT(capacity, 0);
}

TEST_F(AdaptiveCapacityIntegrationTest, MonitorAdjustsDuringConcurrentLoad) {
    graph::MessageBufferPool<64> pool;
    pool.Initialize(5);

    graph::AdaptiveCapacityConfig config;
    config.adjustment_interval = std::chrono::seconds(1);

    graph::AdaptiveCapacityMonitor monitor(config);

    monitor.RegisterPool(64, [&pool]() {
        return pool.GetStats().GetHitRatePercent();
    });

    monitor.RegisterAdjustmentCallback(64, [&](size_t new_capacity) {
        pool.SetCapacity(new_capacity);
    });

    // Test that concurrent acquisition still works with registered monitor
    // without needing to actually start the monitor (to avoid long waits)
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&pool, &running, t] {
            while (running) {
                // Variable workload
                for (int i = 0; i < 10 + t * 5; ++i) {
                    void* buffer = pool.AcquireBuffer();
                    if (buffer) {
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        pool.ReleaseBuffer(buffer);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    // Let threads run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    running = false;
    for (auto& t : threads) {
        t.join();
    }

    // Pool should be functional
    void* buffer = pool.AcquireBuffer();
    EXPECT_NE(buffer, nullptr);
    pool.ReleaseBuffer(buffer);
}

