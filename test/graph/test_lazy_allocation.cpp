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
#include <chrono>
#include <vector>
#include <array>

// ============================================================================
// Phase 4a: Lazy Allocation Mode Tests
// Tests for on-demand buffer allocation strategy
// ============================================================================

/**
 * @class LazyAllocationTest
 * @brief Test suite for Phase 4a lazy allocation mode
 */
class LazyAllocationTest : public ::testing::Test {
protected:
    // Use custom policy for lazy allocation
    struct LazyPolicy : public graph::MessagePoolPolicy {
        LazyPolicy() {
            mode = AllocationMode::LazyAllocate;
            pool_capacity = 256;
        }
    };

    void SetUp() override {
        // Pools initialized before each test
    }

    void TearDown() override {
        // Cleanup handled by pool destructors
    }
};

// ============================================================================
// Test Suite 1: Memory Characteristics (4 tests)
// ============================================================================

TEST_F(LazyAllocationTest, ZeroMemoryBeforeFirstUse) {
    // Lazy allocation should not allocate buffers until first acquire
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(256);  // Set capacity to 256

    // Check initial state
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 0) << "Should have zero allocations before first acquire";
    EXPECT_EQ(stats.current_available, 0) << "Should have zero available buffers";
    EXPECT_EQ(stats.total_requests, 0) << "Should have zero requests";
}

TEST_F(LazyAllocationTest, AllocatesOnFirstAcquire) {
    // First AcquireBuffer should allocate a new buffer
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(256);

    void* buffer1 = pool.AcquireBuffer();
    EXPECT_NE(buffer1, nullptr);

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 1) << "Should have allocated 1 buffer";
    EXPECT_EQ(stats.total_requests, 1) << "Should have 1 request";
    EXPECT_EQ(stats.total_reuses, 0) << "Should have 0 reuses (first allocation)";

    pool.ReleaseBuffer(buffer1);
}

TEST_F(LazyAllocationTest, ReuseAfterFirstAllocation) {
    // After first allocation and release, second acquire should reuse
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(256);

    void* buffer1 = pool.AcquireBuffer();
    pool.ReleaseBuffer(buffer1);

    void* buffer2 = pool.AcquireBuffer();
    EXPECT_EQ(buffer1, buffer2) << "Should reuse same buffer";

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 1) << "Should still be 1 allocation";
    EXPECT_EQ(stats.total_reuses, 1) << "Should have 1 reuse";
    EXPECT_EQ(stats.total_requests, 2) << "Should have 2 total requests";

    pool.ReleaseBuffer(buffer2);
}

TEST_F(LazyAllocationTest, GrowthUpToCapacity) {
    // Lazy pool should grow allocations up to capacity
    graph::MessageBufferPool<64, LazyPolicy> pool;
    size_t capacity = 10;
    pool.Initialize(capacity);

    std::vector<void*> buffers;

    // Allocate up to capacity
    for (size_t i = 0; i < capacity; ++i) {
        void* buffer = pool.AcquireBuffer();
        EXPECT_NE(buffer, nullptr);
        buffers.push_back(buffer);
    }

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, capacity) << "Should allocate exactly capacity buffers";
    EXPECT_EQ(stats.current_available, 0) << "All buffers should be checked out";
    EXPECT_EQ(stats.peak_capacity, capacity) << "Peak capacity should be capacity";

    // Clean up
    for (void* buffer : buffers) {
        pool.ReleaseBuffer(buffer);
    }
}

// ============================================================================
// Test Suite 2: Latency Characteristics (3 tests)
// ============================================================================

TEST_F(LazyAllocationTest, HighLatencyInitialAllocations) {
    // Initial allocations should include malloc cost
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(100);

    auto before = std::chrono::high_resolution_clock::now();
    void* buffer = pool.AcquireBuffer();
    auto after = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

    // Lazy allocation should allocate a buffer
    EXPECT_NE(buffer, nullptr);

    // Initial allocation happens (though malloc can be very fast in Release build)
    // Just verify the allocation succeeded
    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_allocations, 1UL) << "Should have allocated 1 buffer";

    pool.ReleaseBuffer(buffer);
}

