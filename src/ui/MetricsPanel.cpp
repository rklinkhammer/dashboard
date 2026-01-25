#include "ui/MetricsPanel.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>

MetricsPanel::MetricsPanel(const std::string& title)
    : Window(title) {
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

void MetricsPanel::AddPlaceholderMetric(const std::string& metric_name, double value) {
    metrics_.push_back({metric_name, value});
}

void MetricsPanel::ClearPlaceholders() {
    metrics_.clear();
}
