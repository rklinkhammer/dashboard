#include "ui/MetricsTilePanel.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <nlohmann/json.hpp>
#include <log4cxx/logger.h>

using json = nlohmann::json;

static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("ui.MetricsTilePanel");

MetricsTilePanel::MetricsTilePanel(
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap)
    : metrics_cap_(metrics_cap), total_field_count_(0) {
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Created with NodeMetricsTile consolidation");
}

void MetricsTilePanel::AddNodeMetrics(
    const std::string& node_name,
    const std::vector<MetricDescriptor>& descriptors,
    const nlohmann::json& metrics_schema) {
    
    // Create consolidated NodeMetricsTile for all fields in this node
    auto node_tile = std::make_shared<NodeMetricsTile>(
        node_name, descriptors, metrics_schema);
    
    node_tiles_[node_name] = node_tile;
    total_field_count_ += descriptors.size();
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Added NodeMetricsTile: " << node_name 
                  << " with " << descriptors.size() << " fields");
}

void MetricsTilePanel::UpdateAllMetrics() {
    // Get snapshot of latest values (thread-safe)
    std::map<std::string, double> values_snapshot;
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        values_snapshot = latest_values_;
    }
    
    // Route each value to its corresponding NodeMetricsTile
    for (const auto& [metric_id, value] : values_snapshot) {
        // Parse metric_id: "NodeName::metric_name"
        size_t pos = metric_id.find("::");
        if (pos == std::string::npos) continue;
        
        std::string node_name = metric_id.substr(0, pos);
        std::string metric_name = metric_id.substr(pos + 2);
        
        auto it = node_tiles_.find(node_name);
        if (it != node_tiles_.end()) {
            it->second->UpdateMetricValue(metric_name, value);
        }
    }
}

void MetricsTilePanel::SetLatestValue(const std::string& metric_id, double value) {
    std::lock_guard<std::mutex> lock(values_lock_);
    latest_values_[metric_id] = value;
}

std::shared_ptr<NodeMetricsTile> MetricsTilePanel::GetNodeTile(const std::string& node_name) const {
    auto it = node_tiles_.find(node_name);
    if (it != node_tiles_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> MetricsTilePanel::GetNodeNames() const {
    std::vector<std::string> names;
    for (const auto& [node_name, _] : node_tiles_) {
        names.push_back(node_name);
    }
    return names;
}

// ========== Filtering & Search Implementation ==========

bool MetricsTilePanel::MatchesPattern(const std::string& text, const std::string& pattern) {
    // Case-insensitive substring matching
    if (pattern.empty()) {
        return true;  // Empty filter matches everything
    }
    
    std::string lower_text = text;
    std::string lower_pattern = pattern;
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), ::tolower);
    
    return lower_text.find(lower_pattern) != std::string::npos;
}

void MetricsTilePanel::SetGlobalSearchFilter(const std::string& filter) {
    global_search_filter_ = filter;
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Set global search filter: " << filter);
}

void MetricsTilePanel::SetNodeFilter(const std::string& node_name, const std::string& filter) {
    if (filter.empty()) {
        node_filters_.erase(node_name);
    } else {
        node_filters_[node_name] = filter;
    }
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Set filter for node " << node_name << ": " << filter);
}

const std::string& MetricsTilePanel::GetNodeFilter(const std::string& node_name) const {
    static const std::string empty_filter;
    auto it = node_filters_.find(node_name);
    return it != node_filters_.end() ? it->second : empty_filter;
}

void MetricsTilePanel::ClearAllFilters() {
    global_search_filter_.clear();
    node_filters_.clear();
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Cleared all filters");
}

bool MetricsTilePanel::MatchesFilters(const std::string& node_name, const std::string& metric_name) const {
    // Check global search filter
    if (!global_search_filter_.empty()) {
        // For global search, check both node name and metric name
        if (!MatchesPattern(node_name, global_search_filter_) &&
            !MatchesPattern(metric_name, global_search_filter_)) {
            return false;
        }
    }
    
    // Check node-specific filter
    auto node_it = node_filters_.find(node_name);
    if (node_it != node_filters_.end()) {
        if (!MatchesPattern(metric_name, node_it->second)) {
            return false;
        }
    }
    
    return true;  // Passes all filters
}

// ========== Collapse/Expand Implementation ==========

void MetricsTilePanel::ToggleNodeCollapsed(const std::string& node_name) {
    auto it = node_collapsed_.find(node_name);
    if (it != node_collapsed_.end()) {
        it->second = !it->second;
    } else {
        node_collapsed_[node_name] = true;  // Default to collapsed if not found
    }
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Toggled collapse state for node " << node_name 
                  << " to " << (node_collapsed_[node_name] ? "collapsed" : "expanded"));
}

void MetricsTilePanel::SetNodeCollapsed(const std::string& node_name, bool collapsed) {
    node_collapsed_[node_name] = collapsed;
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Set node " << node_name 
                  << " to " << (collapsed ? "collapsed" : "expanded"));
}

bool MetricsTilePanel::IsNodeCollapsed(const std::string& node_name) const {
    auto it = node_collapsed_.find(node_name);
    return it != node_collapsed_.end() && it->second;
}

void MetricsTilePanel::ExpandAll() {
    for (const auto& [node_name, _] : node_tiles_) {
        node_collapsed_[node_name] = false;
    }
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Expanded all nodes");
}