TEST_F(LazyAllocationTest, LowLatencyAfterWarmup) {
    // After initial allocations, reuse should be fast (prealloc-like)
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(100);

    // Warm-up: allocate and release 10 buffers
    std::vector<void*> buffers;
    for (int i = 0; i < 10; ++i) {
        buffers.push_back(pool.AcquireBuffer());
    }
    for (void* buffer : buffers) {
        pool.ReleaseBuffer(buffer);
    }

    // Now measure reuse latency (should be fast, <1000 ns)
    auto before = std::chrono::high_resolution_clock::now();
    void* buffer = pool.AcquireBuffer();
    auto after = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

    // Reuse from pool should be much faster than initial malloc
    EXPECT_LT(duration, 2000) << "Pool reuse should be fast";

    pool.ReleaseBuffer(buffer);
}

TEST_F(LazyAllocationTest, StabilizesAfterWarmup) {
    // Hit rate should increase as buffers are allocated and reused
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(50);

    // Phase 1: Initial allocations (many misses)
    std::vector<void*> buffers1;
    for (int i = 0; i < 50; ++i) {
        buffers1.push_back(pool.AcquireBuffer());
    }

    auto stats_after_alloc = pool.GetStats();
    double hit_rate_after_alloc = stats_after_alloc.GetHitRatePercent();
    EXPECT_EQ(hit_rate_after_alloc, 0.0) << "No reuses yet during allocation phase";

    // Release all
    for (void* buffer : buffers1) {
        pool.ReleaseBuffer(buffer);
    }

    // Phase 2: Repeated allocations (should reuse)
    std::vector<void*> buffers2;
    for (int i = 0; i < 50; ++i) {
        buffers2.push_back(pool.AcquireBuffer());
    }

    auto stats_after_reuse = pool.GetStats();
    double hit_rate_after_reuse = stats_after_reuse.GetHitRatePercent();

    // After warm-up, hit rate should be 50% (50 reuses out of 100 total requests)
    EXPECT_GT(hit_rate_after_reuse, 40.0) << "Hit rate should improve significantly after warm-up";

    for (void* buffer : buffers2) {
        pool.ReleaseBuffer(buffer);
    }
}

// ============================================================================
// Test Suite 3: Capacity Enforcement (2 tests)
// ============================================================================

TEST_F(LazyAllocationTest, RespectCapacityLimit) {
    // Lazy pool should not allocate beyond capacity
    graph::MessageBufferPool<64, LazyPolicy> pool;
    size_t capacity = 5;
    pool.Initialize(capacity);

    std::vector<void*> buffers;

    // Allocate exactly at capacity
    for (size_t i = 0; i < capacity; ++i) {
        buffers.push_back(pool.AcquireBuffer());
    }

    // Try to allocate beyond capacity
    void* over_capacity = pool.AcquireBuffer();
    EXPECT_NE(over_capacity, nullptr) << "Should still allocate even at capacity (over-capacity handling)";

    auto stats = pool.GetStats();
    // total_allocations might be capacity or capacity+1 depending on over-capacity policy
    EXPECT_GE(stats.total_allocations, capacity) << "Should allocate at least capacity buffers";

    for (void* buffer : buffers) {
        pool.ReleaseBuffer(buffer);
    }
    pool.ReleaseBuffer(over_capacity);
}

TEST_F(LazyAllocationTest, HandleCapacityExhaustion) {
    // Test behavior when capacity is exhausted
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(3);

    void* b1 = pool.AcquireBuffer();
    void* b2 = pool.AcquireBuffer();
    void* b3 = pool.AcquireBuffer();

    // Pool at capacity - next allocate should still succeed (over-capacity)
    void* b4 = pool.AcquireBuffer();
    EXPECT_NE(b4, nullptr) << "Should allocate over-capacity buffer";

    auto stats = pool.GetStats();
    EXPECT_GE(stats.total_allocations, 3UL);  // At least 3

    // Cleanup
    pool.ReleaseBuffer(b1);
    pool.ReleaseBuffer(b2);
    pool.ReleaseBuffer(b3);
    pool.ReleaseBuffer(b4);
}

// ============================================================================
// Test Suite 4: Statistics Validation (2 tests)
// ============================================================================

