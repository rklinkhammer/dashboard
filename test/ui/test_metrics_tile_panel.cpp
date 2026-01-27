#include <gtest/gtest.h>
#include "ui/MetricsTilePanel.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

class MetricsTilePanelTest : public ::testing::Test {
protected:
    std::vector<MetricDescriptor> CreateTestDescriptors(const std::string& node_name) {
        std::vector<MetricDescriptor> descriptors;
        
        MetricDescriptor desc1;
        desc1.node_name = node_name;
        desc1.metric_name = "throughput_hz";
        desc1.metric_id = node_name + "::throughput_hz";
        desc1.min_value = 50.0;
        desc1.max_value = 1000.0;
        desc1.unit = "hz";
        descriptors.push_back(desc1);
        
        MetricDescriptor desc2;
        desc2.node_name = node_name;
        desc2.metric_name = "latency_ms";
        desc2.metric_id = node_name + "::latency_ms";
        desc2.min_value = 0.0;
        desc2.max_value = 100.0;
        desc2.unit = "ms";
        descriptors.push_back(desc2);
        
        MetricDescriptor desc3;
        desc3.node_name = node_name;
        desc3.metric_name = "error_count";
        desc3.metric_id = node_name + "::error_count";
        desc3.min_value = 0.0;
        desc3.max_value = 10.0;
        desc3.unit = "count";
        descriptors.push_back(desc3);
        
        return descriptors;
    }
    
    json CreateTestSchema() {
        json schema;
        schema["fields"] = json::array({
            {
                {"name", "throughput_hz"},
                {"display_type", "value"},
                {"unit", "hz"}
            },
            {
                {"name", "latency_ms"},
                {"display_type", "value"},
                {"unit", "ms"}
            },
            {
                {"name", "error_count"},
                {"display_type", "value"},
                {"unit", "count"}
            }
        });
        return schema;
    }
};

// ============================================================================
// Phase 2: NodeMetricsTile Integration Tests
// ============================================================================

TEST_F(MetricsTilePanelTest, AddNodeMetrics_CreatesConsolidatedTile) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    // Add node metrics
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Verify tile was created
    EXPECT_EQ(panel.GetNodeCount(), 1);
    EXPECT_NE(panel.GetNodeTile("Node1"), nullptr);
}

TEST_F(MetricsTilePanelTest, AddNodeMetrics_FieldCountAccuracy) {
    MetricsTilePanel panel;
    
    auto desc1 = CreateTestDescriptors("Node1");
    auto desc2 = CreateTestDescriptors("Node2");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", desc1, schema);
    panel.AddNodeMetrics("Node2", desc2, schema);
    
    // Each node has 3 fields
    EXPECT_EQ(panel.GetTotalFieldCount(), 6);
    EXPECT_EQ(panel.GetNodeCount(), 2);
}

TEST_F(MetricsTilePanelTest, SetLatestValue_ThreadSafe) {
    MetricsTilePanel panel;
    
    // Set values from different "threads"
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.5);
    panel.SetLatestValue("Node2::error_count", 2.0);
    
    // Add node tiles
    auto descriptors1 = CreateTestDescriptors("Node1");
    auto descriptors2 = CreateTestDescriptors("Node2");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors1, schema);
    panel.AddNodeMetrics("Node2", descriptors2, schema);
    
    // Update all metrics
    panel.UpdateAllMetrics();
    
    // Verify tiles were updated
    auto tile1 = panel.GetNodeTile("Node1");
    auto tile2 = panel.GetNodeTile("Node2");
    
    EXPECT_NE(tile1, nullptr);
    EXPECT_NE(tile2, nullptr);
}

TEST_F(MetricsTilePanelTest, UpdateAllMetrics_RoutesValuesToCorrectNodes) {
    MetricsTilePanel panel;
    
    // Set values for two nodes
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.5);
    panel.SetLatestValue("Node2::error_count", 3.0);
    
    auto descriptors1 = CreateTestDescriptors("Node1");
    auto descriptors2 = CreateTestDescriptors("Node2");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors1, schema);
    panel.AddNodeMetrics("Node2", descriptors2, schema);
    
    // Update all metrics
    panel.UpdateAllMetrics();
    
    // Both tiles should be valid after update
    EXPECT_NE(panel.GetNodeTile("Node1"), nullptr);
    EXPECT_NE(panel.GetNodeTile("Node2"), nullptr);
}

