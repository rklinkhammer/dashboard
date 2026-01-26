#include "ui/NodeMetricsTile.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("ui.NodeMetricsTile");

NodeMetricsTile::NodeMetricsTile(
    const std::string& node_name,
    const std::vector<MetricDescriptor>& descriptors,
    const nlohmann::json& metrics_schema)
    : node_name_(node_name),
      field_descriptors_(descriptors),
      metrics_schema_(metrics_schema) {
    
    LOG4CXX_TRACE(logger_, "NodeMetricsTile: Created for node: " << node_name 
                  << " with " << descriptors.size() << " fields");
    
    // Initialize field_renders_ from descriptors
    for (const auto& desc : field_descriptors_) {
        FieldRender field_render;
        field_render.metric_name = desc.metric_name;
        field_render.current_value = 0.0;
        field_render.status = AlertStatus::OK;
        field_render.display_type = "value";  // Default
        field_render.history.clear();
        
        // Extract display type from schema if available
        if (metrics_schema.contains("fields")) {
            for (const auto& field_spec : metrics_schema["fields"]) {
                if (field_spec.contains("name") && 
                    field_spec["name"].get<std::string>() == desc.metric_name) {
                    if (field_spec.contains("display_type")) {
                        field_render.display_type = 
                            field_spec["display_type"].get<std::string>();
                    }
                    break;
                }
            }
        }
        
        field_renders_.push_back(field_render);
    }
}

void NodeMetricsTile::UpdateMetricValue(const std::string& metric_name, double value) {
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        latest_values_[metric_name] = value;
    }
    
    // Update field renders
    for (auto& field : field_renders_) {
        if (field.metric_name == metric_name) {
            field.current_value = value;
            field.history.push_back(value);
            if (field.history.size() > 60) {  // Keep 60-point history
                field.history.pop_front();
            }
            
            // Update status for this field
            for (const auto& desc : field_descriptors_) {
                if (desc.metric_name == metric_name) {
                    field.status = ComputeFieldStatus(desc, value);
                    break;
                }
            }
            break;
        }
    }
}

void NodeMetricsTile::UpdateAllValues(const std::map<std::string, double>& values) {
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        latest_values_ = values;
    }
    
    // Update all field renders
    for (auto& field : field_renders_) {
        auto it = values.find(field.metric_name);
        if (it != values.end()) {
            field.current_value = it->second;
            field.history.push_back(it->second);
            if (field.history.size() > 60) {
                field.history.pop_front();
            }
            
            // Update status
            for (const auto& desc : field_descriptors_) {
                if (desc.metric_name == field.metric_name) {
                    field.status = ComputeFieldStatus(desc, it->second);
                    break;
                }
            }
        }
    }
}

AlertStatus NodeMetricsTile::ComputeFieldStatus(
    const MetricDescriptor& desc, double value) const {
    
    // For now, use a simple threshold-based approach
    // Values within [min_value, max_value] are OK
    // Below min is critical, above max is warning
    
    if (value >= desc.min_value && value <= desc.max_value) {
        return AlertStatus::OK;
    } else if (value < desc.min_value) {
        // Below minimum is critical
        return AlertStatus::CRITICAL;
    } else {
        // Above maximum is warning
        return AlertStatus::WARNING;
    }
}

AlertStatus NodeMetricsTile::ComputeOverallStatus() const {
    AlertStatus worst = AlertStatus::OK;
    
    for (const auto& field : field_renders_) {
        if (field.status == AlertStatus::CRITICAL) {
            return AlertStatus::CRITICAL;  // CRITICAL is worst
        }
        if (field.status == AlertStatus::WARNING) {
            worst = AlertStatus::WARNING;
        }
    }
    
    return worst;
}

AlertStatus NodeMetricsTile::GetStatus() const {
    return ComputeOverallStatus();
}

ftxui::Color NodeMetricsTile::GetStatusColor(AlertStatus status) const {
    switch (status) {
        case AlertStatus::OK:
            return ftxui::Color::Green;
        case AlertStatus::WARNING:
            return ftxui::Color::Yellow;
        case AlertStatus::CRITICAL:
            return ftxui::Color::Red;
    }
    return ftxui::Color::White;
}

std::string NodeMetricsTile::FormatValue(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    return oss.str();
}