void MetricsTilePanel::CollapseAll() {
    for (const auto& [node_name, _] : node_tiles_) {
        node_collapsed_[node_name] = true;
    }
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Collapsed all nodes");
}

ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    // Return empty state if no metrics
    if (node_tiles_.empty()) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    std::vector<Element> node_sections;
    
    // Check if any filters are active
    bool has_filters = !global_search_filter_.empty() || !node_filters_.empty();
    
    // Render each consolidated node tile
    for (const auto& [node_name, node_tile] : node_tiles_) {
        ftxui::Element node_element;
        
        // Check if node is collapsed
        if (IsNodeCollapsed(node_name)) {
            // Render collapsed summary
            node_element = node_tile->RenderCollapsed();
        } else if (has_filters) {
            // Create filter predicate for this node
            auto filter = [this, &node_name](const std::string& metric_name) {
                return this->MatchesFilters(node_name, metric_name);
            };
            
            node_element = node_tile->RenderFiltered(filter);
        } else {
            // No filters, render normally
            node_element = node_tile->Render();
        }
        
        node_sections.push_back(node_element);
        
        // Add separator between nodes (except after last)
        if (node_name != node_tiles_.rbegin()->first) {
            node_sections.push_back(separator());
        }
    }
    
    // If filters are active, show filter status in title
    if (has_filters) {
        std::string filter_info = "Filters Active";
        if (!global_search_filter_.empty()) {
            filter_info += " [Global: \"" + global_search_filter_ + "\"]";
        }
        
        std::vector<Element> with_filter_header;
        with_filter_header.push_back(
            text(filter_info) | color(Color::Magenta) | bold
        );
        with_filter_header.push_back(separator());
        
        for (auto& elem : node_sections) {
            with_filter_header.push_back(std::move(elem));
        }
        
        return vbox(std::move(with_filter_header)) | border | color(Color::Green);
    }
    
    return vbox(std::move(node_sections)) | border | color(Color::Green);
}

// ========== Sorting & Pinning Implementation ==========

void MetricsTilePanel::TogglePinnedMetric(const std::string& node_name, const std::string& metric_name) {
    auto& pinned = pinned_metrics_[node_name];
    
    auto it = pinned.find(metric_name);
    if (it != pinned.end()) {
        pinned.erase(it);
    } else {
        pinned.insert(metric_name);
    }
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Toggled pin state for " << node_name 
                  << "::" << metric_name);
}

bool MetricsTilePanel::IsPinnedMetric(const std::string& node_name, const std::string& metric_name) const {
    auto it = pinned_metrics_.find(node_name);
    if (it == pinned_metrics_.end()) {
        return false;
    }
    return it->second.find(metric_name) != it->second.end();
}

void MetricsTilePanel::ClearAllPinned() {
    pinned_metrics_.clear();
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Cleared all pinned metrics");
}

void MetricsTilePanel::SetSortingMode(const std::string& mode) {
    if (mode == "name" || mode == "value" || mode == "status" || mode == "pinned") {
        sorting_mode_ = mode;
        LOG4CXX_TRACE(logger_, "MetricsTilePanel: Set sorting mode to " << mode);
    } else {
        LOG4CXX_WARN(logger_, "MetricsTilePanel: Invalid sorting mode: " << mode);
    }
}

// ========== Export Implementation ==========

std::string MetricsTilePanel::ExportToJSON() const {
    json export_data;
    export_data["timestamp"] = std::time(nullptr);
    export_data["version"] = "1.0";
    
    json nodes_array = json::array();
    
    for (const auto& [node_name, node_tile] : node_tiles_) {
        json node_obj;
        node_obj["node_name"] = node_name;
        node_obj["field_count"] = node_tile->GetFieldCount();
        node_obj["status"] = static_cast<int>(node_tile->GetStatus());
        node_obj["collapsed"] = IsNodeCollapsed(node_name);
        
        json metrics_array = json::array();
        // Export metric values from latest_values_
        {
            std::lock_guard<std::mutex> lock(values_lock_);
            for (const auto& [metric_id, value] : latest_values_) {
                // Check if this metric belongs to this node
                if (metric_id.find(node_name + "::") == 0) {
                    json metric_obj;
                    metric_obj["metric_id"] = metric_id;
                    metric_obj["value"] = value;
                    metric_obj["pinned"] = IsPinnedMetric(node_name, metric_id.substr(node_name.length() + 2));
                    metrics_array.push_back(metric_obj);
                }
            }
        }
        node_obj["metrics"] = metrics_array;
        nodes_array.push_back(node_obj);
    }
    
    export_data["nodes"] = nodes_array;
    export_data["filter"] = global_search_filter_;
    export_data["sorting_mode"] = sorting_mode_;
    
    return export_data.dump(2);
}

std::string MetricsTilePanel::ExportToCSV() const {
    std::ostringstream csv;
    
    // CSV Header
    csv << "Timestamp,Node,MetricID,Value,Pinned,Collapsed,Status\n";
    
    auto now = std::time(nullptr);
    
    for (const auto& [node_name, node_tile] : node_tiles_) {
        bool is_collapsed = IsNodeCollapsed(node_name);
        int status = static_cast<int>(node_tile->GetStatus());
        
        // Get values for this node
        {
            std::lock_guard<std::mutex> lock(values_lock_);
            for (const auto& [metric_id, value] : latest_values_) {
                if (metric_id.find(node_name + "::") == 0) {
                    std::string metric_name = metric_id.substr(node_name.length() + 2);
                    bool is_pinned = IsPinnedMetric(node_name, metric_name);
                    
                    csv << now << ","
                        << node_name << ","
                        << metric_id << ","
                        << std::fixed << std::setprecision(4) << value << ","
                        << (is_pinned ? "yes" : "no") << ","
                        << (is_collapsed ? "yes" : "no") << ","
                        << status << "\n";
                }
            }
        }
    }
    
    return csv.str();
}

