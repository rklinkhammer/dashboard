#include "ui/MetricsTilePanel.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <iostream>
#include <algorithm>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("ui.MetricsTilePanel");

MetricsTilePanel::MetricsTilePanel(
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap)
    : metrics_cap_(metrics_cap), total_tile_count_(0) {
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Created (Option 2: Separate Nodes Container)");
}

std::pair<std::string, std::string> MetricsTilePanel::ParseMetricId(const std::string& metric_id) {
    // Format: "NodeName::metric_name"
    size_t pos = metric_id.find("::");
    if (pos == std::string::npos) {
        return {metric_id, ""};  // Fallback if not found
    }
    return {metric_id.substr(0, pos), metric_id.substr(pos + 2)};
}

MetricsTilePanel::Node* MetricsTilePanel::FindOrCreateNode(const std::string& node_name) {
    // Check if node already exists
    auto it = node_index_.find(node_name);
    if (it != node_index_.end()) {
        return &nodes_[it->second];
    }
    
    // Create new node
    Node new_node;
    new_node.name = node_name;
    
    // Store in vector and update index
    node_index_[node_name] = nodes_.size();
    nodes_.push_back(new_node);
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Created new node: " << node_name);
    
    return &nodes_.back();
}

void MetricsTilePanel::AddTile(std::shared_ptr<MetricsTileWindow> tile) {
    if (!tile) return;
    
    const std::string& metric_id = tile->GetMetricId();
    auto [node_name, metric_name] = ParseMetricId(metric_id);
    
    // Find or create node
    Node* node = FindOrCreateNode(node_name);
    
    // Check for duplicates within node
    if (node->metrics.find(metric_name) != node->metrics.end()) {
        LOG4CXX_WARN(logger_, "MetricsTilePanel: Warning: Metric " << metric_id 
                      << " already added in node " << node_name);
        return;
    }
    
    // Add tile to node
    node->metrics[metric_name] = tile;
    total_tile_count_++;
    
    LOG4CXX_TRACE(logger_, "MetricsTilePanel: Added tile: " << metric_id 
                  << " to node " << node_name << " (total: " << total_tile_count_ << ")");
}

void MetricsTilePanel::UpdateAllMetrics() {
    // Per ARCHITECTURE.md:
    // - MetricsPublisher invokes callback with MetricsEvent (metric_name, value in data map)
    // - Callback buffers value via SetLatestValue()
    // - UpdateAllMetrics() (called each frame from Run()) propagates buffered values to tiles
    
    // Copy latest values to tiles (lock-free during update iteration)
    std::map<std::string, double> values_snapshot;
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        values_snapshot = latest_values_;
    }
    
    // Update each tile with its latest value
    for (const auto& [metric_id, value] : values_snapshot) {
        if (auto tile = GetTile(metric_id)) {
            tile->UpdateValue(value);
        }
    }
}

void MetricsTilePanel::SetLatestValue(const std::string& metric_id, double value) {
    std::lock_guard<std::mutex> lock(values_lock_);
    latest_values_[metric_id] = value;
}

std::shared_ptr<MetricsTileWindow> MetricsTilePanel::GetTile(const std::string& metric_id) const {
    // Parse metric_id: "NodeName::metric_name"
    auto [node_name, metric_name] = ParseMetricId(metric_id);
    
    // Find node
    auto node_it = node_index_.find(node_name);
    if (node_it == node_index_.end()) {
        return nullptr;  // Node not found
    }
    
    const Node& node = nodes_[node_it->second];
    
    // Find metric within node
    auto metric_it = node.metrics.find(metric_name);
    if (metric_it != node.metrics.end()) {
        return metric_it->second;
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<MetricsTileWindow>> MetricsTilePanel::GetTilesForNode(
    const std::string& node_name) const {
    
    std::vector<std::shared_ptr<MetricsTileWindow>> result;
    
    auto node_it = node_index_.find(node_name);
    if (node_it == node_index_.end()) {
        return result;  // Node not found
    }
    
    const Node& node = nodes_[node_it->second];
    for (const auto& [metric_name, tile] : node.metrics) {
        result.push_back(tile);
    }
    
    return result;
}

ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    if (nodes_.empty() || total_tile_count_ == 0) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    std::vector<Element> node_sections;
    
    // Render each node as a collapsible/expandable section
    for (const auto& node : nodes_) {
        // Node header with count
        std::string header = "[" + node.name + "] (" + 
                            std::to_string(node.metrics.size()) + " metrics)";
        
        auto node_header = text(header) | bold | color(Color::Cyan) | border;
        node_sections.push_back(node_header);
        
        // Render node's metrics as 3-column grid
        std::vector<Element> grid_rows;
        std::vector<Element> current_row;
        
        for (const auto& [metric_name, tile] : node.metrics) {
            current_row.push_back(tile->Render() | flex);
            
            // When row is full (3 columns) or last tile in node
            if (current_row.size() == 3 || 
                metric_name == node.metrics.rbegin()->first) {
                
                // Pad incomplete rows
                while (current_row.size() < 3) {
                    current_row.push_back(text("") | flex);
                }
                
                // Add row to grid
                grid_rows.push_back(hbox(std::move(current_row)));
                current_row.clear();
            }
        }
        
        // Add grid for this node
        if (!grid_rows.empty()) {
            node_sections.push_back(vbox(std::move(grid_rows)));
        }
        
        // Add separator between nodes (except after last)
        if (node.name != nodes_.back().name) {
            node_sections.push_back(separator());
        }
    }
    
    return vbox(std::move(node_sections)) | border | color(Color::Green);
}

