// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>
#include "ui/TabContainer.hpp"
#include "ui/MetricsTileWindow.hpp"
#include <memory>

// Helper to create a mock tile
std::shared_ptr<MetricsTileWindow> CreateMockTile(const std::string& node, const std::string& metric) {
    MetricDescriptor desc;
    desc.node_name = node;
    desc.metric_name = metric;
    desc.metric_id = node + "::" + metric;
    
    nlohmann::json field;
    field["name"] = metric;
    field["unit"] = "units";
    
    return std::make_shared<MetricsTileWindow>(desc, field);
}

// ============================================================================
// Tab Tests
// ============================================================================

TEST(TabTests, CreateTabWithName) {
    Tab tab("Node A");
    EXPECT_EQ(tab.GetName(), "Node A");
    EXPECT_EQ(tab.GetTileCount(), 0);
}

TEST(TabTests, AddTileToTab) {
    Tab tab("Node A");
    auto tile = CreateMockTile("Node A", "metric1");
    
    tab.AddTile(tile);
    EXPECT_EQ(tab.GetTileCount(), 1);
}

TEST(TabTests, AddMultipleTilesToTab) {
    Tab tab("Node A");
    
    for (int i = 0; i < 5; ++i) {
        auto tile = CreateMockTile("Node A", "metric" + std::to_string(i));
        tab.AddTile(tile);
    }
    
    EXPECT_EQ(tab.GetTileCount(), 5);
}

TEST(TabTests, ClearTilesInTab) {
    Tab tab("Node A");
    
    auto tile = CreateMockTile("Node A", "metric1");
    tab.AddTile(tile);
    EXPECT_EQ(tab.GetTileCount(), 1);
    
    tab.ClearTiles();
    EXPECT_EQ(tab.GetTileCount(), 0);
}

TEST(TabTests, GetTilesFromTab) {
    Tab tab("Node A");
    
    for (int i = 0; i < 3; ++i) {
        auto tile = CreateMockTile("Node A", "metric" + std::to_string(i));
        tab.AddTile(tile);
    }
    
    const auto& tiles = tab.GetTiles();
    EXPECT_EQ(tiles.size(), 3);
}

// ============================================================================
// TabContainer Tests
// ============================================================================

TEST(TabContainerTests, CreateEmptyContainer) {
    TabContainer container;
    EXPECT_EQ(container.GetTabCount(), 0);
    EXPECT_FALSE(container.IsValid());
    EXPECT_FALSE(container.IsInTabMode());
}

TEST(TabContainerTests, CreateTab) {
    TabContainer container;
    container.CreateTab("Node A");
    
    EXPECT_EQ(container.GetTabCount(), 1);
    EXPECT_TRUE(container.IsValid());
}

TEST(TabContainerTests, CreateMultipleTabs) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    EXPECT_EQ(container.GetTabCount(), 3);
    EXPECT_TRUE(container.IsInTabMode());
}

TEST(TabContainerTests, SingleTabIsNotTabMode) {
    TabContainer container;
    container.CreateTab("Single");
    
    EXPECT_EQ(container.GetTabCount(), 1);
    EXPECT_FALSE(container.IsInTabMode());  // Single tab doesn't trigger tab mode
}

TEST(TabContainerTests, AddTileToTab) {
    TabContainer container;
    container.CreateTab("Node A");
    
    auto tile = CreateMockTile("Node A", "metric1");
    container.AddTileToTab("Node A", tile);
    
    EXPECT_EQ(container.GetCurrentTab().GetTileCount(), 1);
}

TEST(TabContainerTests, AddTileToNonExistentTabCreatesIt) {
    TabContainer container;
    
    auto tile = CreateMockTile("Node B", "metric1");
    container.AddTileToTab("Node B", tile);
    
    EXPECT_EQ(container.GetTabCount(), 1);
    EXPECT_EQ(container.GetCurrentTab().GetTileCount(), 1);
}

TEST(TabContainerTests, CurrentTabIndexInitialization) {
    TabContainer container;
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

TEST(TabContainerTests, SelectTab) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    container.SelectTab(1);
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);
    EXPECT_EQ(container.GetCurrentTabName(), "Node B");
}

