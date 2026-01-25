#include "ui/CommandWindow.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>

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
    
    // Add output lines
    for (const auto& line : output_lines_) {
        command_elements.push_back(text(line));
    }
    
    // Add input prompt
    command_elements.push_back(
        text("> " + input_buffer_ + "_") | color(Color::Green)
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
