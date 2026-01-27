#include <gtest/gtest.h>
#include "ui/MetricsTilePanel.hpp"
#include <memory>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

// Helper function to check if a string contains a substring
static bool StringContains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

class Phase3dCommandsTest : public ::testing::Test {
protected:
    std::shared_ptr<MetricsTilePanel> tile_panel_;

    void SetUp() override {
        // Create just the tile panel without needing full Dashboard
        tile_panel_ = std::make_shared<MetricsTilePanel>();
        AddTestMetrics();
    }

    // Helper to add test metrics
    void AddTestMetrics() {
        if (!tile_panel_) {
            FAIL() << "MetricsTilePanel not available";
        }
        
        std::vector<MetricDescriptor> descriptors;
        
        MetricDescriptor desc1;
        desc1.node_name = "TestNode1";
        desc1.metric_name = "throughput_hz";
        desc1.metric_id = "TestNode1::throughput_hz";
        desc1.min_value = 0.0;
        desc1.max_value = 1000.0;
        desc1.unit = "hz";
        descriptors.push_back(desc1);
        
        MetricDescriptor desc2;
        desc2.node_name = "TestNode1";
        desc2.metric_name = "latency_ms";
        desc2.metric_id = "TestNode1::latency_ms";
        desc2.min_value = 0.0;
        desc2.max_value = 100.0;
        desc2.unit = "ms";
        descriptors.push_back(desc2);
        
        MetricDescriptor desc3;
        desc3.node_name = "TestNode2";
        desc3.metric_name = "error_count";
        desc3.metric_id = "TestNode2::error_count";
        desc3.min_value = 0.0;
        desc3.max_value = 10.0;
        desc3.unit = "count";
        descriptors.push_back(desc3);
        
        nlohmann::json schema;
        schema["fields"] = nlohmann::json::array();
        
        tile_panel_->AddNodeMetrics("TestNode1", {descriptors[0], descriptors[1]}, schema);
        tile_panel_->AddNodeMetrics("TestNode2", {descriptors[2]}, schema);
        
        // Set some values
        tile_panel_->SetLatestValue("TestNode1::throughput_hz", 500.0);
        tile_panel_->SetLatestValue("TestNode1::latency_ms", 25.0);
        tile_panel_->SetLatestValue("TestNode2::error_count", 2.0);
        tile_panel_->UpdateAllMetrics();
    }
};

// ========== Phase 3b: Filtering Tests ==========

TEST_F(Phase3dCommandsTest, MetricsTilePanel_FilterGlobalSetsFilter) {
    tile_panel_->SetGlobalSearchFilter("throughput");
    EXPECT_EQ(tile_panel_->GetGlobalSearchFilter(), "throughput");
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_FilterClearRemovesFilter) {
    tile_panel_->SetGlobalSearchFilter("throughput");
    EXPECT_EQ(tile_panel_->GetGlobalSearchFilter(), "throughput");
    
    tile_panel_->ClearAllFilters();
    EXPECT_EQ(tile_panel_->GetGlobalSearchFilter(), "");
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_NodeFilterSetsPerNodeFilter) {
    tile_panel_->SetNodeFilter("TestNode1", "latency");
    // Verify filter was set (checking via MatchesFilters)
    EXPECT_TRUE(tile_panel_->MatchesFilters("TestNode1", "TestNode1::latency_ms"));
}

// ========== Phase 3c: Collapse/Expand Tests ==========