TEST_F(MetricsTilePanelTest, Render_WithConsolidatedTiles) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.5);
    panel.UpdateAllMetrics();
    
    // Should render without error
    auto element = panel.Render();
    EXPECT_TRUE(element);  // FTXUI elements are always truthy
}

TEST_F(MetricsTilePanelTest, Render_WithoutTiles_ShowsNoMetrics) {
    MetricsTilePanel panel;
    
    // Without adding any tiles, should show "No metrics"
    auto element = panel.Render();
    EXPECT_TRUE(element);  // Still renders, just shows "No metrics"
}

TEST_F(MetricsTilePanelTest, GetNodeTile_ReturnsCorrectTile) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("TestNode");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("TestNode", descriptors, schema);
    
    auto tile = panel.GetNodeTile("TestNode");
    EXPECT_NE(tile, nullptr);
    EXPECT_EQ(tile->GetNodeName(), "TestNode");
}

TEST_F(MetricsTilePanelTest, GetNodeTile_ReturnsNullForMissingNode) {
    MetricsTilePanel panel;
    
    auto tile = panel.GetNodeTile("NonExistent");
    EXPECT_EQ(tile, nullptr);
}

// ============================================================================
// Backward Compatibility Tests (Legacy Mode)
// ============================================================================
TEST_F(MetricsTilePanelTest, GetTileCount_ReflectsNodeCount) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // GetTileCount now returns node count (Phase 2)
    EXPECT_EQ(panel.GetTileCount(), 1);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MetricsTilePanelTest, MultipleNodeMetrics_FullWorkflow) {
    MetricsTilePanel panel;
    
    // Add metrics for 3 nodes
    std::vector<std::string> nodes = {"Producer", "Transformer", "Consumer"};
    auto schema = CreateTestSchema();
    
    for (const auto& node : nodes) {
        auto descriptors = CreateTestDescriptors(node);
        panel.AddNodeMetrics(node, descriptors, schema);
    }
    
    // Set values for all nodes
    for (const auto& node : nodes) {
        panel.SetLatestValue(node + "::throughput_hz", 500.0);
        panel.SetLatestValue(node + "::latency_ms", 25.0);
        panel.SetLatestValue(node + "::error_count", 0.0);
    }
    
    // Update all metrics
    panel.UpdateAllMetrics();
    
    // Verify all nodes are present
    EXPECT_EQ(panel.GetNodeCount(), 3);
    EXPECT_EQ(panel.GetTotalFieldCount(), 9);  // 3 fields × 3 nodes
    
    // Verify all nodes can be retrieved
    for (const auto& node : nodes) {
        EXPECT_NE(panel.GetNodeTile(node), nullptr);
    }
    
    // Verify rendering works
    auto element = panel.Render();
    EXPECT_TRUE(element);
}

TEST_F(MetricsTilePanelTest, UpdateLatestValueMultipleTimes) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Set initial value
    panel.SetLatestValue("Node1::throughput_hz", 100.0);
    panel.UpdateAllMetrics();
    
    // Update value multiple times
    for (double value : {200.0, 300.0, 400.0, 500.0}) {
        panel.SetLatestValue("Node1::throughput_hz", value);
        panel.UpdateAllMetrics();
    }
    
    // Panel should still render correctly
    auto element = panel.Render();
    EXPECT_TRUE(element);
}

// ============================================================================
// Phase 3b: Filtering & Search Tests
// ============================================================================

TEST_F(MetricsTilePanelTest, SetGlobalSearchFilter_StoresFilter) {
    MetricsTilePanel panel;
    
    panel.SetGlobalSearchFilter("throughput");
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "throughput");
    
    panel.SetGlobalSearchFilter("latency");
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "latency");
}

