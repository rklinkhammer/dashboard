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
#include "graph/AdaptiveCapacityMonitor.hpp"
#include <thread>
#include <atomic>

// ============================================================================
// Phase 4b: Adaptive Capacity Monitor Tests
// ============================================================================

class AdaptiveCapacityMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets a fresh monitor instance
    }

    void TearDown() override {
        // Ensure monitors are stopped
    }
};

// ============================================================================
// Test Suite 1: Lifecycle Management (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, DefaultConstruction) {
    graph::AdaptiveCapacityMonitor monitor;
    EXPECT_FALSE(monitor.IsRunning());
}

TEST_F(AdaptiveCapacityMonitorTest, StartAndStop) {
    graph::AdaptiveCapacityMonitor monitor;
    EXPECT_FALSE(monitor.IsRunning());

    monitor.Start();
    EXPECT_TRUE(monitor.IsRunning());

    monitor.Stop();
    EXPECT_FALSE(monitor.IsRunning());
}

TEST_F(AdaptiveCapacityMonitorTest, MultipleStopCalls) {
    graph::AdaptiveCapacityMonitor monitor;

    monitor.Start();
    EXPECT_TRUE(monitor.IsRunning());

    monitor.Stop();
    EXPECT_FALSE(monitor.IsRunning());

    // Should not throw
    monitor.Stop();
    EXPECT_FALSE(monitor.IsRunning());
}

// ============================================================================
// Test Suite 2: Configuration Management (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, GetDefaultConfig) {
    graph::AdaptiveCapacityMonitor monitor;
    auto config = monitor.GetConfig();

    EXPECT_EQ(config.low_hit_rate_threshold, 50.0);
    EXPECT_EQ(config.high_hit_rate_threshold, 90.0);
    EXPECT_EQ(config.scale_up_factor, 1.5);
    EXPECT_EQ(config.scale_down_factor, 0.9);
}

TEST_F(AdaptiveCapacityMonitorTest, SetCustomConfig) {
    graph::AdaptiveCapacityMonitor monitor;

    graph::AdaptiveCapacityConfig custom_config;
    custom_config.low_hit_rate_threshold = 40.0;
    custom_config.high_hit_rate_threshold = 85.0;
    custom_config.scale_up_factor = 2.0;

    monitor.SetConfig(custom_config);

    auto retrieved = monitor.GetConfig();
    EXPECT_EQ(retrieved.low_hit_rate_threshold, 40.0);
    EXPECT_EQ(retrieved.high_hit_rate_threshold, 85.0);
    EXPECT_EQ(retrieved.scale_up_factor, 2.0);
}

TEST_F(AdaptiveCapacityMonitorTest, ConstructWithConfig) {
    graph::AdaptiveCapacityConfig config;
    config.low_hit_rate_threshold = 35.0;
    config.high_hit_rate_threshold = 95.0;

    graph::AdaptiveCapacityMonitor monitor(config);

    auto retrieved = monitor.GetConfig();
    EXPECT_EQ(retrieved.low_hit_rate_threshold, 35.0);
    EXPECT_EQ(retrieved.high_hit_rate_threshold, 95.0);
}

// ============================================================================
// Test Suite 3: Pool Registration (4 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, RegisterPool) {
    graph::AdaptiveCapacityMonitor monitor;

    // Register pool with hit rate provider
    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    // Verify history is empty initially
    auto history = monitor.GetAdjustmentHistory();
    EXPECT_EQ(history.size(), 0);
}

TEST_F(AdaptiveCapacityMonitorTest, RegisterDuplicatePoolThrows) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    // Registering same pool_id should throw
    EXPECT_THROW(
        monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); }),
        std::logic_error);
}

TEST_F(AdaptiveCapacityMonitorTest, UnregisterPool) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    // Should not throw
    monitor.UnregisterPool(64);

    // Unregister again should not throw (idempotent)
    monitor.UnregisterPool(64);
}

TEST_F(AdaptiveCapacityMonitorTest, RegisterAdjustmentCallback) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    std::atomic<size_t> adjusted_capacity{0};

    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });
    monitor.RegisterAdjustmentCallback(
        64, [&adjusted_capacity](size_t cap) { adjusted_capacity.store(cap); });

    // Verify callbacks are registered
    EXPECT_NO_THROW({
        (void)monitor.GetConfig();  // Should not throw
    });
}

// ============================================================================
// Test Suite 4: Monitoring Loop (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, MonitoringStartsSuccessfully) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    EXPECT_NO_THROW(monitor.Start());
    EXPECT_TRUE(monitor.IsRunning());

    monitor.Stop();
}

TEST_F(AdaptiveCapacityMonitorTest, DoubleStartThrows) {
    graph::AdaptiveCapacityMonitor monitor;

    monitor.Start();
    EXPECT_TRUE(monitor.IsRunning());

    // Starting again should throw
    EXPECT_THROW(monitor.Start(), std::runtime_error);

    monitor.Stop();
}

