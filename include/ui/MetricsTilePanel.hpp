#pragma once

#include <memory>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <ftxui/dom/elements.hpp>
#include "ui/MetricsTileWindow.hpp"
#include "ui/NodeMetricsTile.hpp"

// Forward declaration
namespace app::capabilities {
class MetricsCapability;
}

// Container for metric tiles - manages tile lifecycle and updates
// Uses consolidated NodeMetricsTile for better organization
// One tile per node, containing all metrics from that node's schema
class MetricsTilePanel {
public:
    explicit MetricsTilePanel(
        std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap = nullptr);

    // Add all metrics from a node at once as consolidated NodeMetricsTile
    void AddNodeMetrics(
        const std::string& node_name,
        const std::vector<MetricDescriptor>& descriptors,
        const nlohmann::json& metrics_schema);

    // Update all tiles with latest values from MetricsCapability (called each frame)
    void UpdateAllMetrics();

    // Store latest metric value (called from metrics event callback, thread-safe)
    void SetLatestValue(const std::string& metric_id, double value);

    // Render all tiles as FTXUI elements
    ftxui::Element Render() const;

    // Get tile count (for backward compatibility, now returns node count)
    size_t GetTileCount() const { return node_tiles_.size(); }
    
    // Get number of unique nodes
    size_t GetNodeCount() const { return node_tiles_.size(); }
    
    // Get total field count across all nodes
    size_t GetTotalFieldCount() const { return total_field_count_; }
    
    // Get consolidated tile for a node
    std::shared_ptr<NodeMetricsTile> GetNodeTile(const std::string& node_name) const;
    
    // Get all node names
    std::vector<std::string> GetNodeNames() const;
    
    // ========== Filtering & Search ==========
    
    // Set global search filter (searches all metrics across all nodes)
    void SetGlobalSearchFilter(const std::string& filter);
    
    // Get global search filter
    const std::string& GetGlobalSearchFilter() const { return global_search_filter_; }
    
    // Set per-node filter pattern (regex-like matching for metric names in this node)
    void SetNodeFilter(const std::string& node_name, const std::string& filter);
    
    // Get per-node filter
    const std::string& GetNodeFilter(const std::string& node_name) const;
    
    // Clear all filters
    void ClearAllFilters();
    
    // Check if a metric matches current filters
    bool MatchesFilters(const std::string& node_name, const std::string& metric_name) const;
    
    // ========== Collapse/Expand State ==========
    
    // Toggle collapse state for a node
    void ToggleNodeCollapsed(const std::string& node_name);
    
    // Set collapse state for a node
    void SetNodeCollapsed(const std::string& node_name, bool collapsed);
    
    // Check if a node is collapsed
    bool IsNodeCollapsed(const std::string& node_name) const;
    
    // Expand all nodes
    void ExpandAll();
    
    // Collapse all nodes
    void CollapseAll();
    
    // ========== Sorting & Pinning ==========
    
    // Toggle pin state for a metric (pins it to top of node)
    void TogglePinnedMetric(const std::string& node_name, const std::string& metric_name);
    
    // Check if a metric is pinned
    bool IsPinnedMetric(const std::string& node_name, const std::string& metric_name) const;
    
    // Clear all pinned metrics
    void ClearAllPinned();
    
    // Set sorting mode: "name", "value", "status", "pinned"
    void SetSortingMode(const std::string& mode);
    
    // Get current sorting mode
    const std::string& GetSortingMode() const { return sorting_mode_; }
    
    // ========== Export ==========
    
    // Export metrics to JSON format
    std::string ExportToJSON() const;
    
    // Export metrics to CSV format (comma-separated values)
    std::string ExportToCSV() const;

    // Set metrics capability (for dynamic updates)
    void SetMetricsCapability(std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap) {
        metrics_cap_ = metrics_cap;
    }

private:
    // Consolidated storage using NodeMetricsTile
    std::map<std::string, std::shared_ptr<NodeMetricsTile>> node_tiles_;
    
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap_;
    
    // Latest values storage (thread-safe, updated from metrics callback)
    std::map<std::string, double> latest_values_;                // metric_id -> value
    mutable std::mutex values_lock_;
    
    // Track field counts for metrics
    size_t total_field_count_ = 0;
    
    // Filter state
    std::string global_search_filter_;                            // Global search across all metrics
    std::map<std::string, std::string> node_filters_;             // Per-node metric filters
    
    // Collapse/expand state
    std::map<std::string, bool> node_collapsed_;                  // node_name -> is_collapsed
    
    // Pinning state: node_name -> set of pinned metric names
    std::map<std::string, std::set<std::string>> pinned_metrics_;
    
    // Sorting configuration
    std::string sorting_mode_ = "name";                           // Default sort by name
    
    // Helper to check if text matches filter pattern (case-insensitive substring)
    static bool MatchesPattern(const std::string& text, const std::string& pattern);
};