TEST_F(MetricsTilePanelTest, SetNodeFilter_StoresPerNodeFilter) {
    MetricsTilePanel panel;
    
    panel.SetNodeFilter("Node1", "throughput");
    EXPECT_EQ(panel.GetNodeFilter("Node1"), "throughput");
    
    panel.SetNodeFilter("Node2", "latency");
    EXPECT_EQ(panel.GetNodeFilter("Node2"), "latency");
    
    // Different nodes have different filters
    EXPECT_NE(panel.GetNodeFilter("Node1"), panel.GetNodeFilter("Node2"));
}

TEST_F(MetricsTilePanelTest, SetNodeFilter_EmptyStringClearsFilter) {
    MetricsTilePanel panel;
    
    panel.SetNodeFilter("Node1", "throughput");
    EXPECT_EQ(panel.GetNodeFilter("Node1"), "throughput");
    
    // Clear with empty string
    panel.SetNodeFilter("Node1", "");
    EXPECT_EQ(panel.GetNodeFilter("Node1"), "");
}

TEST_F(MetricsTilePanelTest, MatchesFilters_GlobalSearch) {
    MetricsTilePanel panel;
    
    // Global search for "throughput" should match only throughput metrics
    panel.SetGlobalSearchFilter("throughput");
    
    EXPECT_TRUE(panel.MatchesFilters("Node1", "throughput_hz"));
    EXPECT_FALSE(panel.MatchesFilters("Node1", "latency_ms"));
    EXPECT_FALSE(panel.MatchesFilters("Node1", "error_count"));
}

TEST_F(MetricsTilePanelTest, MatchesFilters_CaseInsensitiveSearch) {
    MetricsTilePanel panel;
    
    // Search should be case-insensitive
    panel.SetGlobalSearchFilter("LATENCY");
    
    EXPECT_TRUE(panel.MatchesFilters("Node1", "latency_ms"));
    EXPECT_TRUE(panel.MatchesFilters("Node1", "Latency_avg"));
    EXPECT_FALSE(panel.MatchesFilters("Node1", "throughput_hz"));
}

TEST_F(MetricsTilePanelTest, MatchesFilters_NodeFilter) {
    MetricsTilePanel panel;
    
    // Per-node filter for Node1 only
    panel.SetNodeFilter("Node1", "error");
    
    EXPECT_TRUE(panel.MatchesFilters("Node1", "error_count"));
    EXPECT_FALSE(panel.MatchesFilters("Node1", "throughput_hz"));
    
    // Node2 has no filter, so all metrics match
    EXPECT_TRUE(panel.MatchesFilters("Node2", "error_count"));
    EXPECT_TRUE(panel.MatchesFilters("Node2", "throughput_hz"));
}

TEST_F(MetricsTilePanelTest, MatchesFilters_BothGlobalAndNodeFilter) {
    MetricsTilePanel panel;
    
    // Apply both global and node-specific filters
    panel.SetGlobalSearchFilter("throughput");  // Only show metrics with "throughput"
    panel.SetNodeFilter("Node1", "hz");         // In Node1, also filter to "hz"
    
    EXPECT_TRUE(panel.MatchesFilters("Node1", "throughput_hz"));   // Matches both filters
    EXPECT_FALSE(panel.MatchesFilters("Node1", "latency_ms"));     // Fails global filter
    EXPECT_TRUE(panel.MatchesFilters("Node2", "throughput_hz"));   // Passes global filter (no node-specific filter)
}


TEST_F(MetricsTilePanelTest, ClearAllFilters_RemovesAllFilters) {
    MetricsTilePanel panel;
    
    panel.SetGlobalSearchFilter("throughput");
    panel.SetNodeFilter("Node1", "hz");
    
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "throughput");
    EXPECT_EQ(panel.GetNodeFilter("Node1"), "hz");
    
    // Clear all
    panel.ClearAllFilters();
    
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "");
    EXPECT_EQ(panel.GetNodeFilter("Node1"), "");
}

