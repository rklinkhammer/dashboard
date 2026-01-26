#include "ui/MetricsPanel.hpp"
#include "ui/MetricsTileWindow.hpp"
#include "ui/MetricsTilePanel.hpp"
#include "graph/GraphExecutor.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "app/metrics/NodeMetricsSchema.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger_ = log4cxx::Logger::getLogger("ui.MetricsPanel");

MetricsPanel::MetricsPanel(const std::string& title)
    : Window(title), metrics_tile_panel_(std::make_shared<MetricsTilePanel>()) {
    // Add initial placeholder metrics
    metrics_ = {
        {"Throughput", 523.4},
        {"Processing Time", 48.2},
        {"Queue Depth", 12},
        {"Error Count", 0},
        {"Node A Status", 1.0},
        {"Node B Status", 1.0},
        {"Cache Hit Rate", 0.95},
        {"Memory Usage", 0.42},
        {"CPU Usage", 0.38},
    };
    LOG4CXX_TRACE(logger_, "MetricsPanel: Created with " << metrics_.size() << " placeholder metrics");
}

ftxui::Element MetricsPanel::Render() const {
    using namespace ftxui;
    
    // If tab mode is enabled, render with tabs
    if (tab_mode_enabled_) {
        return RenderTabbed();
    }
    
    // If we have tiles from MetricsCapability discovery, render flat grid
    if (metrics_tile_panel_ && metrics_tile_panel_->GetTileCount() > 0) {
        return RenderFlatGrid();
    }
    
    // Otherwise fall back to placeholder rendering
    std::vector<Element> metric_elements;
    
    // Add title
    metric_elements.push_back(
        text(title_) | bold | color(Color::Cyan)
    );
    
    // Add metric rows
    for (size_t i = 0; i < metrics_.size() && i < 10; ++i) {
        const auto& metric = metrics_[i];
        std::string metric_text = metric.name + ": " + std::to_string(metric.value).substr(0, 6);
        metric_elements.push_back(text(metric_text));
    }
    
    // If there are more metrics, add indicator
    if (metrics_.size() > 10) {
        metric_elements.push_back(
            text("... " + std::to_string(metrics_.size() - 10) + " more metrics")
                | dim
        );
    }
    
    return vbox(metric_elements)
        | border
        | color(Color::Green);
}

