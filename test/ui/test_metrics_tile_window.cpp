#include <gtest/gtest.h>
#include "ui/MetricsTileWindow.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================================
// MetricsTileWindow Tests - Alert Status and Color Coding
// ============================================================================

class MetricsTileWindowTest : public ::testing::Test {
protected:
    MetricDescriptor CreateCPUMetricDescriptor() {
        MetricDescriptor desc;
        desc.node_name = "ComputeNode";
        desc.metric_name = "cpu_percent";
        desc.metric_id = "ComputeNode::cpu_percent";
        desc.unit = "%";
        desc.min_value = 0.0;
        desc.max_value = 100.0;
        return desc;
    }

    MetricDescriptor CreateMemoryMetricDescriptor() {
        MetricDescriptor desc;
        desc.node_name = "MemoryNode";
        desc.metric_name = "memory_usage_percent";
        desc.metric_id = "MemoryNode::memory_usage_percent";
        desc.unit = "%";
        desc.min_value = 0.0;
        desc.max_value = 100.0;
        return desc;
    }

    json CreateCPUFieldSpec() {
        json spec;
        spec["name"] = "cpu_percent";
        spec["type"] = "double";
        spec["unit"] = "%";
        spec["display_type"] = "gauge";
        spec["alert_rule"]["ok"] = json::array({0.0, 50.0});
        spec["alert_rule"]["warning"] = json::array({50.0, 80.0});
        spec["alert_rule"]["critical_min"] = 80.0;
        return spec;
    }

    json CreateMemoryFieldSpec() {
        json spec;
        spec["name"] = "memory_usage_percent";
        spec["type"] = "double";
        spec["unit"] = "%";
        spec["display_type"] = "gauge";
        spec["alert_rule"]["ok"] = json::array({0.0, 70.0});
        spec["alert_rule"]["warning"] = json::array({70.0, 85.0});
        spec["alert_rule"]["critical_min"] = 85.0;
        return spec;
    }
};

// Test 1: Construct from descriptor and field spec
TEST_F(MetricsTileWindowTest, ConstructsFromDescriptorAndFieldSpec) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();

    ASSERT_NO_THROW({
        MetricsTileWindow tile(desc, spec);
        EXPECT_EQ(tile.GetMetricId(), "ComputeNode::cpu_percent");
        EXPECT_EQ(tile.GetDisplayType(), "gauge");
    });
}

// Test 2: Default status is OK (value at 0)
TEST_F(MetricsTileWindowTest, DefaultStatusIsOK) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);
}

// Test 3: Status becomes WARNING in warning range (CPU 60%)
TEST_F(MetricsTileWindowTest, StatusWarningInWarningRange) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    tile.UpdateValue(60.0);  // In warning range [50, 80]

    EXPECT_EQ(tile.GetStatus(), AlertStatus::WARNING);
}

// Test 4: Status is OK in OK range (CPU 30%)
TEST_F(MetricsTileWindowTest, StatusOKInOKRange) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    tile.UpdateValue(30.0);  // In OK range [0, 50]

    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);
}

// Test 5: Status becomes CRITICAL above warning (CPU 95%)
TEST_F(MetricsTileWindowTest, StatusCriticalAboveWarning) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    tile.UpdateValue(95.0);  // Above warning range

    EXPECT_EQ(tile.GetStatus(), AlertStatus::CRITICAL);
}

// Test 6: Multiple status transitions work correctly
TEST_F(MetricsTileWindowTest, MultipleStatusTransitions) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // OK
    tile.UpdateValue(25.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);

    // WARNING
    tile.UpdateValue(60.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::WARNING);

    // CRITICAL
    tile.UpdateValue(90.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::CRITICAL);

    // Back to OK
    tile.UpdateValue(40.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);
}

// Test 7: Different metric types (memory) have different thresholds
TEST_F(MetricsTileWindowTest, DifferentMetricsHaveDifferentThresholds) {
    auto cpu_desc = CreateCPUMetricDescriptor();
    auto cpu_spec = CreateCPUFieldSpec();
    MetricsTileWindow cpu_tile(cpu_desc, cpu_spec);

    auto mem_desc = CreateMemoryMetricDescriptor();
    auto mem_spec = CreateMemoryFieldSpec();
    MetricsTileWindow mem_tile(mem_desc, mem_spec);

    double test_value = 75.0;

    // CPU at 75% is in WARNING range [50, 80]
    cpu_tile.UpdateValue(test_value);
    EXPECT_EQ(cpu_tile.GetStatus(), AlertStatus::WARNING);

    // Memory at 75% is in OK range [0, 70]... wait, that's wrong
    // Memory at 75% should be in WARNING range [70, 85]
    mem_tile.UpdateValue(test_value);
    EXPECT_EQ(mem_tile.GetStatus(), AlertStatus::WARNING);
}

// Test 8: Sparkline history is maintained
TEST_F(MetricsTileWindowTest, MaintainsSparklineHistory) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // Update with 10 values
    for (int i = 0; i < 10; ++i) {
        tile.UpdateValue(10.0 + i);
    }

    // Render to trigger any rendering that depends on history
    auto element = tile.Render();
    EXPECT_NE(element, nullptr);
}

// Test 9: Value rendering includes color
TEST_F(MetricsTileWindowTest, RendersValueWithProperColor) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // Set to WARNING level
    tile.UpdateValue(65.0);

    auto element = tile.Render();
    // Should render without errors
    EXPECT_NE(element, nullptr);
}

// Test 10: Gauge rendering with alert colors
TEST_F(MetricsTileWindowTest, GaugeRenderingAtDifferentLevels) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // Test rendering at OK level
    tile.UpdateValue(30.0);
    auto ok_element = tile.Render();
    EXPECT_NE(ok_element, nullptr);

    // Test rendering at WARNING level
    tile.UpdateValue(65.0);
    auto warning_element = tile.Render();
    EXPECT_NE(warning_element, nullptr);

    // Test rendering at CRITICAL level
    tile.UpdateValue(90.0);
    auto critical_element = tile.Render();
    EXPECT_NE(critical_element, nullptr);
}

// Test 11: Boundary values (exact threshold points)
TEST_F(MetricsTileWindowTest, BoundaryValueTransitions) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // At OK upper bound
    tile.UpdateValue(50.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);

    // Just above OK bound (in WARNING)
    tile.UpdateValue(50.1);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::WARNING);

    // At WARNING upper bound
    tile.UpdateValue(80.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::WARNING);

    // Just above WARNING bound (in CRITICAL)
    tile.UpdateValue(80.1);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::CRITICAL);
}

// Test 12: Update increments properly
TEST_F(MetricsTileWindowTest, UpdateIncrementsMetricValue) {
    auto desc = CreateCPUMetricDescriptor();
    auto spec = CreateCPUFieldSpec();
    MetricsTileWindow tile(desc, spec);

    // Start with low value (OK)
    tile.UpdateValue(20.0);
    EXPECT_EQ(tile.GetStatus(), AlertStatus::OK);

    // Gradually increase
    for (double val = 20.0; val <= 90.0; val += 10.0) {
        tile.UpdateValue(val);
    }

    // End with high value (CRITICAL)
    EXPECT_EQ(tile.GetStatus(), AlertStatus::CRITICAL);
}

