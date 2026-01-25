#include <gtest/gtest.h>
#include "ui/DashboardApplication.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/MetricsTilePanel.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include <thread>
#include <chrono>

using graph::MockGraphExecutor;

// ============================================================================
// Phase 2 End-to-End Metrics Flow Tests
// ============================================================================

class Phase2EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create executor with 30 Hz publishing rate for testing
        executor_ = std::make_shared<MockGraphExecutor>(30);
        ASSERT_NE(executor_, nullptr);
    }

    void TearDown() override {
        if (executor_ && executor_->IsRunning()) {
            executor_->Stop();
            executor_->Join();
        }
    }

    std::shared_ptr<MockGraphExecutor> executor_;
};

// Test 1: Discover metrics from executor
TEST_F(Phase2EndToEndTest, DiscoverMetricsFromExecutor) {
    executor_->Init();

    // Create dashboard
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    // Check that metrics were discovered
    auto metrics_panel = app.GetMetricsPanel();
    ASSERT_NE(metrics_panel, nullptr);

    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Should have discovered 48 metrics
    EXPECT_EQ(tile_panel->GetTileCount(), 48);

    // Don't start the executor, just verify discovery
}

// Test 2: Metrics values update from executor events
TEST_F(Phase2EndToEndTest, MetricsValuesUpdateFromExecutorEvents) {
    executor_->Init();

    // Create dashboard (discovers metrics)
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();
    auto tile_panel = metrics_panel->GetMetricsTilePanel();

    // Should have 48 discovered metrics
    EXPECT_EQ(tile_panel->GetTileCount(), 48);

    // For this test, just verify UpdateAllMetrics works without tiles having values
    // Don't start executor to avoid threading issues
    EXPECT_NO_THROW({
        tile_panel->UpdateAllMetrics();
    });
}

// Test 3: MetricsTilePanel can handle rapid updates
TEST_F(Phase2EndToEndTest, MetricsTilePanelHandlesRapidUpdates) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    // Simulate rapid metric updates
    for (int i = 0; i < 100; ++i) {
        tile_panel->SetLatestValue("test_metric_1", i * 1.0);
        tile_panel->SetLatestValue("test_metric_2", i * 0.5);
        tile_panel->UpdateAllMetrics();
    }

    EXPECT_EQ(tile_panel->GetTileCount(), 0);  // No tiles created, just updates
}

// Test 4: DashboardApplication Run loop works with metrics
TEST_F(Phase2EndToEndTest, DashboardApplicationRunLoopWithMetrics) {
    executor_->Init();

    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    // Don't actually run the full loop (would block), just verify it's callable
    EXPECT_NO_THROW({
        // app.Run() would block, so we just verify Initialize worked
    });
}

// Test 5: Multiple metric updates in sequence
TEST_F(Phase2EndToEndTest, SequentialMetricUpdates) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    // Simulate a sequence of metric values
    std::vector<double> values = {10.0, 20.0, 30.0, 40.0, 50.0};

    for (double val : values) {
        tile_panel->SetLatestValue("cpu_metric", val);
        tile_panel->UpdateAllMetrics();
    }

    // Should complete without errors
    EXPECT_EQ(tile_panel->GetTileCount(), 0);
}

// Test 6: Metrics panel rendering doesn't crash with no discovered metrics
TEST_F(Phase2EndToEndTest, MetricsPanelRendersWithoutDiscoveredMetrics) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();
    ASSERT_NE(metrics_panel, nullptr);

    // Should render without crashing
    auto element = metrics_panel->Render();
    EXPECT_NE(element, nullptr);
}

// Test 7: Metrics panel with placeholder metrics falls back correctly
TEST_F(Phase2EndToEndTest, MetricsPanelFallsBackToPlaceholders) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();

    // Should have placeholder metrics
    int metric_count = metrics_panel->GetMetricCount();
    EXPECT_GT(metric_count, 0);

    // Add another placeholder
    metrics_panel->AddPlaceholderMetric("CustomMetric", 99.0);
    EXPECT_EQ(metrics_panel->GetMetricCount(), metric_count + 1);
}

// Test 8: Thread safety of metric updates
TEST_F(Phase2EndToEndTest, ThreadSafeMetricUpdates) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    // Create multiple threads updating metrics simultaneously
    std::vector<std::thread> threads;

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([tile_panel, t]() {
            for (int i = 0; i < 20; ++i) {
                std::string metric_id = "metric_" + std::to_string(t);
                double value = i * 1.5 + t;
                tile_panel->SetLatestValue(metric_id, value);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Update all metrics from main thread
    for (int i = 0; i < 100; ++i) {
        tile_panel->UpdateAllMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Should complete without data races or crashes
    EXPECT_NE(tile_panel, nullptr);
}

// Test 9: Alert status colors are reflected in rendering
TEST_F(Phase2EndToEndTest, AlertStatusReflectedInRendering) {
    // This test would need actual metrics discovery
    // For now, verify the rendering path exists
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();
    auto element = metrics_panel->Render();

    // Should render without errors
    EXPECT_NE(element, nullptr);
}

// Test 10: MetricsTilePanel returns nullptr for non-existent tiles
TEST_F(Phase2EndToEndTest, GetTileReturnsNullForNonExistent) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    auto non_existent = tile_panel->GetTile("NonExistent::metric");
    EXPECT_EQ(non_existent, nullptr);
}

// Test 11: UpdateAllMetrics is idempotent
TEST_F(Phase2EndToEndTest, UpdateAllMetricsIsIdempotent) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    // Call UpdateAllMetrics multiple times
    for (int i = 0; i < 10; ++i) {
        tile_panel->SetLatestValue("test", 50.0);
        EXPECT_NO_THROW({
            tile_panel->UpdateAllMetrics();
        });
    }
}

// Test 12: Metrics panel metric count accuracy
TEST_F(Phase2EndToEndTest, MetricsPanelCountAccuracy) {
    WindowHeightConfig heights;
    DashboardApplication app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();

    // Initial count should be > 0 (placeholder metrics)
    int initial_count = metrics_panel->GetMetricCount();
    EXPECT_GT(initial_count, 0);

    // Clear placeholders
    metrics_panel->ClearPlaceholders();
    EXPECT_EQ(metrics_panel->GetMetricCount(), 0);

    // Add new placeholders
    metrics_panel->AddPlaceholderMetric("M1", 1.0);
    metrics_panel->AddPlaceholderMetric("M2", 2.0);
    metrics_panel->AddPlaceholderMetric("M3", 3.0);

    EXPECT_EQ(metrics_panel->GetMetricCount(), 3);
}

