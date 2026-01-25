#pragma once

#include "Window.hpp"
#include <ftxui/dom/elements.hpp>
#include <deque>
#include <string>

class LoggingWindow : public Window {
public:
    explicit LoggingWindow(const std::string& title = "Logs");

    // Rendering
    ftxui::Element Render() const override;

    // Log management
    void AddLogLine(const std::string& line);
    void ClearLogs();
    size_t GetLogCount() const { return log_lines_.size(); }

    // Scrolling
    void ScrollUp() { if (scroll_offset_ > 0) scroll_offset_--; }
    void ScrollDown() { scroll_offset_++; }

private:
    std::deque<std::string> log_lines_;
    size_t scroll_offset_ = 0;
    static constexpr size_t MAX_LINES = 500;
};
