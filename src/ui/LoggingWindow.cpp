#include "ui/LoggingWindow.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

LoggingWindow::LoggingWindow(const std::string& title)
    : Window(title) {
    // Create the log4cxx appender with callback
    appender_ = NewLoggingAppender([this](const std::string& level, const std::string& message) {
        this->AddLogLineWithLevel(level, message);
    });

    // Add initial placeholder logs
    AddLogLineWithLevel("INFO", "Dashboard starting");
    AddLogLineWithLevel("DEBUG", "GraphExecutor created");
    AddLogLineWithLevel("INFO", "Metrics capability available");
    
    std::cerr << "[LoggingWindow] Created with logging appender and 3 initial log lines\n";
}

LoggingWindow::~LoggingWindow() {
    std::lock_guard<std::mutex> lock(logs_mutex_);
    log_entries_.clear();
}

ftxui::Element LoggingWindow::Render() const {
    using namespace ftxui;
    
    std::lock_guard<std::mutex> lock(logs_mutex_);
    std::vector<Element> log_elements;

    // Add title with filter info
    std::string title_text = title_;
    if (!search_text_.empty()) {
        title_text += " [Filter: " + search_text_ + "]";
    }
    if (level_filter_ != "TRACE") {
        title_text += " [Level: " + level_filter_ + "+]";
    }
    log_elements.push_back(
        text(title_text) | bold | color(Color::Magenta)
    );

    // Get filtered logs
    auto filtered = GetFilteredLogs();

    // Display recent logs (scrollable)
    size_t display_count = 0;
    const size_t max_display = 15;
    
    for (size_t i = 0; i < filtered.size() && display_count < max_display; ++i) {
        const auto& entry = filtered[i];
        ftxui::Color log_color = GetLevelColor(entry.level);
        
        std::string display_line = "[" + entry.level + "] " + entry.message;
        log_elements.push_back(
            text(display_line) | ftxui::color(log_color)
        );
        display_count++;
    }

    // Add log count summary
    std::string summary = std::to_string(filtered.size()) + " matching / " +
                         std::to_string(log_entries_.size()) + " total";
    log_elements.push_back(separator());
    log_elements.push_back(
        text(summary) | dim | italic
    );

    return vbox(log_elements)
        | border
        | color(Color::Blue);
}

void LoggingWindow::AddLogLine(const std::string& line) {
    // Parse line to extract level if present
    std::string level = "INFO";
    std::string message = line;
    
    // Check for [LEVEL] pattern at start
    if (line.size() > 2 && line[0] == '[') {
        size_t close_bracket = line.find(']');
        if (close_bracket != std::string::npos) {
            std::string potential_level = line.substr(1, close_bracket - 1);
            if (potential_level == "TRACE" || potential_level == "DEBUG" ||
                potential_level == "INFO" || potential_level == "WARN" ||
                potential_level == "ERROR" || potential_level == "FATAL") {
                level = potential_level;
                message = line.substr(close_bracket + 1);
                // Trim leading space
                if (!message.empty() && message[0] == ' ') {
                    message = message.substr(1);
                }
            }
        }
    }

    AddLogLineWithLevel(level, message);
}

void LoggingWindow::AddLogLineWithLevel(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logs_mutex_);
    
    // Create formatted log line
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    std::string raw_line = "[" + oss.str() + "] [" + level + "] " + message;

    // Add entry
    log_entries_.emplace_back(level, message, raw_line);

    // Maintain circular buffer
    if (log_entries_.size() > MAX_LINES) {
        log_entries_.pop_front();
    }
}

void LoggingWindow::ClearLogs() {
    std::lock_guard<std::mutex> lock(logs_mutex_);
    log_entries_.clear();
}

size_t LoggingWindow::GetLogCount() const {
    std::lock_guard<std::mutex> lock(logs_mutex_);
    return log_entries_.size();
}

void LoggingWindow::SetLevelFilter(const std::string& level) {
    std::string upper_level = level;
    std::transform(upper_level.begin(), upper_level.end(), upper_level.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    if (upper_level == "TRACE" || upper_level == "DEBUG" || upper_level == "INFO" ||
        upper_level == "WARN" || upper_level == "ERROR" || upper_level == "FATAL") {
        level_filter_ = upper_level;
    }
}

void LoggingWindow::CycleLevelFilter() {
    if (level_filter_ == "TRACE") level_filter_ = "DEBUG";
    else if (level_filter_ == "DEBUG") level_filter_ = "INFO";
    else if (level_filter_ == "INFO") level_filter_ = "WARN";
    else if (level_filter_ == "WARN") level_filter_ = "ERROR";
    else if (level_filter_ == "ERROR") level_filter_ = "FATAL";
    else level_filter_ = "TRACE";
}

void LoggingWindow::SetSearchText(const std::string& text) {
    search_text_ = text;
    std::transform(search_text_.begin(), search_text_.end(), search_text_.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

void LoggingWindow::ScrollUp() {
    auto filtered = GetFilteredLogs();
    if (scroll_offset_ < filtered.size() - 1) {
        scroll_offset_++;
    }
}

void LoggingWindow::ScrollDown() {
    if (scroll_offset_ > 0) {
        scroll_offset_--;
    }
}

bool LoggingWindow::PassesLevelFilter(const std::string& level) const {
    int entry_priority = GetLevelPriority(level);
    int filter_priority = GetLevelPriority(level_filter_);
    return entry_priority >= filter_priority;
}

bool LoggingWindow::PassesSearchFilter(const LogEntry& entry) const {
    if (search_text_.empty()) {
        return true;
    }

    std::string lower_message = entry.message;
    std::transform(lower_message.begin(), lower_message.end(), lower_message.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    return lower_message.find(search_text_) != std::string::npos;
}

int LoggingWindow::GetLevelPriority(const std::string& level) const {
    if (level == "TRACE") return 0;
    if (level == "DEBUG") return 1;
    if (level == "INFO") return 2;
    if (level == "WARN") return 3;
    if (level == "ERROR") return 4;
    if (level == "FATAL") return 5;
    return 2;  // Default to INFO
}

ftxui::Color LoggingWindow::GetLevelColor(const std::string& level) const {
    if (level == "TRACE") return ftxui::Color::GrayDark;
    if (level == "DEBUG") return ftxui::Color::Blue;
    if (level == "INFO") return ftxui::Color::Green;
    if (level == "WARN") return ftxui::Color::Yellow;
    if (level == "ERROR") return ftxui::Color::Red;
    if (level == "FATAL") return ftxui::Color::Red;
    return ftxui::Color::White;
}

std::vector<LoggingWindow::LogEntry> LoggingWindow::GetFilteredLogs() const {
    std::vector<LogEntry> result;
    
    for (const auto& entry : log_entries_) {
        if (PassesLevelFilter(entry.level) && PassesSearchFilter(entry)) {
            result.push_back(entry);
        }
    }

    // Reverse to show newest first
    std::reverse(result.begin(), result.end());
    return result;
}
