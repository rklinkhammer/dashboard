#include "ui/LoggingWindow.hpp"
#include <iostream>

LoggingWindow::LoggingWindow(const std::string& title)
    : Window(title) {
    // Add initial placeholder logs
    log_lines_.push_back("[2025-01-24 12:00:00.123] [INFO]  Dashboard starting");
    log_lines_.push_back("[2025-01-24 12:00:00.456] [DEBUG] GraphExecutor created");
    log_lines_.push_back("[2025-01-24 12:00:00.789] [INFO]  Metrics capability available");
    std::cerr << "[LoggingWindow] Created with 3 initial log lines\n";
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
