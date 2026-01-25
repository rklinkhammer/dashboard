#pragma once

#include "Window.hpp"
#include <deque>
#include <string>

class CommandWindow : public Window {
public:
    explicit CommandWindow(const std::string& title = "Command");

    // Command/output management
    void AddOutputLine(const std::string& line);
    void ClearOutput();
    std::string GetInputBuffer() const { return input_buffer_; }
    size_t GetOutputCount() const { return output_lines_.size(); }

private:
    std::string input_buffer_;
    std::deque<std::string> output_lines_;
    static constexpr size_t MAX_OUTPUT = 100;
};
