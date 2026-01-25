#include "ui/CommandWindow.hpp"
#include <iostream>

CommandWindow::CommandWindow(const std::string& title)
    : Window(title) {
    // Add initial prompt message
    output_lines_.push_back("Type 'help' for available commands");
    output_lines_.push_back("Type 'q' to quit");
    std::cerr << "[CommandWindow] Created with command prompt\n";
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
