#include "ui/StatusBar.hpp"
#include <iostream>
#include <ftxui/dom/elements.hpp>

StatusBar::StatusBar() : Window("Status") {
    // Initialize with default values
    std::cerr << "[StatusBar] Created\n";
}

ftxui::Element StatusBar::Render() const {
    using namespace ftxui;
    
    std::string status_text = "Status: " + status_ + 
                              " | Nodes: " + std::to_string(active_nodes_) + "/" + std::to_string(total_nodes_) +
                              " | Errors: " + std::to_string(error_count_);
    
    return text(status_text) 
        | bgcolor(Color::Black) 
        | color(Color::White)
        | hcenter;
}