void MetricsPanel::DiscoverMetricsFromExecutor(std::shared_ptr<app::capabilities::MetricsCapability> capability) {
    if (!capability) {
        LOG4CXX_ERROR(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: capability is null");    
        return;
    }

    try {
         
        if (!capability) {
            LOG4CXX_ERROR(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: No MetricsCapability in executor");
            return;
        }

        // Get all node metrics schemas
        const auto& schemas = capability->GetNodeMetricsSchemas();
        LOG4CXX_TRACE(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: Discovered " << schemas.size() 
                  << " nodes with metrics\n");

        // Create tiles for each metric in each node
        for (const auto& schema : schemas) {
            LOG4CXX_TRACE(logger_, "Processing node: " << schema.node_name);
            
            // Check if metrics_schema has "fields" array
            if (schema.metrics_schema.contains("fields") && 
                schema.metrics_schema["fields"].is_array()) {
                
                const auto& fields = schema.metrics_schema["fields"];
                for (const auto& field : fields) {
                    if (!field.contains("name")) {
                        continue;
                    }

                    std::string metric_name = field["name"];
                    std::string metric_id = schema.node_name + "::" + metric_name;
                    
                    // Create descriptor for this metric
                    MetricDescriptor descriptor;
                    descriptor.node_name = schema.node_name;
                    descriptor.metric_name = metric_name;
                    descriptor.metric_id = metric_id;
                    descriptor.unit = field.contains("unit") ? field["unit"].get<std::string>() : "";
                    
                    // Set default min/max values
                    descriptor.min_value = 0.0;
                    descriptor.max_value = 100.0;
                    
                    // Create tile with descriptor and field spec
                    auto tile = std::make_shared<MetricsTileWindow>(descriptor, field);
                    metrics_tile_panel_->AddTile(tile);
                    
                    LOG4CXX_TRACE(logger_, "MetricsPanel: Created tile for: " << metric_id);
                }
            }
        }
        metrics_tile_panel_->SetMetricsCapability(capability);
        LOG4CXX_TRACE(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: Created " 
                  << metrics_tile_panel_->GetTileCount() << " metric tiles")    ;
        
        // Check if we need to activate tab mode (when tile count > 36, meaning more than 6×6 grid)
        if (metrics_tile_panel_->GetTileCount() > 36) {
            ActivateTabMode();
        }

    } catch (const std::exception& e) {
        LOG4CXX_ERROR(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: Exception: " << e.what());
    }
}

void MetricsPanel::RegisterMetricsCapabilityCallback(std::shared_ptr<app::capabilities::MetricsCapability> capability) {
    if (!capability) {
        LOG4CXX_ERROR(logger_, "MetricsPanel::RegisterMetricsCapabilityCallback: capability is null");    
        return;
    }

    try {
        capability->RegisterMetricsCallback(this);

    } catch (const std::exception& e) {
        LOG4CXX_ERROR(logger_, "MetricsPanel::RegisterMetricsCapabilityCallback: Exception: " << e.what());
    }
}

void MetricsPanel::AddPlaceholderMetric(const std::string& metric_name, double value) {
    metrics_.push_back({metric_name, value});
}

void MetricsPanel::ClearPlaceholders() {
    metrics_.clear();
}

ftxui::Element MetricsPanel::RenderFlatGrid() const {
    using namespace ftxui;
    return metrics_tile_panel_->Render() | border | color(Color::Green);
}

ftxui::Element MetricsPanel::RenderTabbed() const {
    using namespace ftxui;
    return tab_container_.RenderWithTabs() | border | color(Color::Green);
}

void MetricsPanel::ActivateTabMode() {
    LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Transitioning to tabbed mode");
    
    if (!metrics_tile_panel_) {
        LOG4CXX_ERROR(logger_, "MetricsPanel::ActivateTabMode: ERROR: No metrics_tile_panel");
        return;
    }
    
    // Collect all unique node names by iterating known nodes
    // Since we store node_to_tab_index during discovery, we can use those
    // Or we can discover by trying common node names
    std::set<std::string> node_names = {
        "DataValidator", "Transform", "Aggregator", "Filter", "Sink", "Monitor"
    };
    
    // Try to get tiles for each known node name and see which ones have tiles
    std::set<std::string> actual_nodes;
    for (const auto& node_name : node_names) {
        auto tiles = metrics_tile_panel_->GetTilesForNode(node_name);
        if (!tiles.empty()) {
            actual_nodes.insert(node_name);
        }
    }
    
    LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Found " << actual_nodes.size() << " nodes with tiles");
    
    // Create tabs for each node that has tiles
    int tab_index = 0;
    for (const auto& node_name : actual_nodes) {
        tab_container_.CreateTab(node_name);
        node_to_tab_index_[node_name] = tab_index;
        
        // Get all tiles for this node and add them to the tab
        auto tiles = metrics_tile_panel_->GetTilesForNode(node_name);
        for (const auto& tile : tiles) {
            if (tile) {
                tab_container_.AddTileToTab(node_name, tile);
                LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Added tile " << tile->GetMetricId()
                          << " to tab: " << node_name);
            }
        }
        
        LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Created tab " << tab_index 
                  << " for node: " << node_name << " with " << tiles.size() << " tiles");
        tab_index++;
    }
    
    tab_mode_enabled_ = true;
    LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Tab mode activated with " 
              << tab_container_.GetTabCount() << " tabs");
}

int MetricsPanel::FindOrCreateTabForNode(const std::string& node_name) {
    // Check if tab already exists
    auto it = node_to_tab_index_.find(node_name);
    if (it != node_to_tab_index_.end()) {
        return it->second;
    }
    
    // Create new tab for this node
    int new_tab_index = tab_container_.GetTabCount();
    tab_container_.CreateTab(node_name);
    node_to_tab_index_[node_name] = new_tab_index;
    
    LOG4CXX_TRACE(logger_, "MetricsPanel::FindOrCreateTabForNode: Created new tab " << new_tab_index
              << " for node: " << node_name);
    
    return new_tab_index;
}

int MetricsPanel::FindTabForNode(const std::string& node_name) const {
    auto it = node_to_tab_index_.find(node_name);
    if (it != node_to_tab_index_.end()) {
        return it->second;
    }
    return -1;  // Not found
}

void MetricsPanel::OnMetricsEvent(const app::metrics::MetricsEvent &event)
{
    (void)event;   // // Extract metric details from event
    // // event.data contains: {"metric_name": "...", "value": "..."}
    // try
    // {
    //     if (event.data.count("metric_name") && event.data.count("value"))
    //     {
    //         std::string metric_id = event.source + "::" + event.data.at("metric_name");
    //         double value = std::stod(event.data.at("value"));

    //         // Buffer the value for UpdateAllMetrics() to propagate
    //         metrics_tile_panel_->SetLatestValue(metric_id, value);
    //     }
    // }
    // catch (const std::exception &e)
    // {
    //     std::cerr << "[MetricsPanel] Error processing event: " << e.what() << "\n";
    // }
}
