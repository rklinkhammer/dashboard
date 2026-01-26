#pragma once

#include "Window.hpp"
#include "LoggingAppender.hpp"
#include <ftxui/dom/elements.hpp>
#include <deque>
#include <string>
#include <memory>
#include <mutex>

/**
 * LoggingWindow: Enhanced logging display with filtering, search, and log4cxx integration
 * 
 * Features:
 * - Display up to 1000 lines in circular buffer
 * - Filter by log level (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Text search with highlighting
 * - Auto-scroll to latest messages
 * - Color-coded output by level
 * - Integration with log4cxx via custom appender
 */
class LoggingWindow : public Window {
public:
    explicit LoggingWindow(const std::string& title = "Logs");
    ~LoggingWindow() override;

    // Rendering
    ftxui::Element Render() const override;

    // Log management
    void AddLogLine(const std::string& line);
    void AddLogLineWithLevel(const std::string& level, const std::string& message);
    void ClearLogs();
    size_t GetLogCount() const;

    // Filtering by level
    void SetLevelFilter(const std::string& level);
    std::string GetLevelFilter() const { return level_filter_; }
    void CycleLevelFilter();  // TRACE -> DEBUG -> INFO -> WARN -> ERROR -> FATAL -> TRACE

    // Text search
    void SetSearchText(const std::string& text);
    std::string GetSearchText() const { return search_text_; }
    void ClearSearch() { search_text_.clear(); }

    // Scrolling
    void ScrollUp();
    void ScrollDown();
    void ScrollToBottom() { scroll_offset_ = 0; }

    // Log4cxx integration
    std::shared_ptr<LoggingAppender> GetAppender() const { return appender_; }

    // Component event handling (override from ComponentBase)
    bool OnEvent(ftxui::Event event) override;

private:
    // Internal structure for log entries
    struct LogEntry {
        std::string level;
        std::string message;
        std::string raw_line;  // Full formatted line
        
        LogEntry(const std::string& lvl, const std::string& msg, const std::string& raw)
            : level(lvl), message(msg), raw_line(raw) {}
    };

    std::deque<LogEntry> log_entries_;
    mutable std::mutex logs_mutex_;
    size_t scroll_offset_ = 0;
    std::string level_filter_ = "TRACE";  // Show all by default
    std::string search_text_;
    std::shared_ptr<LoggingAppender> appender_;
    
    static constexpr size_t MAX_LINES = 1000;
    
    // Helper methods
    bool PassesLevelFilter(const std::string& level) const;
    bool PassesSearchFilter(const LogEntry& entry) const;
    int GetLevelPriority(const std::string& level) const;
    ftxui::Color GetLevelColor(const std::string& level) const;
    std::vector<LogEntry> GetFilteredLogs() const;
};
