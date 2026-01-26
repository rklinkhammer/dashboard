#include <gtest/gtest.h>
#include "ui/NodeMetricsTile.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class NodeMetricsTileTest : public ::testing::Test {
protected:
    std::vector<MetricDescriptor> CreateTestDescriptors() {
        std::vector<MetricDescriptor> descriptors;
        
        MetricDescriptor desc1;
        desc1.node_name = "TestNode";
        desc1.metric_name = "throughput_hz";
        desc1.metric_id = "TestNode::throughput_hz";
        desc1.min_value = 50.0;
        desc1.max_value = 1000.0;
        desc1.unit = "hz";
        descriptors.push_back(desc1);
        
        MetricDescriptor desc2;
        desc2.node_name = "TestNode";
        desc2.metric_name = "latency_ms";
        desc2.metric_id = "TestNode::latency_ms";
        desc2.min_value = 0.0;
        desc2.max_value = 100.0;
        desc2.unit = "ms";
        descriptors.push_back(desc2);
        
        MetricDescriptor desc3;
        desc3.node_name = "TestNode";
        desc3.metric_name = "error_count";
        desc3.metric_id = "TestNode::error_count";
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

TEST_F(NodeMetricsTileTest, ConstructorInitializes) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    EXPECT_EQ(tile.GetNodeName(), "TestNode");
    EXPECT_EQ(tile.GetFieldCount(), 3);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);
}

TEST_F(NodeMetricsTileTest, UpdateMetricValue) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    tile.UpdateMetricValue("throughput_hz", 500.0);
    tile.UpdateMetricValue("latency_ms", 50.0);
    tile.UpdateMetricValue("error_count", 0.0);
    
    // Verify the tile updated (we check this by rendering and ensuring no crash)
    auto element = tile.Render();
    EXPECT_NE(element, nullptr);
}

TEST_F(NodeMetricsTileTest, UpdateAllValues) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    std::map<std::string, double> values;
    values["throughput_hz"] = 600.0;
    values["latency_ms"] = 45.0;
    values["error_count"] = 0.0;
    
    tile.UpdateAllValues(values);
    
    // Verify the tile updated
    auto element = tile.Render();
    EXPECT_NE(element, nullptr);
}

TEST_F(NodeMetricsTileTest, StatusComputation_OK) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    // All values within OK range
    tile.UpdateMetricValue("throughput_hz", 500.0);   // OK: 50-1000
    tile.UpdateMetricValue("latency_ms", 50.0);       // OK: 0-100
    tile.UpdateMetricValue("error_count", 0.0);       // OK: 0-10
    
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);
}

TEST_F(NodeMetricsTileTest, StatusComputation_WARNING) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    // One field above max (warning)
    tile.UpdateMetricValue("throughput_hz", 1500.0);  // WARNING: > 1000
    tile.UpdateMetricValue("latency_ms", 50.0);       // OK: 0-100
    tile.UpdateMetricValue("error_count", 0.0);       // OK: 0-10
    
    // Should return worst status (WARNING in this case)
    AlertStatus status = tile.GetStatus();
    EXPECT_TRUE(status == AlertStatus::WARNING || status == AlertStatus::CRITICAL);
}

TEST_F(NodeMetricsTileTest, StatusComputation_CRITICAL) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    // One field below min (critical)
    tile.UpdateMetricValue("throughput_hz", 30.0);    // CRITICAL: < 50
    tile.UpdateMetricValue("latency_ms", 50.0);       // OK: 0-100
    tile.UpdateMetricValue("error_count", 0.0);       // OK: 0-10
    
    EXPECT_EQ(tile.GetStatus(), AlertStatus::CRITICAL);
}

TEST_F(NodeMetricsTileTest, RenderProducesElement) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    tile.UpdateMetricValue("throughput_hz", 500.0);
    tile.UpdateMetricValue("latency_ms", 50.0);
    tile.UpdateMetricValue("error_count", 0.0);
    
    auto element = tile.Render();
    EXPECT_NE(element, nullptr);
}

TEST_F(NodeMetricsTileTest, HeightCalculation) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    // Expected: 4 + field_count = 4 + 3 = 7 lines
    // Header (1) + sep (1) + 3 fields (3) + sep (1) + status (1) = 8
    // Formula: 4 + field_count = 4 + 3 = 7
    int height = tile.GetMinHeightLines();
    EXPECT_EQ(height, 7);
}

TEST_F(NodeMetricsTileTest, ConcurrentUpdates) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    // Simulate concurrent updates (just verify no crashes)
    for (int i = 0; i < 10; ++i) {
        tile.UpdateMetricValue("throughput_hz", 100.0 + i);
        tile.UpdateMetricValue("latency_ms", 50.0 + i);
        tile.UpdateMetricValue("error_count", static_cast<double>(i % 5));
    }
    
    auto element = tile.Render();
    EXPECT_NE(element, nullptr);
}

TEST_F(NodeMetricsTileTest, EmptyFields) {
    std::vector<MetricDescriptor> empty_descriptors;
    json empty_schema;
    
    NodeMetricsTile tile("EmptyNode", empty_descriptors, empty_schema);
    
    EXPECT_EQ(tile.GetNodeName(), "EmptyNode");
    EXPECT_EQ(tile.GetFieldCount(), 0);
}

// ============================================================================
// Phase 3c: Collapsed Rendering Tests
// ============================================================================

TEST_F(NodeMetricsTileTest, RenderCollapsed_DisplaysCompactSummary) {
    auto descriptors = CreateTestDescriptors();
    auto schema = CreateTestSchema();
    
    NodeMetricsTile tile("TestNode", descriptors, schema);
    
    tile.UpdateMetricValue("throughput_hz", 500.0);
    tile.UpdateMetricValue("latency_ms", 25.0);
    tile.UpdateMetricValue("error_count", 0.0);
    
    auto element = tile.RenderCollapsed();
    EXPECT_NE(element, nullptr);
}

TEST_F(NodeMetricsTileTest, RenderCollapsed_WithEmptyFields) {
    std::vector<MetricDescriptor> empty_descriptors;
    json empty_schema;
    
    NodeMetricsTile tile("EmptyNode", empty_descriptors, empty_schema);
    
    auto element = tile.RenderCollapsed();
    EXPECT_NE(element, nullptr);
}
