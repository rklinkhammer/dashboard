#include "ui/TabContainer.hpp"
#include "ui/MetricsTileWindow.hpp"
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <algorithm>

// ============================================================================
// Tab Implementation
// ============================================================================

Tab::Tab(const std::string& name)
    : name_(name) {
}

void Tab::AddTile(std::shared_ptr<MetricsTileWindow> tile) {
    if (!tile) return;
    tiles_.push_back(tile);
}

void Tab::ClearTiles() {
    tiles_.clear();
}

ftxui::Element Tab::Render() const {
    using namespace ftxui;
    
    if (tiles_.empty()) {
        return text("No metrics in this tab") | center | color(Color::Yellow);
    }
    
    // Render tiles as 3-column grid (up to 18 per tab = 6 rows × 3 columns)
    std::vector<Element> grid_rows;
    std::vector<Element> current_row;
    
    for (size_t i = 0; i < tiles_.size(); ++i) {
        current_row.push_back(tiles_[i]->Render() | flex);
        
        // When row is full (3 columns) or last tile
        if (current_row.size() == 3 || i == tiles_.size() - 1) {
            // Pad incomplete rows
            while (current_row.size() < 3) {
                current_row.push_back(text("") | flex);
            }
            
            grid_rows.push_back(hbox(std::move(current_row)));
            current_row.clear();
        }
    }
    
    return vbox(std::move(grid_rows));
}

// ============================================================================
// TabContainer Implementation
// ============================================================================

void TabContainer::CreateTab(const std::string& tab_name) {
    tabs_.emplace_back(tab_name);
}

void TabContainer::AddTileToTab(const std::string& tab_name, std::shared_ptr<MetricsTileWindow> tile) {
    if (!tile) return;
    
    // Find tab by name
    for (auto& tab : tabs_) {
        if (tab.GetName() == tab_name) {
            tab.AddTile(tile);
            return;
        }
    }
    
    // Tab not found - create it
    std::cerr << "[TabContainer] Warning: Tab '" << tab_name << "' not found, creating it\n";
    CreateTab(tab_name);
    AddTileToTab(tab_name, tile);
}

void TabContainer::ClearTabs() {
    tabs_.clear();
    current_tab_index_ = 0;
}

const Tab& TabContainer::GetCurrentTab() const {
    static const Tab empty_tab("empty");
    
    if (tabs_.empty()) {
        return empty_tab;
    }
    
    if (current_tab_index_ < 0 || current_tab_index_ >= static_cast<int>(tabs_.size())) {
        return empty_tab;
    }
    
    return tabs_[current_tab_index_];
}

void TabContainer::SelectTab(int index) {
    current_tab_index_ = index;
    ClampTabIndex();
}

void TabContainer::NextTab() {
    current_tab_index_++;
    ClampTabIndex();
}

void TabContainer::PreviousTab() {
    current_tab_index_--;
    ClampTabIndex();
}

void TabContainer::ClampTabIndex() {
    if (tabs_.empty()) {
        current_tab_index_ = 0;
        return;
    }
    
    if (current_tab_index_ < 0) {
        current_tab_index_ = 0;
    }
    
    if (current_tab_index_ >= static_cast<int>(tabs_.size())) {
        current_tab_index_ = static_cast<int>(tabs_.size()) - 1;
    }
}

std::string TabContainer::GetCurrentTabName() const {
    if (tabs_.empty()) {
        return "No tabs";
    }
    return GetCurrentTab().GetName();
}

std::string TabContainer::GetTabInfo() const {
    if (tabs_.empty()) {
        return "No tabs";
    }
    
    return "Tab " + std::to_string(current_tab_index_ + 1) + "/" + 
           std::to_string(tabs_.size());
}

ftxui::Element TabContainer::RenderTabs() const {
    using namespace ftxui;
    
    if (tabs_.empty()) {
        return text("No tabs");
    }
    
    std::vector<Element> tab_labels;
    
    for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
        std::string label = " " + tabs_[i].GetName() + " ";
        
        if (i == current_tab_index_) {
            // Current tab: highlight
            tab_labels.push_back(text(label) | bold | inverted | color(Color::White));
        } else {
            // Other tabs: normal
            tab_labels.push_back(text(label) | color(Color::Cyan));
        }
        
        // Add separator between tabs (except after last)
        if (i < static_cast<int>(tabs_.size()) - 1) {
            tab_labels.push_back(text(" | "));
        }
    }
    
    return hbox(std::move(tab_labels));
}

ftxui::Element TabContainer::RenderCurrentTab() const {
    if (tabs_.empty()) {
        return ftxui::text("No tabs") | ftxui::center | ftxui::color(ftxui::Color::Yellow);
    }
    
    return GetCurrentTab().Render();
}

ftxui::Element TabContainer::RenderWithTabs() const {
    using namespace ftxui;
    
    if (!IsInTabMode()) {
        // Single tab - render directly
        return RenderCurrentTab();
    }
    
    // Multiple tabs - show tab bar and current tab content
    std::vector<Element> layout;
    layout.push_back(RenderTabs() | border);
    layout.push_back(RenderCurrentTab());
    
    return vbox(std::move(layout));
}