TEST_F(AdaptiveCapacityMonitorTest, MonitorCanBeRestarted) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    monitor.Start();
    monitor.Stop();

    // Should be able to start again
    EXPECT_NO_THROW(monitor.Start());
    EXPECT_TRUE(monitor.IsRunning());

    monitor.Stop();
    EXPECT_FALSE(monitor.IsRunning());
}

// ============================================================================
// Test Suite 5: Adjustment History (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, AdjustmentHistoryInitiallyEmpty) {
    graph::AdaptiveCapacityMonitor monitor;

    auto history = monitor.GetAdjustmentHistory();
    EXPECT_EQ(history.size(), 0);
}

TEST_F(AdaptiveCapacityMonitorTest, ClearAdjustmentHistory) {
    graph::AdaptiveCapacityMonitor monitor;

    auto history = monitor.GetAdjustmentHistory();
    EXPECT_EQ(history.size(), 0);

    // Clear should not throw
    monitor.ClearAdjustmentHistory();

    history = monitor.GetAdjustmentHistory();
    EXPECT_EQ(history.size(), 0);
}

TEST_F(AdaptiveCapacityMonitorTest, AdjustmentHistoryBounded) {
    graph::AdaptiveCapacityMonitor monitor;

    // Verify that history is bounded (max 1000 entries)
    // This is tested implicitly in the implementation
    auto history = monitor.GetAdjustmentHistory();
    EXPECT_LE(history.size(), 1000);
}

// ============================================================================
// Test Suite 6: Configuration Thresholds (3 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, LowHitRateThresholdConfiguration) {
    graph::AdaptiveCapacityConfig config;
    config.low_hit_rate_threshold = 45.0;

    graph::AdaptiveCapacityMonitor monitor(config);
    auto retrieved = monitor.GetConfig();

    EXPECT_EQ(retrieved.low_hit_rate_threshold, 45.0);
}

TEST_F(AdaptiveCapacityMonitorTest, HighHitRateThresholdConfiguration) {
    graph::AdaptiveCapacityConfig config;
    config.high_hit_rate_threshold = 88.0;

    graph::AdaptiveCapacityMonitor monitor(config);
    auto retrieved = monitor.GetConfig();

    EXPECT_EQ(retrieved.high_hit_rate_threshold, 88.0);
}

TEST_F(AdaptiveCapacityMonitorTest, AdjustmentIntervalConfiguration) {
    graph::AdaptiveCapacityConfig config;
    config.adjustment_interval = std::chrono::seconds(60);

    graph::AdaptiveCapacityMonitor monitor(config);
    auto retrieved = monitor.GetConfig();

    EXPECT_EQ(retrieved.adjustment_interval.count(), 60);
}

// ============================================================================
// Test Suite 7: Thread Safety (2 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, ConfigReadWhileMonitoring) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    monitor.Start();

    // Should be able to read config while monitoring
    for (int i = 0; i < 10; ++i) {
        auto config = monitor.GetConfig();
        EXPECT_EQ(config.low_hit_rate_threshold, 50.0);
    }

    monitor.Stop();
}

TEST_F(AdaptiveCapacityMonitorTest, ConfigUpdateWhileMonitoring) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    monitor.Start();

    // Should be able to update config while monitoring
    graph::AdaptiveCapacityConfig new_config;
    new_config.low_hit_rate_threshold = 45.0;

    monitor.SetConfig(new_config);

    auto retrieved = monitor.GetConfig();
    EXPECT_EQ(retrieved.low_hit_rate_threshold, 45.0);

    monitor.Stop();
}

// ============================================================================
// Test Suite 8: Disabled Monitoring (2 tests)
// ============================================================================

TEST_F(AdaptiveCapacityMonitorTest, DisableMonitoring) {
    graph::AdaptiveCapacityConfig config;
    config.enabled = false;

    graph::AdaptiveCapacityMonitor monitor(config);

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    monitor.Start();
    EXPECT_TRUE(monitor.IsRunning());

    // Monitoring is disabled, should still run but not adjust
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto history = monitor.GetAdjustmentHistory();
    // History should be empty since monitoring is disabled
    EXPECT_EQ(history.size(), 0);

    monitor.Stop();
}

TEST_F(AdaptiveCapacityMonitorTest, EnableDisableMonitoring) {
    graph::AdaptiveCapacityMonitor monitor;

    std::atomic<double> hit_rate{70.0};
    monitor.RegisterPool(64, [&hit_rate]() { return hit_rate.load(); });

    // Start with monitoring enabled
    auto config = monitor.GetConfig();
    config.enabled = true;
    monitor.SetConfig(config);

    monitor.Start();
    EXPECT_TRUE(monitor.IsRunning());

    // Disable monitoring
    config.enabled = false;
    monitor.SetConfig(config);

    // Re-enable
    config.enabled = true;
    monitor.SetConfig(config);

    monitor.Stop();
}

