#include "ui/MetricsTileWindow.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

MetricsTileWindow::MetricsTileWindow(
    const MetricDescriptor& descriptor,
    const nlohmann::json& field_spec)
    : descriptor_(descriptor), field_spec_(field_spec) {
    
    // Extract rendering configuration
    if (field_spec.contains("display_type")) {
        display_type_ = field_spec["display_type"].get<std::string>();
    } else {
        display_type_ = "value";  // Default
    }

    if (field_spec.contains("description")) {
        description_ = field_spec["description"].get<std::string>();
    }

    if (field_spec.contains("precision")) {
        precision_ = field_spec["precision"].get<int>();
    }

    // Extract alert thresholds
    if (field_spec.contains("alert_rule")) {
        const auto& rule = field_spec["alert_rule"];
        
        if (rule.contains("ok") && rule["ok"].is_array()) {
            auto ok_range = rule["ok"];
            ok_min_ = ok_range[0].get<double>();
            ok_max_ = ok_range[1].get<double>();
        }
        
        if (rule.contains("warning") && rule["warning"].is_array()) {
            auto warn_range = rule["warning"];
            warning_min_ = warn_range[0].get<double>();
            warning_max_ = warn_range[1].get<double>();
        }
    }

    std::cerr << "[MetricsTileWindow] Created: " << descriptor_.metric_id 
              << " (" << display_type_ << ")\n";
}

void MetricsTileWindow::UpdateValue(double new_value) {
    current_value_ = new_value;
    
    // Update sparkline history
    history_.push_back(new_value);
    if (history_.size() > MAX_HISTORY) {
        history_.pop_front();
    }
}

std::string MetricsTileWindow::FormatValue(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision_) << value;
    return oss.str();
}

AlertStatus MetricsTileWindow::GetStatus() const {
    if (current_value_ >= ok_min_ && current_value_ <= ok_max_) {
        return AlertStatus::OK;
    } else if (current_value_ >= warning_min_ && current_value_ <= warning_max_) {
        return AlertStatus::WARNING;
    } else {
        return AlertStatus::CRITICAL;
    }
}

ftxui::Color MetricsTileWindow::GetStatusColor() const {
    switch (GetStatus()) {
        case AlertStatus::OK:
            return ftxui::Color::Green;
        case AlertStatus::WARNING:
            return ftxui::Color::Yellow;
        case AlertStatus::CRITICAL:
            return ftxui::Color::Red;
    }
    return ftxui::Color::White;
}

ftxui::Element MetricsTileWindow::RenderValue() const {
    using namespace ftxui;
    
    std::string title = descriptor_.metric_name;
    std::string value_str = FormatValue(current_value_) + " " + descriptor_.unit;
    
    auto status_color = GetStatusColor();
    
    return vbox({
        text(title) | bold | color(Color::Cyan),
        text(value_str) | center | color(status_color) | bold,
    }) | border | color(status_color);
}

ftxui::Element MetricsTileWindow::RenderGauge() const {
    using namespace ftxui;
    
    // Calculate gauge fill percentage
    double range = ok_max_ - ok_min_;
    double normalized = (current_value_ - ok_min_) / (range > 0 ? range : 1.0);
    normalized = std::max(0.0, std::min(1.0, normalized));
    
    int fill_width = static_cast<int>(normalized * 12);  // 12-char wide gauge
    std::string filled(fill_width, '#');
    std::string empty(12 - fill_width, '-');
    std::string gauge = "[" + filled + empty + "]";
    
    std::string title = descriptor_.metric_name;
    std::string percentage = FormatValue(normalized * 100.0) + "%";
    std::string value_text = FormatValue(current_value_) + " / " + 
                            FormatValue(ok_max_) + " " + descriptor_.unit;
    
    auto status_color = GetStatusColor();
    
    return vbox({
        text(title) | bold | color(Color::Cyan),
        text(gauge + " " + percentage) | color(status_color),
        text(value_text) | dim,
    }) | border | color(status_color);
}

ftxui::Element MetricsTileWindow::RenderSparkline() const {
    using namespace ftxui;
    
    std::string title = descriptor_.metric_name;
    std::string current_str = FormatValue(current_value_) + " " + descriptor_.unit;
    
    // Build sparkline visualization using ASCII
    std::string sparkline;
    if (!history_.empty()) {
        // Find min/max for scaling
        double min_val = *std::min_element(history_.begin(), history_.end());
        double max_val = *std::max_element(history_.begin(), history_.end());
        double range = max_val - min_val;
        if (range == 0) range = 1.0;
        
        // Sparkline characters (ASCII): . - + o O *
        const char sparkchars[] = {'.', '-', '+', 'o', 'O', '*'};
        
        for (double val : history_) {
            double normalized = (val - min_val) / range;
            int idx = static_cast<int>(normalized * 5);
            idx = std::max(0, std::min(5, idx));
            sparkline += sparkchars[idx];
        }
    }
    
    auto status_color = GetStatusColor();
    
    return vbox({
        text(title) | bold | color(Color::Cyan),
        text(sparkline.length() > 40 ? sparkline.substr(sparkline.length() - 40) : sparkline) |
            color(status_color),
        text("(" + current_str + ")") | center | color(status_color) | bold,
    }) | border | color(status_color);
}

ftxui::Element MetricsTileWindow::RenderState() const {
    using namespace ftxui;
    
    // State is typically boolean/true-false
    bool is_active = current_value_ > 0.5;  // Threshold at 0.5
    std::string state_text = is_active ? "[*] Active" : "[ ] Inactive";
    std::string title = descriptor_.metric_name;
    
    auto status_color = is_active ? Color::Green : Color::Red;
    
    return vbox({
        text(title) | bold | color(Color::Cyan),
        text(state_text) | center | color(status_color) | bold,
    }) | border | color(status_color);
}

ftxui::Element MetricsTileWindow::Render() const {
    if (display_type_ == "gauge") {
        return RenderGauge();
    } else if (display_type_ == "sparkline") {
        return RenderSparkline();
    } else if (display_type_ == "state") {
        return RenderState();
    } else {
        return RenderValue();  // Default
    }
}

int MetricsTileWindow::GetMinHeightLines() const {
    if (display_type_ == "gauge") {
        return 5;
    } else if (display_type_ == "sparkline") {
        return 7;
    } else if (display_type_ == "state") {
        return 3;
    } else {
        return 3;  // "value"
    }
}
