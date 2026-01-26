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
    
    // PHASE 2: Update all metrics before rendering
    if (metrics_tile_panel_) {
        metrics_tile_panel_->UpdateAllMetrics();
    }
    
    // If tab mode is enabled, render with tabs
    if (tab_mode_enabled_) {
        return RenderTabbed();
    }
    
    // PHASE 2: If we have consolidated node tiles, render them
    if (metrics_tile_panel_ && metrics_tile_panel_->GetNodeCount() > 0) {
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

        // PHASE 2: Use consolidated AddNodeMetrics() instead of individual AddTile()
        // This creates one NodeMetricsTile per node containing all its metrics
        for (const auto& schema : schemas) {
            LOG4CXX_TRACE(logger_, "Processing node: " << schema.node_name);
            
            // Check if metrics_schema has "fields" array
            if (schema.metrics_schema.contains("fields") && 
                schema.metrics_schema["fields"].is_array()) {
                
                // Build descriptors for all metrics in this node
                std::vector<MetricDescriptor> descriptors;
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
                    
                    descriptors.push_back(descriptor);
                    LOG4CXX_TRACE(logger_, "MetricsPanel: Added descriptor for: " << metric_id);
                }
                
                // PHASE 2: Create consolidated NodeMetricsTile for all metrics in this node
                if (!descriptors.empty()) {
                    metrics_tile_panel_->AddNodeMetrics(schema.node_name, descriptors, schema.metrics_schema);
                    LOG4CXX_TRACE(logger_, "MetricsPanel: Added NodeMetricsTile for node: " << schema.node_name 
                                  << " with " << descriptors.size() << " metrics");
                }
            }
        }
        
        metrics_tile_panel_->SetMetricsCapability(capability);
        LOG4CXX_TRACE(logger_, "MetricsPanel::DiscoverMetricsFromExecutor: Created " 
                  << metrics_tile_panel_->GetNodeCount() << " consolidated node tiles with "
                  << metrics_tile_panel_->GetTotalFieldCount() << " total fields");
        
        // Tab mode not needed for Phase 2 (nodes organized hierarchically)
        // Keep for future enhancement if needed
        // if (metrics_tile_panel_->GetTotalFieldCount() > 36) {
        //     ActivateTabMode();
        // }

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
    
    // Get all nodes that have been registered (Phase 2)
    std::vector<std::string> node_names = metrics_tile_panel_->GetNodeNames();
    
    LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Found " << node_names.size() << " nodes");
    
    // Create tabs for each node
    int tab_index = 0;
    for (const auto& node_name : node_names) {
        tab_container_.CreateTab(node_name);
        node_to_tab_index_[node_name] = tab_index;
        
        // Note: With Phase 2 consolidation, each node is now a single NodeMetricsTile
        // No need to add individual tiles to tabs
        LOG4CXX_TRACE(logger_, "MetricsPanel::ActivateTabMode: Created tab " << tab_index 
                  << " for node: " << node_name);
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
    // PHASE 2: Route metric values to MetricsTilePanel
    // event.data contains: {"metric_name": "...", "value": "..."}
    try
    {
        if (event.data.count("metric_name") && event.data.count("value") && metrics_tile_panel_)
        {
            std::string metric_id = event.source + "::" + event.data.at("metric_name");
            double value = std::stod(event.data.at("value"));

            // PHASE 2: Store value in thread-safe SetLatestValue()
            // UpdateAllMetrics() will route this to the appropriate NodeMetricsTile
            metrics_tile_panel_->SetLatestValue(metric_id, value);
            LOG4CXX_TRACE(logger_, "MetricsPanel::OnMetricsEvent: Set " << metric_id << " = " << value);
        }
    }
    catch (const std::exception &e)
    {
        LOG4CXX_WARN(logger_, "MetricsPanel::OnMetricsEvent: Error processing event: " << e.what());
    }
}