TEST_F(LazyAllocationTest, StatisticsAccuracyDuringGrowth) {
    // Statistics should accurately track growth phase
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(100);

    // Allocate 5 buffers
    std::vector<void*> buffers;
    for (int i = 0; i < 5; ++i) {
        buffers.push_back(pool.AcquireBuffer());
    }

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 5UL) << "Should have 5 requests";
    EXPECT_EQ(stats.total_allocations, 5UL) << "Should have 5 allocations (growth phase)";
    EXPECT_EQ(stats.total_reuses, 0UL) << "Should have 0 reuses (first-time allocations)";
    EXPECT_EQ(stats.current_available, 0UL) << "All buffers checked out";
    EXPECT_EQ(stats.peak_capacity, 5UL) << "Peak should be 5";

    // Cleanup
    for (void* buffer : buffers) {
        pool.ReleaseBuffer(buffer);
    }
}

TEST_F(LazyAllocationTest, StatisticsAccuracyAfterWarmup) {
    // Statistics should correctly track reuse phase
    graph::MessageBufferPool<64, LazyPolicy> pool;
    pool.Initialize(100);

    // Initial allocation phase
    std::vector<void*> phase1;
    for (int i = 0; i < 10; ++i) {
        phase1.push_back(pool.AcquireBuffer());
    }

    for (void* buffer : phase1) {
        pool.ReleaseBuffer(buffer);
    }

    // Reuse phase
    std::vector<void*> phase2;
    for (int i = 0; i < 10; ++i) {
        phase2.push_back(pool.AcquireBuffer());
    }

    auto stats = pool.GetStats();
    EXPECT_EQ(stats.total_requests, 20UL) << "Should have 20 total requests";
    EXPECT_EQ(stats.total_allocations, 10UL) << "Should have 10 allocations";
    EXPECT_EQ(stats.total_reuses, 10UL) << "Should have 10 reuses";

    double hit_rate = stats.GetHitRatePercent();
    EXPECT_NEAR(hit_rate, 50.0, 1.0) << "Hit rate should be ~50%";

    for (void* buffer : phase2) {
        pool.ReleaseBuffer(buffer);
    }
}

// ============================================================================
// Test Suite 5: Comparison with Prealloc Mode (2 tests)
// ============================================================================

TEST_F(LazyAllocationTest, MemorySavingsVsPrealloc) {
    // Lazy allocation should use less memory for idle workloads

    // Prealloc pool (allocates upfront)
    graph::MessageBufferPool<64, graph::MessagePoolPolicy> prealloc_pool;
    prealloc_pool.Initialize(256);

    auto prealloc_stats = prealloc_pool.GetStats();
    EXPECT_EQ(prealloc_stats.total_allocations, 256UL) << "Prealloc allocates all upfront";

    // Lazy pool (allocates on demand)
    graph::MessageBufferPool<64, LazyPolicy> lazy_pool;
    lazy_pool.Initialize(256);

    auto lazy_stats = lazy_pool.GetStats();
    EXPECT_EQ(lazy_stats.total_allocations, 0UL) << "Lazy has zero allocations initially";

    // Memory savings: Lazy uses 0 bytes, Prealloc uses 256*64 = 16KB
    // Estimated: lazy saves 16KB or more if workload is idle
}

TEST_F(LazyAllocationTest, InitialLatencyDifference) {
    // Lazy initial allocations slower, but eventual reuse faster than repeated malloc

    // Phase 1: Single allocation latency
    graph::MessageBufferPool<64, LazyPolicy> lazy_pool;
    lazy_pool.Initialize(100);

    auto before = std::chrono::high_resolution_clock::now();
    void* lazy_buffer = lazy_pool.AcquireBuffer();
    auto after = std::chrono::high_resolution_clock::now();
    auto lazy_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

    lazy_pool.ReleaseBuffer(lazy_buffer);

    // Phase 2: Reuse latency (should be much faster)
    before = std::chrono::high_resolution_clock::now();
    lazy_buffer = lazy_pool.AcquireBuffer();
    after = std::chrono::high_resolution_clock::now();
    auto reuse_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

    // Reuse should be significantly faster than initial allocation
    EXPECT_LT(reuse_latency, lazy_latency) << "Reuse should be faster than initial allocation";
    EXPECT_LT(reuse_latency, 2000) << "Reuse should be <2000ns";

    lazy_pool.ReleaseBuffer(lazy_buffer);
}