TEST_F(MetricsTilePanelTest, RenderWithFilters_DisplaysFilteredMetrics) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Set values
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 0.0);
    panel.UpdateAllMetrics();
    
    // Apply filter
    panel.SetGlobalSearchFilter("throughput");
    
    // Render with filter
    auto element = panel.Render();
    EXPECT_TRUE(element);
    
    // Verify filter is still active
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "throughput");
}

TEST_F(MetricsTilePanelTest, MultiNodeFiltering_PerNodeFilters) {
    MetricsTilePanel panel;
    
    auto descriptors1 = CreateTestDescriptors("Producer");
    auto descriptors2 = CreateTestDescriptors("Consumer");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Producer", descriptors1, schema);
    panel.AddNodeMetrics("Consumer", descriptors2, schema);
    
    // Apply different filters to each node
    panel.SetNodeFilter("Producer", "throughput");   // Show only throughput in Producer
    panel.SetNodeFilter("Consumer", "error");        // Show only error in Consumer
    
    // Verify filtering works correctly
    EXPECT_TRUE(panel.MatchesFilters("Producer", "throughput_hz"));
    EXPECT_FALSE(panel.MatchesFilters("Producer", "latency_ms"));
    
    EXPECT_TRUE(panel.MatchesFilters("Consumer", "error_count"));
    EXPECT_FALSE(panel.MatchesFilters("Consumer", "throughput_hz"));
}

TEST_F(MetricsTilePanelTest, FilteringWithNoMatches_StillRendersPanel) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 0.0);
    panel.UpdateAllMetrics();
    
    // Apply filter that matches nothing
    panel.SetGlobalSearchFilter("nonexistent");
    
    // Should still render without crashing
    auto element = panel.Render();
    EXPECT_TRUE(element);
}
// ============================================================================
// Phase 3c: Collapse/Expand Tests
// ============================================================================

TEST_F(MetricsTilePanelTest, IsNodeCollapsed_DefaultNotCollapsed) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Default state should be not collapsed
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
}

TEST_F(MetricsTilePanelTest, SetNodeCollapsed_TogglesState) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Initially not collapsed
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
    
    // Set to collapsed
    panel.SetNodeCollapsed("Node1", true);
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    
    // Set back to expanded
    panel.SetNodeCollapsed("Node1", false);
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
}

TEST_F(MetricsTilePanelTest, ToggleNodeCollapsed_SwitchesState) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Initially not collapsed
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
    
    // Toggle to collapsed
    panel.ToggleNodeCollapsed("Node1");
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    
    // Toggle back to expanded
    panel.ToggleNodeCollapsed("Node1");
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
}

TEST_F(MetricsTilePanelTest, MultiNodeCollapseState_Independent) {
    MetricsTilePanel panel;
    
    auto desc1 = CreateTestDescriptors("Node1");
    auto desc2 = CreateTestDescriptors("Node2");
    auto desc3 = CreateTestDescriptors("Node3");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", desc1, schema);
    panel.AddNodeMetrics("Node2", desc2, schema);
    panel.AddNodeMetrics("Node3", desc3, schema);
    
    // Set different collapse states
    panel.SetNodeCollapsed("Node1", true);   // Collapsed
    panel.SetNodeCollapsed("Node2", false);  // Expanded
    panel.SetNodeCollapsed("Node3", true);   // Collapsed
    
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    EXPECT_FALSE(panel.IsNodeCollapsed("Node2"));
    EXPECT_TRUE(panel.IsNodeCollapsed("Node3"));
}

TEST_F(MetricsTilePanelTest, ExpandAll_ExpandsAllNodes) {
    MetricsTilePanel panel;
    
    auto desc1 = CreateTestDescriptors("Node1");
    auto desc2 = CreateTestDescriptors("Node2");
    auto desc3 = CreateTestDescriptors("Node3");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", desc1, schema);
    panel.AddNodeMetrics("Node2", desc2, schema);
    panel.AddNodeMetrics("Node3", desc3, schema);
    
    // Collapse all
    panel.SetNodeCollapsed("Node1", true);
    panel.SetNodeCollapsed("Node2", true);
    panel.SetNodeCollapsed("Node3", true);
    
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    EXPECT_TRUE(panel.IsNodeCollapsed("Node2"));
    EXPECT_TRUE(panel.IsNodeCollapsed("Node3"));
    
    // Expand all
    panel.ExpandAll();
    
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
    EXPECT_FALSE(panel.IsNodeCollapsed("Node2"));
    EXPECT_FALSE(panel.IsNodeCollapsed("Node3"));
}

