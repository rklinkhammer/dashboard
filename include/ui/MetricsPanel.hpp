#pragma once

#include "Window.hpp"
#include <ftxui/dom/elements.hpp>
#include <vector>

class MetricsPanel : public Window {
public:
    explicit MetricsPanel(const std::string& title = "Metrics");

    // Rendering
    ftxui::Element Render() const override;

    // Metrics placeholder management
    void AddPlaceholderMetric(const std::string& metric_name, double value);
    void ClearPlaceholders();
    int GetMetricCount() const { return static_cast<int>(metrics_.size()); }

    // Scrolling support
    void ScrollUp() { if (scroll_offset_ > 0) scroll_offset_--; }
    void ScrollDown() { scroll_offset_++; }

private:
    struct PlaceholderMetric {
        std::string name;
        double value;
    };

    std::vector<PlaceholderMetric> metrics_;
    size_t scroll_offset_ = 0;
};
