#include "ui/LoggingWindow.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>

LoggingWindow::LoggingWindow(const std::string& title)
    : Window(title) {
    // Add initial placeholder logs
    log_lines_.push_back("[2025-01-24 12:00:00.123] [INFO]  Dashboard starting");
    log_lines_.push_back("[2025-01-24 12:00:00.456] [DEBUG] GraphExecutor created");
    log_lines_.push_back("[2025-01-24 12:00:00.789] [INFO]  Metrics capability available");
    std::cerr << "[LoggingWindow] Created with 3 initial log lines\n";
}

ftxui::Element LoggingWindow::Render() const {
    using namespace ftxui;
    
    std::vector<Element> log_elements;
    
    // Add title
    log_elements.push_back(
        text(title_) | bold | color(Color::Magenta)
    );
    
    // Add log lines
    for (size_t i = 0; i < log_lines_.size() && i < 15; ++i) {
        log_elements.push_back(text(log_lines_[i]) | dim);
    }
    
    // If there are more logs, add indicator
    if (log_lines_.size() > 15) {
        log_elements.push_back(
            text("... " + std::to_string(log_lines_.size() - 15) + " earlier log lines")
                | dim
        );
    }
    
    return vbox(log_elements)
        | border
        | color(Color::Blue);
}

void LoggingWindow::AddLogLine(const std::string& line) {
    log_lines_.push_back(line);
    if (log_lines_.size() > MAX_LINES) {
        log_lines_.pop_front();
    }
}

void LoggingWindow::ClearLogs() {
    log_lines_.clear();
    scroll_offset_ = 0;
}
