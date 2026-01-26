#include "ui/CommandWindow.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <sstream>

CommandWindow::CommandWindow(const std::string& title)
    : Window(title) {
    // Add initial prompt message
    output_lines_.push_back("Type 'help' for available commands");
    output_lines_.push_back("Type 'q' to quit");
    std::cerr << "[CommandWindow] Created with command prompt\n";
}

ftxui::Element CommandWindow::Render() const {
    using namespace ftxui;
    
    std::vector<Element> command_elements;
    
    // Add title
    command_elements.push_back(
        text(title_) | bold | color(Color::Yellow)
    );
    
    // Add input prompt - simplified, just the input line
    // Command output will go to logging window instead
    command_elements.push_back(
        text("> " + input_buffer_ + "_") | color(Color::Green) | bold
    );
    
    return vbox(command_elements)
        | border
        | color(Color::Yellow);
}

void CommandWindow::AddOutputLine(const std::string& line) {
    output_lines_.push_back(line);
    if (output_lines_.size() > MAX_OUTPUT) {
        output_lines_.pop_front();
    }
}

void CommandWindow::ClearOutput() {
    output_lines_.clear();
}

void CommandWindow::ExecuteInputCommand() {
    if (input_buffer_.empty()) {
        return;
    }
    
    // Add command to output for visibility
    AddOutputLine("> " + input_buffer_);
    
    // Save to history
    command_history_.push_back(input_buffer_);
    if (command_history_.size() > MAX_HISTORY) {
        command_history_.pop_front();
    }
    history_index_ = command_history_.size();
    
    // Parse and execute
    ParseAndExecuteCommand();
    
    // Clear input buffer
    input_buffer_.clear();
}

void CommandWindow::ParseAndExecuteCommand() {
    // Split input into command and arguments
    std::istringstream iss(input_buffer_);
    std::string command_name;
    iss >> command_name;
    
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    // Check for built-in commands first
    if (command_name == "clear" || command_name == "cls") {
        ClearOutput();
        AddOutputLine("[INFO] Output cleared");
        return;
    }
    
    if (command_name == "help") {
        if (!registry_) {
            AddOutputLine("[ERROR] Command registry not initialized");
            return;
        }
        AddOutputLine(registry_->GenerateHelpText());
        return;
    }
    
    // Execute via registry if available
    if (!registry_) {
        AddOutputLine("[ERROR] Command registry not initialized");
        return;
    }
    
    CommandResult result = registry_->ExecuteCommand(command_name, args);
    if (result.success) {
        AddOutputLine("[SUCCESS] " + result.message);
    } else {
        AddOutputLine("[ERROR] " + result.message);
    }
}

void CommandWindow::HandleKeyInput(int key) {
    // Handle special keys
    switch (key) {
        case 10:  // Enter
            ExecuteInputCommand();
            break;
        case 127:  // Backspace
            if (!input_buffer_.empty()) {
                input_buffer_.pop_back();
            }
            break;
        case 27:  // Escape
            input_buffer_.clear();
            break;
        default:
            if (key >= 32 && key <= 126) {  // Printable ASCII
                input_buffer_ += static_cast<char>(key);
            }
            break;
    }
}

void CommandWindow::ClearHistory() {
    command_history_.clear();
    history_index_ = 0;
}

std::vector<std::string> CommandWindow::GetHistory() const {
    return std::vector<std::string>(command_history_.begin(), command_history_.end());
}