TEST_F(Phase3dCommandsTest, MetricsTilePanel_CollapseToggle) {
    EXPECT_FALSE(tile_panel_->IsNodeCollapsed("TestNode1"));
    
    tile_panel_->ToggleNodeCollapsed("TestNode1");
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode1"));
    
    tile_panel_->ToggleNodeCollapsed("TestNode1");
    EXPECT_FALSE(tile_panel_->IsNodeCollapsed("TestNode1"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_CollapseSet) {
    tile_panel_->SetNodeCollapsed("TestNode1", true);
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode1"));
    
    tile_panel_->SetNodeCollapsed("TestNode1", false);
    EXPECT_FALSE(tile_panel_->IsNodeCollapsed("TestNode1"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_CollapseAll) {
    tile_panel_->CollapseAll();
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode1"));
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode2"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExpandAll) {
    tile_panel_->CollapseAll();
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode1"));
    
    tile_panel_->ExpandAll();
    EXPECT_FALSE(tile_panel_->IsNodeCollapsed("TestNode1"));
    EXPECT_FALSE(tile_panel_->IsNodeCollapsed("TestNode2"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_GetNodeNames) {
    auto node_names = tile_panel_->GetNodeNames();
    EXPECT_EQ(node_names.size(), 2);
    EXPECT_TRUE(std::find(node_names.begin(), node_names.end(), "TestNode1") != node_names.end());
    EXPECT_TRUE(std::find(node_names.begin(), node_names.end(), "TestNode2") != node_names.end());
}

// ========== Phase 3d: Pinning Tests ==========

TEST_F(Phase3dCommandsTest, MetricsTilePanel_PinToggle) {
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_PinMultipleMetrics) {
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    tile_panel_->TogglePinnedMetric("TestNode1", "latency_ms");
    
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "latency_ms"));
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "error_count"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ClearAllPinned) {
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    tile_panel_->TogglePinnedMetric("TestNode1", "latency_ms");
    
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    
    tile_panel_->ClearAllPinned();
    
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "latency_ms"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_PinAcrossNodes) {
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    tile_panel_->TogglePinnedMetric("TestNode2", "error_count");
    
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode1", "error_count"));
    EXPECT_FALSE(tile_panel_->IsPinnedMetric("TestNode2", "throughput_hz"));
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode2", "error_count"));
}

// ========== Phase 3d: Sorting Tests ==========

TEST_F(Phase3dCommandsTest, MetricsTilePanel_SetSortingMode) {
    EXPECT_EQ(tile_panel_->GetSortingMode(), "name");
    
    tile_panel_->SetSortingMode("value");
    EXPECT_EQ(tile_panel_->GetSortingMode(), "value");
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ValidSortingModes) {
    std::vector<std::string> valid_modes = {"name", "value", "status", "pinned"};
    
    for (const auto& mode : valid_modes) {
        tile_panel_->SetSortingMode(mode);
        EXPECT_EQ(tile_panel_->GetSortingMode(), mode);
    }
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_InvalidSortingModeIgnored) {
    tile_panel_->SetSortingMode("value");
    EXPECT_EQ(tile_panel_->GetSortingMode(), "value");
    
    // Try to set invalid mode
    tile_panel_->SetSortingMode("invalid_mode");
    // Should still be "value"
    EXPECT_EQ(tile_panel_->GetSortingMode(), "value");
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_DefaultSortingMode) {
    auto fresh_panel = std::make_shared<MetricsTilePanel>();
    EXPECT_EQ(fresh_panel->GetSortingMode(), "name");
}

// ========== Phase 3d: Export Tests ==========

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportToJSONProducesValidJson) {
    std::string json_data = tile_panel_->ExportToJSON();
    
    EXPECT_TRUE(StringContains(json_data, "{"));
    EXPECT_TRUE(StringContains(json_data, "}"));
    EXPECT_TRUE(StringContains(json_data, "timestamp"));
    EXPECT_TRUE(StringContains(json_data, "nodes"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportToJSONIncludesMetrics) {
    std::string json_data = tile_panel_->ExportToJSON();
    
    EXPECT_TRUE(StringContains(json_data, "TestNode1"));
    EXPECT_TRUE(StringContains(json_data, "TestNode2"));
    EXPECT_TRUE(StringContains(json_data, "throughput_hz"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportToCSVProducesValidCsv) {
    std::string csv_data = tile_panel_->ExportToCSV();
    
    EXPECT_TRUE(StringContains(csv_data, "Timestamp"));
    EXPECT_TRUE(StringContains(csv_data, "Node"));
    EXPECT_TRUE(StringContains(csv_data, "MetricID"));
    EXPECT_TRUE(StringContains(csv_data, ","));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportToCSVIncludesMetrics) {
    std::string csv_data = tile_panel_->ExportToCSV();
    
    EXPECT_TRUE(StringContains(csv_data, "TestNode1"));
    EXPECT_TRUE(StringContains(csv_data, "TestNode2"));
    EXPECT_TRUE(StringContains(csv_data, "throughput_hz"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportJSONWithFilter) {
    tile_panel_->SetGlobalSearchFilter("throughput");
    std::string json_data = tile_panel_->ExportToJSON();
    
    EXPECT_TRUE(StringContains(json_data, "throughput"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportJSONWithCollapse) {
    tile_panel_->SetNodeCollapsed("TestNode1", true);
    std::string json_data = tile_panel_->ExportToJSON();
    
    EXPECT_TRUE(StringContains(json_data, "TestNode1"));
}

TEST_F(Phase3dCommandsTest, MetricsTilePanel_ExportJSONWithPin) {
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    std::string json_data = tile_panel_->ExportToJSON();
    
    EXPECT_TRUE(StringContains(json_data, "pinned"));
}

// ========== Integration Tests ==========

TEST_F(Phase3dCommandsTest, Integration_FilterAndCollapseWork) {
    tile_panel_->SetGlobalSearchFilter("throughput");
    tile_panel_->SetNodeCollapsed("TestNode1", true);
    
    EXPECT_EQ(tile_panel_->GetGlobalSearchFilter(), "throughput");
    EXPECT_TRUE(tile_panel_->IsNodeCollapsed("TestNode1"));
}

TEST_F(Phase3dCommandsTest, Integration_PinAndSort) {
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    tile_panel_->SetSortingMode("pinned");
    
    EXPECT_TRUE(tile_panel_->IsPinnedMetric("TestNode1", "throughput_hz"));
    EXPECT_EQ(tile_panel_->GetSortingMode(), "pinned");
}

TEST_F(Phase3dCommandsTest, Integration_ExportWithAllFeaturesActive) {
    // Apply all features
    tile_panel_->SetGlobalSearchFilter("throughput");
    tile_panel_->SetNodeCollapsed("TestNode2", true);
    tile_panel_->TogglePinnedMetric("TestNode1", "throughput_hz");
    tile_panel_->SetSortingMode("value");
    
    // Export should include everything
    std::string json_data = tile_panel_->ExportToJSON();
    std::string csv_data = tile_panel_->ExportToCSV();
    
    EXPECT_TRUE(StringContains(json_data, "throughput"));
    EXPECT_TRUE(StringContains(csv_data, "TestNode1"));
}
