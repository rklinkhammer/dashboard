#pragma once

#include <memory>
#include <string>
#include <deque>
#include <nlohmann/json.hpp>
#include <ftxui/dom/elements.hpp>

// Simple descriptor for metric metadata
struct MetricDescriptor {
    std::string node_name;      // e.g., "ProcessingNode"
    std::string metric_name;    // e.g., "throughput_hz"
    std::string metric_id;      // Unique identifier (node_name + metric_name)
    double min_value = 0.0;     // From alert_rule.ok[0]
    double max_value = 100.0;   // From alert_rule.ok[1]
    std::string unit;           // "hz", "ms", "percent", etc.
};

// Represents the alert status of a metric
enum class AlertStatus {
    OK,       // Green
    WARNING,  // Yellow
    CRITICAL  // Red
};

// Individual metric tile window
class MetricsTileWindow {
public:
    // Constructor: configure tile from metric descriptor and field spec
    MetricsTileWindow(
        const MetricDescriptor& descriptor,
        const nlohmann::json& field_spec);

    // Update tile with latest value from MetricsCapability
    void UpdateValue(double new_value);

    // Render tile as FTXUI element
    ftxui::Element Render() const;

    // Get tile sizing info
    int GetMinHeightLines() const;
    std::string GetDisplayType() const { return display_type_; }
    const std::string& GetMetricId() const { return descriptor_.metric_id; }

    // Get current status for color coding
    AlertStatus GetStatus() const;

private:
    MetricDescriptor descriptor_;
    nlohmann::json field_spec_;
    std::string display_type_;  // "value", "gauge", "sparkline", "state"
    std::string description_;
    int precision_ = 2;

    // Data for rendering
    double current_value_ = 0.0;
    std::deque<double> history_;  // For sparkline rendering
    static constexpr size_t MAX_HISTORY = 60;

    // Alert thresholds
    double ok_min_ = 0.0;
    double ok_max_ = 100.0;
    double warning_min_ = 0.0;
    double warning_max_ = 100.0;

    // Private rendering methods for each display type
    ftxui::Element RenderValue() const;
    ftxui::Element RenderGauge() const;
    ftxui::Element RenderSparkline() const;
    ftxui::Element RenderState() const;

    // Helper to get color based on status
    ftxui::Color GetStatusColor() const;
    
    // Helper to format value as string
    std::string FormatValue(double value) const;
};
