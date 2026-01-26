#pragma once

#include "Window.hpp"
#include "CommandRegistry.hpp"
#include <ftxui/dom/elements.hpp>
#include <deque>
#include <string>
#include <memory>
#include <sstream>

class CommandWindow : public Window {
public:
    explicit CommandWindow(const std::string& title = "Command");

    // Rendering
    ftxui::Element Render() const override;

    // Command/output management
    void AddOutputLine(const std::string& line);
    void ClearOutput();
    std::string GetInputBuffer() const { return input_buffer_; }
    size_t GetOutputCount() const { return output_lines_.size(); }

    // Command handling
    void SetCommandRegistry(std::shared_ptr<CommandRegistry> registry) { registry_ = registry; }
    void ExecuteInputCommand();
    void HandleKeyInput(int key);

    // Focus management
    void SetFocus(bool focused) { has_focus_ = focused; }
    bool HasFocus() const { return has_focus_; }

    // Scrolling
    void ScrollUp();
    void ScrollDown();
    void ScrollToBottom() { scroll_offset_ = 0; }

    // Component event handling (override from ComponentBase)
    bool OnEvent(ftxui::Event event) override;

    // Command history
    void ClearHistory();
    std::vector<std::string> GetHistory() const;

private:
    std::string input_buffer_;
    std::deque<std::string> output_lines_;
    std::deque<std::string> command_history_;
    size_t history_index_ = 0;
    std::shared_ptr<CommandRegistry> registry_;
    bool has_focus_ = true;  // CommandWindow is focused by default
    size_t scroll_offset_ = 0;  // For scrolling output history
    
    static constexpr size_t MAX_OUTPUT = 100;
    static constexpr size_t MAX_HISTORY = 50;

    // Helper to parse command from input
    void ParseAndExecuteCommand();
};
