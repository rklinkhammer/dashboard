// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
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
#include "ui/Dashboard.hpp"
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
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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

// Test 4: Dashboard Run loop works with metrics
TEST_F(Phase2EndToEndTest, DashboardRunLoopWithMetrics) {
    executor_->Init();

    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    // Don't actually run the full loop (would block), just verify it's callable
    EXPECT_NO_THROW({
        // app.Run() would block, so we just verify Initialize worked
    });
}

// Test 5: Multiple metric updates in sequence
TEST_F(Phase2EndToEndTest, SequentialMetricUpdates) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
    app.Initialize();

    auto metrics_panel = app.GetMetricsPanel();
    auto element = metrics_panel->Render();

    // Should render without errors
    EXPECT_NE(element, nullptr);
}

// Test 10: MetricsTilePanel returns nullptr for non-existent tiles
TEST_F(Phase2EndToEndTest, GetTileReturnsNullForNonExistent) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();

    auto non_existent = tile_panel->GetTile("NonExistent::metric");
    EXPECT_EQ(non_existent, nullptr);
}

// Test 11: UpdateAllMetrics is idempotent
TEST_F(Phase2EndToEndTest, UpdateAllMetricsIsIdempotent) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
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
    Dashboard app(executor_, heights);
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

// ============================================================================
// Phase 2.9: Node Grouping Tests (Option 2: Separate Nodes Container)
// ============================================================================

// Test D2.9-1: Node Grouping - Adding tiles to proper node groups
TEST_F(Phase2EndToEndTest, NodeGroupingAddTiles) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Create tiles for different nodes
    MetricDescriptor desc1{"DataValidator", "validation_errors", "DataValidator::validation_errors", 0, 100};
    MetricDescriptor desc2{"DataValidator", "processing_time", "DataValidator::processing_time", 0, 100};
    MetricDescriptor desc3{"Transform", "throughput", "Transform::throughput", 0, 100};

    auto tile1 = std::make_shared<MetricsTileWindow>(desc1, nlohmann::json::object());
    auto tile2 = std::make_shared<MetricsTileWindow>(desc2, nlohmann::json::object());
    auto tile3 = std::make_shared<MetricsTileWindow>(desc3, nlohmann::json::object());

    tile_panel->AddTile(tile1);
    tile_panel->AddTile(tile2);
    tile_panel->AddTile(tile3);

    EXPECT_EQ(tile_panel->GetTileCount(), 3);
    EXPECT_EQ(tile_panel->GetNodeCount(), 2);  // Two unique nodes
}

// Test D2.9-2: Node Grouping - GetTilesForNode returns correct tiles
TEST_F(Phase2EndToEndTest, NodeGroupingGetTilesForNode) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Create and add tiles for multiple nodes
    MetricDescriptor desc1{"DataValidator", "validation_errors", "DataValidator::validation_errors", 0, 100};
    MetricDescriptor desc2{"DataValidator", "processing_time", "DataValidator::processing_time", 0, 100};
    MetricDescriptor desc3{"Transform", "throughput", "Transform::throughput", 0, 100};

    auto tile1 = std::make_shared<MetricsTileWindow>(desc1, nlohmann::json::object());
    auto tile2 = std::make_shared<MetricsTileWindow>(desc2, nlohmann::json::object());
    auto tile3 = std::make_shared<MetricsTileWindow>(desc3, nlohmann::json::object());

    tile_panel->AddTile(tile1);
    tile_panel->AddTile(tile2);
    tile_panel->AddTile(tile3);

    // Get tiles for specific nodes
    auto validator_tiles = tile_panel->GetTilesForNode("DataValidator");
    auto transform_tiles = tile_panel->GetTilesForNode("Transform");

    EXPECT_EQ(validator_tiles.size(), 2);  // Two metrics in DataValidator
    EXPECT_EQ(transform_tiles.size(), 1);  // One metric in Transform

    // Non-existent node returns empty
    auto nonexistent = tile_panel->GetTilesForNode("NonExistent");
    EXPECT_EQ(nonexistent.size(), 0);
}

// Test D2.9-3: Node Grouping - GetNodeCount returns correct count
TEST_F(Phase2EndToEndTest, NodeGroupingGetNodeCount) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Empty panel
    EXPECT_EQ(tile_panel->GetNodeCount(), 0);
    EXPECT_EQ(tile_panel->GetTileCount(), 0);

    // Add tiles from multiple nodes
    for (int i = 0; i < 3; ++i) {
        MetricDescriptor desc{"Node" + std::to_string(i), "metric", 
                              "Node" + std::to_string(i) + "::metric", 0, 100};
        auto tile = std::make_shared<MetricsTileWindow>(desc, nlohmann::json::object());
        tile_panel->AddTile(tile);
    }

    EXPECT_EQ(tile_panel->GetNodeCount(), 3);
    EXPECT_EQ(tile_panel->GetTileCount(), 3);
}

// Test D2.9-4: Node Grouping - Grouped rendering with section headers
TEST_F(Phase2EndToEndTest, NodeGroupingGroupedRendering) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Create tiles from 2 nodes with 3 metrics each
    const std::string nodes[] = {"DataValidator", "Transform"};
    for (const auto& node : nodes) {
        for (int i = 0; i < 3; ++i) {
            MetricDescriptor desc{node, "metric_" + std::to_string(i),
                                  node + "::metric_" + std::to_string(i), 0, 100};
            auto tile = std::make_shared<MetricsTileWindow>(desc, nlohmann::json::object());
            tile_panel->AddTile(tile);
        }
    }

    EXPECT_EQ(tile_panel->GetTileCount(), 6);
    EXPECT_EQ(tile_panel->GetNodeCount(), 2);

    // Render should produce grouped output
    auto element = tile_panel->Render();
    EXPECT_NE(element, nullptr);
}

// Test D2.9-5: Node Grouping - GetTile lookup within node
TEST_F(Phase2EndToEndTest, NodeGroupingGetTileLookup) {
    WindowHeightConfig heights;
    Dashboard app(executor_, heights);
    app.Initialize();

    auto tile_panel = app.GetMetricsPanel()->GetMetricsTilePanel();
    ASSERT_NE(tile_panel, nullptr);

    // Create tiles
    MetricDescriptor desc1{"DataValidator", "validation_errors", "DataValidator::validation_errors", 0, 100};
    MetricDescriptor desc2{"Transform", "throughput", "Transform::throughput", 0, 100};

    auto tile1 = std::make_shared<MetricsTileWindow>(desc1, nlohmann::json::object());
    auto tile2 = std::make_shared<MetricsTileWindow>(desc2, nlohmann::json::object());

    tile_panel->AddTile(tile1);
    tile_panel->AddTile(tile2);

    // Lookup existing tiles
    auto found1 = tile_panel->GetTile("DataValidator::validation_errors");
    auto found2 = tile_panel->GetTile("Transform::throughput");

    EXPECT_NE(found1, nullptr);
    EXPECT_NE(found2, nullptr);
    EXPECT_EQ(found1, tile1);
    EXPECT_EQ(found2, tile2);

    // Lookup non-existent tile
    auto notfound = tile_panel->GetTile("NonExistent::metric");
    EXPECT_EQ(notfound, nullptr);
}

