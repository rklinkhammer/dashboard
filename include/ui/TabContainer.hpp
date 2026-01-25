#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <ftxui/dom/elements.hpp>

// Forward declaration
class MetricsTileWindow;

// Represents a single tab containing a group of metrics
class Tab {
public:
    explicit Tab(const std::string& name = "");
    
    // Tab metadata
    const std::string& GetName() const { return name_; }
    
    // Tile management
    void AddTile(std::shared_ptr<MetricsTileWindow> tile);
    void ClearTiles();
    size_t GetTileCount() const { return tiles_.size(); }
    const std::vector<std::shared_ptr<MetricsTileWindow>>& GetTiles() const { return tiles_; }
    
    // Rendering
    ftxui::Element Render() const;
    
private:
    std::string name_;
    std::vector<std::shared_ptr<MetricsTileWindow>> tiles_;
};

// Container that manages multiple tabs and handles navigation
class TabContainer {
public:
    TabContainer() = default;
    
    // Tab management
    void CreateTab(const std::string& tab_name);
    void AddTileToTab(const std::string& tab_name, std::shared_ptr<MetricsTileWindow> tile);
    void ClearTabs();
    
    // Tab count and info
    size_t GetTabCount() const { return tabs_.size(); }
    const std::vector<Tab>& GetAllTabs() const { return tabs_; }
    
    // Current tab selection
    int GetCurrentTabIndex() const { return current_tab_index_; }
    const Tab& GetCurrentTab() const;
    
    // Navigation
    void SelectTab(int index);
    void NextTab();
    void PreviousTab();
    
    // Validation
    bool IsValid() const { return !tabs_.empty(); }
    bool IsInTabMode() const { return GetTabCount() > 1; }
    
    // Rendering
    ftxui::Element RenderTabs() const;
    ftxui::Element RenderCurrentTab() const;
    ftxui::Element RenderWithTabs() const;
    
    // Status info
    std::string GetCurrentTabName() const;
    std::string GetTabInfo() const;  // Returns "Tab 1/3" style info
    
private:
    std::vector<Tab> tabs_;
    int current_tab_index_ = 0;
    
    // Helper to clamp index
    void ClampTabIndex();
};
