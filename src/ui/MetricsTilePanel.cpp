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
    if (!metrics_cap_) return;
    
    // For now, this is a placeholder
    // In real implementation, would query MetricsCapability for latest values
    // and call UpdateValue() on each tile
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
