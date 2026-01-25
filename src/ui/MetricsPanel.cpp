#include "ui/MetricsPanel.hpp"
#include "ui/MetricsTileWindow.hpp"
#include "ui/MetricsTilePanel.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "app/metrics/NodeMetricsSchema.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <sstream>

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
    std::cerr << "[MetricsPanel] Created with " << metrics_.size() << " placeholder metrics\n";
}

ftxui::Element MetricsPanel::Render() const {
    using namespace ftxui;
    
    // If we have tiles from MetricsCapability discovery, render those
    if (metrics_tile_panel_ && metrics_tile_panel_->GetTileCount() > 0) {
        return metrics_tile_panel_->Render() | border | color(Color::Green);
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

void MetricsPanel::DiscoverMetricsFromExecutor(std::shared_ptr<graph::MockGraphExecutor> executor) {
    if (!executor) {
        std::cerr << "[MetricsPanel::DiscoverMetricsFromExecutor] ERROR: executor is null\n";
        return;
    }

    try {
        // Get MetricsCapability from executor
        auto metrics_cap = executor->GetCapability<app::capabilities::MetricsCapability>();
        
        if (!metrics_cap) {
            std::cerr << "[MetricsPanel::DiscoverMetricsFromExecutor] ERROR: No MetricsCapability in executor\n";
            return;
        }

        // Get all node metrics schemas
        auto schemas = metrics_cap->GetNodeMetricsSchemas();
        std::cerr << "[MetricsPanel::DiscoverMetricsFromExecutor] Discovered " << schemas.size() 
                  << " nodes with metrics\n";

        // Create tiles for each metric in each node
        for (const auto& schema : schemas) {
            std::cerr << "[MetricsPanel] Processing node: " << schema.node_name << "\n";
            
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
                    
                    std::cerr << "[MetricsPanel] Created tile for: " << metric_id << "\n";
                }
            }
        }

        metrics_tile_panel_->SetMetricsCapability(metrics_cap);
        std::cerr << "[MetricsPanel::DiscoverMetricsFromExecutor] Created " 
                  << metrics_tile_panel_->GetTileCount() << " metric tiles\n";

    } catch (const std::exception& e) {
        std::cerr << "[MetricsPanel::DiscoverMetricsFromExecutor] Exception: " << e.what() << "\n";
    }
}

void MetricsPanel::RegisterMetricsCapabilityCallback(std::shared_ptr<graph::MockGraphExecutor> executor) {
    if (!executor) {
        std::cerr << "[MetricsPanel::RegisterMetricsCapabilityCallback] ERROR: executor is null\n";
        return;
    }

    try {
        auto metrics_cap = executor->GetCapability<app::capabilities::MetricsCapability>();
        if (!metrics_cap) {
            std::cerr << "[MetricsPanel::RegisterMetricsCapabilityCallback] ERROR: No MetricsCapability\n";
            return;
        }

        // Register callback lambda to update tiles as metrics arrive
        executor->RegisterMetricsCallback([this](const app::metrics::MetricsEvent& event) {
            // Extract metric ID: node_name + "::" + metric_name
            std::string metric_id = event.source + "::" + event.data.at("metric_name");
            
            // Find and update corresponding tile
            if (auto tile = metrics_tile_panel_->GetTile(metric_id)) {
                try {
                    double value = std::stod(event.data.at("value"));
                    tile->UpdateValue(value);
                } catch (...) {
                    // Ignore parse errors
                }
            }
        });

        std::cerr << "[MetricsPanel::RegisterMetricsCapabilityCallback] Callback registered\n";

    } catch (const std::exception& e) {
        std::cerr << "[MetricsPanel::RegisterMetricsCapabilityCallback] Exception: " << e.what() << "\n";
    }
}

void MetricsPanel::AddPlaceholderMetric(const std::string& metric_name, double value) {
    metrics_.push_back({metric_name, value});
}

void MetricsPanel::ClearPlaceholders() {
    metrics_.clear();
}

