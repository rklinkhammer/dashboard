#pragma once

#include <memory>
#include <vector>
#include <string>
#include <deque>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <ftxui/dom/elements.hpp>
#include "ui/MetricsTileWindow.hpp"

// Consolidated tile displaying all fields from a single node's metrics schema
// Replaces the previous one-tile-per-field design with one-tile-per-node
class NodeMetricsTile {
public:
    // Constructor: takes node_name and all field specs from schema
    NodeMetricsTile(
        const std::string& node_name,
        const std::vector<MetricDescriptor>& descriptors,
        const nlohmann::json& metrics_schema);

    ~NodeMetricsTile() = default;

    // Update a single metric within this node tile
    void UpdateMetricValue(const std::string& metric_name, double value);

    // Update all metrics at once from a map
    void UpdateAllValues(const std::map<std::string, double>& values);

    // Render consolidated tile as FTXUI element
    ftxui::Element Render() const;

    // Get tile sizing info
    int GetMinHeightLines() const;
    std::string GetNodeName() const { return node_name_; }
    size_t GetFieldCount() const { return field_descriptors_.size(); }

    // Get overall status (worst status of all fields)
    AlertStatus GetStatus() const;

private:
    std::string node_name_;
    std::vector<MetricDescriptor> field_descriptors_;  // All fields for this node
    std::map<std::string, double> latest_values_;      // metric_name -> value
    nlohmann::json metrics_schema_;

    // Per-field render data
    struct FieldRender {
        std::string metric_name;
        double current_value = 0.0;
        AlertStatus status = AlertStatus::OK;
        std::string display_type;
        std::deque<double> history;  // For potential sparkline rendering
    };

    std::vector<FieldRender> field_renders_;
    mutable std::mutex values_lock_;

    // Private rendering helpers
    ftxui::Element RenderFieldRow(const FieldRender& field) const;
    AlertStatus ComputeFieldStatus(const MetricDescriptor& desc, double value) const;
    AlertStatus ComputeOverallStatus() const;
    ftxui::Color GetStatusColor(AlertStatus status) const;
    std::string FormatValue(double value) const;
    std::string StatusString(AlertStatus status) const;
};
