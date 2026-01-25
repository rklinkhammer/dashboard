#include "ui/MetricsPanel.hpp"
#include <iostream>

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

void MetricsPanel::AddPlaceholderMetric(const std::string& metric_name, double value) {
    metrics_.push_back({metric_name, value});
}

void MetricsPanel::ClearPlaceholders() {
    metrics_.clear();
}