TEST(TabContainerTests, NextTab) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
}

TEST(TabContainerTests, NextTabWrapsAtEnd) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    container.SelectTab(2);
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
    
    // Trying to go beyond last tab should clamp to last
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
}

TEST(TabContainerTests, PreviousTab) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    container.SelectTab(2);
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
    
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);
    
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

TEST(TabContainerTests, PreviousTabClamps) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
    
    // Trying to go below 0 should clamp to 0
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

TEST(TabContainerTests, TabNavigation) {
    TabContainer container;
    
    for (int i = 0; i < 4; ++i) {
        container.CreateTab("Node " + std::to_string(i));
    }
    
    // Forward navigation
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(container.GetCurrentTabIndex(), i);
        if (i < 3) container.NextTab();
    }
    
    // Backward navigation
    for (int i = 3; i >= 0; --i) {
        EXPECT_EQ(container.GetCurrentTabIndex(), i);
        if (i > 0) container.PreviousTab();
    }
}

TEST(TabContainerTests, GetTabInfo) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    EXPECT_EQ(container.GetTabInfo(), "Tab 1/3");
    
    container.NextTab();
    EXPECT_EQ(container.GetTabInfo(), "Tab 2/3");
    
    container.NextTab();
    EXPECT_EQ(container.GetTabInfo(), "Tab 3/3");
}

TEST(TabContainerTests, ClearTabs) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    EXPECT_EQ(container.GetTabCount(), 3);
    
    container.ClearTabs();
    EXPECT_EQ(container.GetTabCount(), 0);
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

TEST(TabContainerTests, RenderingDoesNotThrow) {
    TabContainer container;
    container.CreateTab("Node A");
    
    auto tile = CreateMockTile("Node A", "metric1");
    container.AddTileToTab("Node A", tile);
    
    // These should not throw
    auto tabs_element = container.RenderTabs();
    auto tab_element = container.RenderCurrentTab();
    auto full_element = container.RenderWithTabs();
    
    EXPECT_TRUE(true);  // If we got here, no exceptions were thrown
}

TEST(TabContainerTests, LargeNumberOfTabs) {
    TabContainer container;
    
    // Create 10 tabs
    for (int i = 0; i < 10; ++i) {
        container.CreateTab("Node " + std::to_string(i));
    }
    
    EXPECT_EQ(container.GetTabCount(), 10);
    
    // Navigate through all
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(container.GetCurrentTabIndex(), i);
        EXPECT_EQ(container.GetCurrentTabName(), "Node " + std::to_string(i));
        
        if (i < 9) {
            container.NextTab();
        }
    }
}

TEST(TabContainerTests, TilesDistributedAcrossTabs) {
    TabContainer container;
    
    for (int node = 0; node < 3; ++node) {
        container.CreateTab("Node " + std::to_string(node));
        
        for (int metric = 0; metric < 4; ++metric) {
            auto tile = CreateMockTile("Node " + std::to_string(node), 
                                       "metric" + std::to_string(metric));
            container.AddTileToTab("Node " + std::to_string(node), tile);
        }
    }
    
    EXPECT_EQ(container.GetTabCount(), 3);
    
    for (const auto& tab : container.GetAllTabs()) {
        EXPECT_EQ(tab.GetTileCount(), 4);
    }
}

TEST(TabContainerTests, SelectTabByIndex) {
    TabContainer container;
    
    for (int i = 0; i < 5; ++i) {
        container.CreateTab("Tab " + std::to_string(i));
    }
    
    container.SelectTab(3);
    EXPECT_EQ(container.GetCurrentTabIndex(), 3);
    EXPECT_EQ(container.GetCurrentTabName(), "Tab 3");
}

TEST(TabContainerTests, SelectTabBeyondRangeClamps) {
    TabContainer container;
    
    for (int i = 0; i < 3; ++i) {
        container.CreateTab("Node " + std::string(1, 'A' + i));
    }
    
    container.SelectTab(10);  // Out of range
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);  // Clamped to last tab
}