std::string NodeMetricsTile::StatusString(AlertStatus status) const {
    switch (status) {
        case AlertStatus::OK:
            return "OK";
        case AlertStatus::WARNING:
            return "WARN";
        case AlertStatus::CRITICAL:
            return "CRIT";
    }
    return "UNKNOWN";
}

ftxui::Element NodeMetricsTile::RenderFieldRow(const FieldRender& field) const {
    using namespace ftxui;
    
    // Find the unit for this field
    std::string unit;
    for (const auto& desc : field_descriptors_) {
        if (desc.metric_name == field.metric_name) {
            unit = desc.unit;
            break;
        }
    }
    
    // Format: "metric_name:    value    unit"
    std::string label = field.metric_name + ":";
    std::string value_str = FormatValue(field.current_value);
    if (!unit.empty()) {
        value_str += " " + unit;
    }
    
    auto row = hbox({
        text(label) | size(WIDTH, LESS_THAN, 25),
        filler(),
        text(value_str) | color(GetStatusColor(field.status))
    });
    
    return row;
}

ftxui::Element NodeMetricsTile::Render() const {
    using namespace ftxui;
    
    std::string header = node_name_ + " (" + 
                        std::to_string(field_descriptors_.size()) + " metrics)";
    
    std::vector<Element> rows;
    rows.push_back(text(header) | bold | color(Color::Cyan));
    rows.push_back(separator());
    
    // Add each field row
    for (const auto& field : field_renders_) {
        rows.push_back(RenderFieldRow(field));
    }
    
    // Add status line
    rows.push_back(separator());
    auto overall_status = ComputeOverallStatus();
    rows.push_back(text("Status: " + StatusString(overall_status)) 
                  | color(GetStatusColor(overall_status)) | align_right);
    
    return vbox(std::move(rows)) | border | color(Color::Green);
}

int NodeMetricsTile::GetMinHeightLines() const {
    // Header (1) + separator (1) + field rows (N) + separator (1) + status (1)
    // = 2 + field_count + 2 = 4 + field_count
    return 4 + static_cast<int>(field_descriptors_.size());
}
size_t NodeMetricsTile::GetFilteredFieldCount(const std::function<bool(const std::string&)>& filter) const {
    size_t count = 0;
    for (const auto& field : field_renders_) {
        if (filter(field.metric_name)) {
            count++;
        }
    }
    return count;
}

ftxui::Element NodeMetricsTile::RenderFiltered(const std::function<bool(const std::string&)>& filter) const {
    using namespace ftxui;
    
    // Count filtered fields
    size_t filtered_count = GetFilteredFieldCount(filter);
    
    std::string header = node_name_ + " (" + 
                        std::to_string(filtered_count) + "/" + 
                        std::to_string(field_descriptors_.size()) + " metrics)";
    
    std::vector<Element> rows;
    rows.push_back(text(header) | bold | color(Color::Cyan));
    rows.push_back(separator());
    
    // Add each field row that matches the filter
    for (const auto& field : field_renders_) {
        if (filter(field.metric_name)) {
            rows.push_back(RenderFieldRow(field));
        }
    }
    
    // If no fields match, show filtered message
    if (filtered_count == 0) {
        rows.push_back(text("(no metrics match filter)") | color(Color::Yellow) | italic);
    }
    
    // Add status line
    rows.push_back(separator());
    auto overall_status = ComputeOverallStatus();
    rows.push_back(text("Status: " + StatusString(overall_status)) 
                  | color(GetStatusColor(overall_status)) | align_right);
    
    return vbox(std::move(rows)) | border | color(Color::Green);
}

ftxui::Element NodeMetricsTile::RenderCollapsed() const {
    using namespace ftxui;
    
    // Get overall status for summary
    auto overall_status = ComputeOverallStatus();
    
    // Create compact summary line: [NodeName] 5/5 metrics - Status
    std::string summary = "[" + node_name_ + "] " + 
                         std::to_string(field_descriptors_.size()) + " metrics";
    
    std::vector<Element> rows;
    rows.push_back(text(summary) | bold | color(Color::Cyan));
    rows.push_back(text("Status: " + StatusString(overall_status)) 
                  | color(GetStatusColor(overall_status)) | align_right);
    
    return vbox(std::move(rows)) | border | color(Color::Yellow);
}
