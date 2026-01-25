#include "ui/MetricsTilePanel.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <iostream>

MetricsTilePanel::MetricsTilePanel(
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap)
    : metrics_cap_(metrics_cap) {
    std::cerr << "[MetricsTilePanel] Created\n";
}

void MetricsTilePanel::AddTile(std::shared_ptr<MetricsTileWindow> tile) {
    if (!tile) return;
    
    const std::string& metric_id = tile->GetMetricId();
    
    // Check for duplicates
    if (tile_index_.find(metric_id) != tile_index_.end()) {
        std::cerr << "[MetricsTilePanel] Warning: Metric " << metric_id 
                  << " already added\n";
        return;
    }
    
    tile_index_[metric_id] = tiles_.size();
    tiles_.push_back(tile);
    
    std::cerr << "[MetricsTilePanel] Added tile: " << metric_id << " (total: " 
              << tiles_.size() << ")\n";
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

std::shared_ptr<MetricsTileWindow> MetricsTilePanel::GetTile(
    const std::string& metric_id) const {
    
    auto it = tile_index_.find(metric_id);
    if (it != tile_index_.end()) {
        return tiles_[it->second];
    }
    return nullptr;
}

ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    if (tiles_.empty()) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    // For now, arrange tiles in a simple vbox
    // In Phase 2.5+, could implement grid/horizontal/tabbed layouts
    std::vector<Element> tile_elements;
    
    for (const auto& tile : tiles_) {
        tile_elements.push_back(tile->Render());
    }
    
    return vbox(tile_elements);
}
