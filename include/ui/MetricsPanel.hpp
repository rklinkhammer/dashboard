#pragma once

#include "Window.hpp"
#include "MetricsTilePanel.hpp"
#include <ftxui/dom/elements.hpp>
#include <vector>
#include <memory>

// Forward declarations
namespace graph {
class MockGraphExecutor;
}

class MetricsPanel : public Window {
public:
    explicit MetricsPanel(const std::string& title = "Metrics");

    // Rendering
    ftxui::Element Render() const override;

    // Metrics discovery and management
    void DiscoverMetricsFromExecutor(std::shared_ptr<graph::MockGraphExecutor> executor);
    void RegisterMetricsCapabilityCallback(std::shared_ptr<graph::MockGraphExecutor> executor);

    // Metrics placeholder management (legacy)
    void AddPlaceholderMetric(const std::string& metric_name, double value);
    void ClearPlaceholders();
    int GetMetricCount() const { 
        // Return tile count if we have discovered metrics, otherwise placeholder count
        size_t tile_count = metrics_tile_panel_ ? metrics_tile_panel_->GetTileCount() : 0;
        return tile_count > 0 ? static_cast<int>(tile_count) : static_cast<int>(metrics_.size()); 
    }

    // Scrolling support
    void ScrollUp() { if (scroll_offset_ > 0) scroll_offset_--; }
    void ScrollDown() { scroll_offset_++; }

    // Accessors
    std::shared_ptr<MetricsTilePanel> GetMetricsTilePanel() const { return metrics_tile_panel_; }

private:
    struct PlaceholderMetric {
        std::string name;
        double value;
    };

    std::vector<PlaceholderMetric> metrics_;
    size_t scroll_offset_ = 0;
    
    // New Phase 2 metrics tile infrastructure
    std::shared_ptr<MetricsTilePanel> metrics_tile_panel_;
};
