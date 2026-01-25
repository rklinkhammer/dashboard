#pragma once

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <ftxui/dom/elements.hpp>
#include "ui/MetricsTileWindow.hpp"

// Forward declaration
namespace app::capabilities {
class MetricsCapability;
}

// Container for metric tiles - manages tile lifecycle and updates
class MetricsTilePanel {
public:
    explicit MetricsTilePanel(
        std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap = nullptr);

    // Add a tile to the panel (typically called during Initialize())
    void AddTile(std::shared_ptr<MetricsTileWindow> tile);

    // Update all tiles with latest values from MetricsCapability (called each frame)
    void UpdateAllMetrics();

    // Store latest metric value (called from metrics event callback, thread-safe)
    void SetLatestValue(const std::string& metric_id, double value);

    // Render all tiles as FTXUI elements
    ftxui::Element Render() const;

    // Get tile count
    size_t GetTileCount() const { return tiles_.size(); }
    
    // Get tile by metric ID
    std::shared_ptr<MetricsTileWindow> GetTile(const std::string& metric_id) const;

    // Set metrics capability (for dynamic updates)
    void SetMetricsCapability(std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap) {
        metrics_cap_ = metrics_cap;
    }

private:
    std::vector<std::shared_ptr<MetricsTileWindow>> tiles_;
    std::map<std::string, size_t> tile_index_;  // metric_id → tile vector index
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap_;
    
    // Latest values storage (thread-safe, updated from metrics callback)
    std::map<std::string, double> latest_values_;
    mutable std::mutex values_lock_;
};