TEST_F(MetricsTilePanelTest, CollapseAll_CollapsesAllNodes) {
    MetricsTilePanel panel;
    
    auto desc1 = CreateTestDescriptors("Node1");
    auto desc2 = CreateTestDescriptors("Node2");
    auto desc3 = CreateTestDescriptors("Node3");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", desc1, schema);
    panel.AddNodeMetrics("Node2", desc2, schema);
    panel.AddNodeMetrics("Node3", desc3, schema);
    
    // All start expanded
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
    EXPECT_FALSE(panel.IsNodeCollapsed("Node2"));
    EXPECT_FALSE(panel.IsNodeCollapsed("Node3"));
    
    // Collapse all
    panel.CollapseAll();
    
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    EXPECT_TRUE(panel.IsNodeCollapsed("Node2"));
    EXPECT_TRUE(panel.IsNodeCollapsed("Node3"));
}

TEST_F(MetricsTilePanelTest, RenderWithCollapsedNode_DisplaysCollapsedSummary) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 0.0);
    panel.UpdateAllMetrics();
    
    // Collapse the node
    panel.SetNodeCollapsed("Node1", true);
    
    // Should still render without crashing
    auto element = panel.Render();
    EXPECT_TRUE(element);
}

TEST_F(MetricsTilePanelTest, CollapseAndFilter_BothApplied) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 0.0);
    panel.UpdateAllMetrics();
    
    // Apply both collapse and filter
    panel.SetNodeCollapsed("Node1", true);
    panel.SetGlobalSearchFilter("throughput");
    
    // Collapsed takes precedence
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "throughput");
    
    // Should render with collapsed display
    auto element = panel.Render();
    EXPECT_TRUE(element);
}

TEST_F(MetricsTilePanelTest, ToggleCollapse_WithFilter_StaysCollapsed) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 0.0);
    panel.UpdateAllMetrics();
    
    // Set filter
    panel.SetGlobalSearchFilter("latency");
    
    // Collapse node - should show collapsed view
    panel.SetNodeCollapsed("Node1", true);
    EXPECT_TRUE(panel.IsNodeCollapsed("Node1"));
    
    // Toggle back - should show filtered view with filter active
    panel.ToggleNodeCollapsed("Node1");
    EXPECT_FALSE(panel.IsNodeCollapsed("Node1"));
    EXPECT_EQ(panel.GetGlobalSearchFilter(), "latency");
}

// ========== Phase 3d: Sorting & Pinning Tests ==========

TEST_F(MetricsTilePanelTest, TogglePinnedMetric_SetsPinState) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Initially not pinned
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    
    // Toggle to pin
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    
    // Toggle to unpin
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "throughput_hz"));
}

TEST_F(MetricsTilePanelTest, IsPinnedMetric_MultipleMetrics) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Pin multiple metrics
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    panel.TogglePinnedMetric("Node1", "latency_ms");
    
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "latency_ms"));
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "error_count"));
}

TEST_F(MetricsTilePanelTest, ClearAllPinned_RemovesAllPins) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    
    // Pin multiple metrics
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    panel.TogglePinnedMetric("Node1", "latency_ms");
    panel.TogglePinnedMetric("Node1", "error_count");
    
    // Verify all pinned
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "latency_ms"));
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "error_count"));
    
    // Clear all
    panel.ClearAllPinned();
    
    // Verify all unpinned
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "latency_ms"));
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "error_count"));
}

