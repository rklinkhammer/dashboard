#include "ui/StatusBar.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>

StatusBar::StatusBar() : Window("Status") {
    // Initialize with default values
    std::cerr << "[StatusBar] Created\n";
}

ftxui::Element StatusBar::Render() const {
    using namespace ftxui;
    
    // Left side: status info
    std::string left_text = "Status: " + status_ + 
                            " | Nodes: " + std::to_string(active_nodes_) + "/" + std::to_string(total_nodes_) +
                            " | Errors: " + std::to_string(error_count_);
    
    // Right side: keyboard shortcuts
    std::string right_text = "q=quit | help=commands";
    
    // Combine with hbox to spread across width
    return hbox({
        text(left_text),
        filler(),
        text(right_text)
    })
        | bgcolor(Color::Black) 
        | color(Color::White);
}
