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
// Option 2: Separate Nodes Container - nodes as first-class concept
class MetricsTilePanel {
public:
    explicit MetricsTilePanel(
        std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap = nullptr);

    // Add a tile to the panel (typically called during Initialize())
    // Automatically groups tile by its NodeName
    void AddTile(std::shared_ptr<MetricsTileWindow> tile);

    // Update all tiles with latest values from MetricsCapability (called each frame)
    void UpdateAllMetrics();

    // Store latest metric value (called from metrics event callback, thread-safe)
    void SetLatestValue(const std::string& metric_id, double value);

    // Render all tiles as FTXUI elements (grouped by NodeName with headers)
    ftxui::Element Render() const;

    // Get tile count (total across all nodes)
    size_t GetTileCount() const { return total_tile_count_; }
    
    // Get number of unique nodes
    size_t GetNodeCount() const { return nodes_.size(); }
    
    // Get tile by metric ID (format: "NodeName::metric_name")
    std::shared_ptr<MetricsTileWindow> GetTile(const std::string& metric_id) const;
    
    // Get all tiles from a specific node
    std::vector<std::shared_ptr<MetricsTileWindow>> GetTilesForNode(const std::string& node_name) const;

    // Set metrics capability (for dynamic updates)
    void SetMetricsCapability(std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap) {
        metrics_cap_ = metrics_cap;
    }

private:
    // Option 2: Node structure with metrics as first-class concept
    struct Node {
        std::string name;
        // metric_name -> tile (e.g., "validation_errors" -> MetricsTileWindow)
        std::map<std::string, std::shared_ptr<MetricsTileWindow>> metrics;
    };
    
    std::vector<Node> nodes_;                                    // Ordered by discovery
    std::map<std::string, size_t> node_index_;                   // NodeName -> node vector index
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap_;
    
    // Latest values storage (thread-safe, updated from metrics callback)
    std::map<std::string, double> latest_values_;                // metric_id -> value
    mutable std::mutex values_lock_;
    
    // Total tile count (for compatibility with GetTileCount())
    size_t total_tile_count_ = 0;
    
    // Private helper to find or create node by name
    Node* FindOrCreateNode(const std::string& node_name);
    
    // Helper to parse metric_id into node_name and metric_name
    static std::pair<std::string, std::string> ParseMetricId(const std::string& metric_id);
};