TEST_F(MetricsTilePanelTest, PinnedMetric_AcrossMultipleNodes) {
    MetricsTilePanel panel;
    
    auto descriptors1 = CreateTestDescriptors("Node1");
    auto descriptors2 = CreateTestDescriptors("Node2");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors1, schema);
    panel.AddNodeMetrics("Node2", descriptors2, schema);
    
    // Pin metrics in different nodes
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    panel.TogglePinnedMetric("Node2", "latency_ms");
    
    // Verify pin state is independent per node
    EXPECT_TRUE(panel.IsPinnedMetric("Node1", "throughput_hz"));
    EXPECT_FALSE(panel.IsPinnedMetric("Node1", "latency_ms"));
    EXPECT_FALSE(panel.IsPinnedMetric("Node2", "throughput_hz"));
    EXPECT_TRUE(panel.IsPinnedMetric("Node2", "latency_ms"));
}

TEST_F(MetricsTilePanelTest, SetSortingMode_ValidModes) {
    MetricsTilePanel panel;
    
    // Test valid modes
    panel.SetSortingMode("name");
    EXPECT_EQ(panel.GetSortingMode(), "name");
    
    panel.SetSortingMode("value");
    EXPECT_EQ(panel.GetSortingMode(), "value");
    
    panel.SetSortingMode("status");
    EXPECT_EQ(panel.GetSortingMode(), "status");
    
    panel.SetSortingMode("pinned");
    EXPECT_EQ(panel.GetSortingMode(), "pinned");
}

TEST_F(MetricsTilePanelTest, SetSortingMode_InvalidMode) {
    MetricsTilePanel panel;
    
    panel.SetSortingMode("name");
    EXPECT_EQ(panel.GetSortingMode(), "name");
    
    // Try to set invalid mode - should not change
    panel.SetSortingMode("invalid_mode");
    EXPECT_EQ(panel.GetSortingMode(), "name");
}

TEST_F(MetricsTilePanelTest, GetSortingMode_DefaultMode) {
    MetricsTilePanel panel;
    
    // Default should be "name"
    EXPECT_EQ(panel.GetSortingMode(), "name");
}

TEST_F(MetricsTilePanelTest, ExportToJSON_HasRequiredFields) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    std::string json_str = panel.ExportToJSON();
    json result = json::parse(json_str);
    
    // Verify structure
    EXPECT_TRUE(result.contains("timestamp"));
    EXPECT_TRUE(result.contains("version"));
    EXPECT_TRUE(result.contains("nodes"));
    EXPECT_TRUE(result.contains("filter"));
    EXPECT_TRUE(result.contains("sorting_mode"));
    
    // Verify version
    EXPECT_EQ(result["version"], "1.0");
    
    // Verify default sorting mode
    EXPECT_EQ(result["sorting_mode"], "name");
}

TEST_F(MetricsTilePanelTest, ExportToJSON_IncludesNodeData) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    std::string json_str = panel.ExportToJSON();
    json result = json::parse(json_str);
    
    // Verify nodes array
    EXPECT_TRUE(result["nodes"].is_array());
    EXPECT_GT(result["nodes"].size(), 0);
    
    // Verify node content
    const auto& node = result["nodes"][0];
    EXPECT_EQ(node["node_name"], "Node1");
    EXPECT_TRUE(node.contains("metrics"));
    EXPECT_TRUE(node.contains("collapsed"));
}

TEST_F(MetricsTilePanelTest, ExportToJSON_WithFilter) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    // Set filter
    panel.SetGlobalSearchFilter("throughput");
    
    std::string json_str = panel.ExportToJSON();
    json result = json::parse(json_str);
    
    // Verify filter is included
    EXPECT_EQ(result["filter"], "throughput");
}

TEST_F(MetricsTilePanelTest, ExportToJSON_WithPinnedMetrics) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    // Pin a metric
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    
    std::string json_str = panel.ExportToJSON();
    json result = json::parse(json_str);
    
    // Verify pinned flag in metrics
    bool found_pinned = false;
    for (const auto& metric : result["nodes"][0]["metrics"]) {
        if (metric["metric_id"] == "Node1::throughput_hz") {
            EXPECT_TRUE(metric["pinned"]);
            found_pinned = true;
        }
    }
    EXPECT_TRUE(found_pinned);
}

TEST_F(MetricsTilePanelTest, ExportToCSV_HasRequiredColumns) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    std::string csv_str = panel.ExportToCSV();
    
    // Verify header row
    EXPECT_TRUE(csv_str.find("Timestamp") != std::string::npos);
    EXPECT_TRUE(csv_str.find("Node") != std::string::npos);
    EXPECT_TRUE(csv_str.find("MetricID") != std::string::npos);
    EXPECT_TRUE(csv_str.find("Value") != std::string::npos);
    EXPECT_TRUE(csv_str.find("Pinned") != std::string::npos);
    EXPECT_TRUE(csv_str.find("Collapsed") != std::string::npos);
    EXPECT_TRUE(csv_str.find("Status") != std::string::npos);
}

TEST_F(MetricsTilePanelTest, ExportToCSV_IncludesMetricData) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.5);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    std::string csv_str = panel.ExportToCSV();
    
    // Verify node name and metric IDs are in CSV
    EXPECT_TRUE(csv_str.find("Node1") != std::string::npos);
    EXPECT_TRUE(csv_str.find("throughput_hz") != std::string::npos);
    EXPECT_TRUE(csv_str.find("latency_ms") != std::string::npos);
    EXPECT_TRUE(csv_str.find("error_count") != std::string::npos);
    
    // Verify values are present
    EXPECT_TRUE(csv_str.find("500") != std::string::npos);
    EXPECT_TRUE(csv_str.find("25.5") != std::string::npos);
    EXPECT_TRUE(csv_str.find("2") != std::string::npos);
}

TEST_F(MetricsTilePanelTest, ExportToCSV_WithPinnedMetrics) {
    MetricsTilePanel panel;
    
    auto descriptors = CreateTestDescriptors("Node1");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors, schema);
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node1::latency_ms", 25.0);
    panel.SetLatestValue("Node1::error_count", 2.0);
    panel.UpdateAllMetrics();
    
    // Pin throughput_hz
    panel.TogglePinnedMetric("Node1", "throughput_hz");
    
    std::string csv_str = panel.ExportToCSV();
    
    // Verify "yes" appears for pinned metric
    auto lines = [](const std::string& s) {
        std::vector<std::string> result;
        std::istringstream iss(s);
        std::string line;
        while (std::getline(iss, line)) {
            result.push_back(line);
        }
        return result;
    };
    
    auto csv_lines = lines(csv_str);
    bool found_pinned = false;
    for (const auto& line : csv_lines) {
        if (line.find("throughput_hz") != std::string::npos) {
            // This line should have "yes" in the Pinned column
            size_t pinned_col = line.rfind(',');
            pinned_col = line.rfind(',', pinned_col - 1);  // Second to last comma
            pinned_col = line.rfind(',', pinned_col - 1);  // Third to last comma
            if (line.find("yes", pinned_col) != std::string::npos) {
                found_pinned = true;
            }
        }
    }
    EXPECT_TRUE(found_pinned);
}

TEST_F(MetricsTilePanelTest, ExportToJSON_MultiplNodes) {
    MetricsTilePanel panel;
    
    auto descriptors1 = CreateTestDescriptors("Node1");
    auto descriptors2 = CreateTestDescriptors("Node2");
    auto schema = CreateTestSchema();
    
    panel.AddNodeMetrics("Node1", descriptors1, schema);
    panel.AddNodeMetrics("Node2", descriptors2, schema);
    
    panel.SetLatestValue("Node1::throughput_hz", 500.0);
    panel.SetLatestValue("Node2::throughput_hz", 600.0);
    panel.UpdateAllMetrics();
    
    std::string json_str = panel.ExportToJSON();
    json result = json::parse(json_str);
    
    // Verify both nodes in export
    EXPECT_EQ(result["nodes"].size(), 2);
    
    bool found_node1 = false, found_node2 = false;
    for (const auto& node : result["nodes"]) {
        if (node["node_name"] == "Node1") found_node1 = true;
        if (node["node_name"] == "Node2") found_node2 = true;
    }
    EXPECT_TRUE(found_node1);
    EXPECT_TRUE(found_node2);
}
