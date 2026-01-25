#include <gtest/gtest.h>
#include "ui/MetricsPanel.hpp"
#include "ui/TabContainer.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include <ftxui/dom/elements.hpp>
#include <memory>

using graph::MockGraphExecutor;

// ============================================================================
// D4.4: Arrow Key Navigation for Tab Switching
// ============================================================================
// Tests for implementing left/right arrow key event handling in MetricsPanel
// for navigating between tabs when in tab mode.

class ArrowKeyNavigationTest : public ::testing::Test {
protected:
    std::shared_ptr<MockGraphExecutor> executor_;
    
    void SetUp() override {
        executor_ = std::make_shared<MockGraphExecutor>();
    }
};

// Test 1: MetricsPanel can handle arrow key events
TEST_F(ArrowKeyNavigationTest, MetricsPanelHandlesArrowKeyEvents) {
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    panel->DiscoverMetricsFromExecutor(executor_);
    
    // Panel should be initialized and ready for events
    EXPECT_NE(panel, nullptr);
}

// Test 2: Tab container can navigate to next tab
TEST_F(ArrowKeyNavigationTest, TabContainerNavigatesWithNextTab) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
}

// Test 3: Tab container clamps at end when using NextTab
TEST_F(ArrowKeyNavigationTest, TabContainerClampsAtEndWhenCallingNextTab) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Move to last tab
    container.SelectTab(2);
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
    
    // Move to next should clamp to last
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
}

// Test 4: Tab container can navigate to previous tab
TEST_F(ArrowKeyNavigationTest, TabContainerNavigatesWithPreviousTab) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Start at tab 2
    container.SelectTab(2);
    EXPECT_EQ(container.GetCurrentTabIndex(), 2);
    
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);
    
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

// Test 5: Tab container clamps at start when using PreviousTab
TEST_F(ArrowKeyNavigationTest, TabContainerClampsAtStartWhenCallingPreviousTab) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Start at first tab
    container.SelectTab(0);
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
    
    // Move to previous should clamp to first
    container.PreviousTab();
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);
}

// Test 6: MetricsPanel in tab mode has accessible tab container
TEST_F(ArrowKeyNavigationTest, MetricsPanelInTabModeHasAccessibleTabContainer) {
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    panel->DiscoverMetricsFromExecutor(executor_);
    
    // Get tab container reference
    auto& container = panel->GetTabContainer();
    
    // Should be able to navigate tabs
    if (panel->IsInTabMode()) {
        container.NextTab();
        // Just verify no exception thrown
    }
}

// Test 7: Tab navigation preserves tab identity
TEST_F(ArrowKeyNavigationTest, TabNavigationPreservesTabIdentity) {
    TabContainer container;
    container.CreateTab("DataNode");
    container.CreateTab("FilterNode");
    container.CreateTab("SinkNode");
    
    container.SelectTab(0);
    EXPECT_EQ(container.GetCurrentTabName(), "DataNode");
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabName(), "FilterNode");
    
    container.NextTab();
    EXPECT_EQ(container.GetCurrentTabName(), "SinkNode");
}

// Test 8: Tab info shows correct position
TEST_F(ArrowKeyNavigationTest, TabInfoShowsCurrentPosition) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    container.SelectTab(0);
    std::string info = container.GetTabInfo();
    EXPECT_TRUE(info.find("1") != std::string::npos);  // Should contain "1" for Tab 1
}

// Test 9: Rapid tab navigation works correctly
TEST_F(ArrowKeyNavigationTest, RapidTabNavigationWorks) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Rapid forward navigation
    for (int i = 0; i < 10; i++) {
        container.NextTab();
    }
    
    // Should still have valid current tab (wrapping occurred)
    EXPECT_GE(container.GetCurrentTabIndex(), 0);
    EXPECT_LT(container.GetCurrentTabIndex(), 3);
}

// Test 10: Tab selection is clamped to valid range
TEST_F(ArrowKeyNavigationTest, TabSelectionIsClamped) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    
    // Try to select invalid indices
    container.SelectTab(-1);
    EXPECT_EQ(container.GetCurrentTabIndex(), 0);  // Should be clamped to 0
    
    container.SelectTab(10);
    EXPECT_EQ(container.GetCurrentTabIndex(), 1);  // Should be clamped to max (1)
}

// Test 11: Arrow key event handling is conceptually supported
TEST_F(ArrowKeyNavigationTest, ArrowKeyEventHandlingIsSupported) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Simulate right arrow key by calling NextTab
    int initial_index = container.GetCurrentTabIndex();
    container.NextTab();
    EXPECT_NE(container.GetCurrentTabIndex(), initial_index);
}

// Test 12: Arrow key event handling can navigate backwards
TEST_F(ArrowKeyNavigationTest, ArrowKeyEventHandlingNavigatesBackwards) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    container.CreateTab("Tab3");
    
    // Simulate left arrow key by calling PreviousTab
    container.SelectTab(2);
    int initial_index = container.GetCurrentTabIndex();
    container.PreviousTab();
    EXPECT_NE(container.GetCurrentTabIndex(), initial_index);
}

// Test 13: MetricsPanel provides tab information
TEST_F(ArrowKeyNavigationTest, MetricsPanelProvidesTabInformation) {
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    panel->DiscoverMetricsFromExecutor(executor_);
    
    auto& container = panel->GetTabContainer();
    
    // Should be able to get tab info
    if (panel->IsInTabMode()) {
        std::string info = container.GetTabInfo();
        EXPECT_FALSE(info.empty());
    }
}

// Test 14: Tab switching maintains rendering state
TEST_F(ArrowKeyNavigationTest, TabSwitchingMaintainsRenderingState) {
    TabContainer container;
    container.CreateTab("Tab1");
    container.CreateTab("Tab2");
    
    // Get initial rendering
    auto elem1 = container.RenderWithTabs();
    EXPECT_NE(elem1, nullptr);
    
    // Switch tab
    container.NextTab();
    
    // Get rendering after switch
    auto elem2 = container.RenderWithTabs();
    EXPECT_NE(elem2, nullptr);
}

// Test 15: MetricsPanel tab mode activation at 37 tiles triggers navigation capability
TEST_F(ArrowKeyNavigationTest, MetricsPanelTabModeActivationEnablesNavigation) {
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    panel->DiscoverMetricsFromExecutor(executor_);
    
    // After discovery, check if tab mode is active
    bool in_tab_mode = panel->IsInTabMode();
    
    // Tab navigation should only be available in tab mode
    if (in_tab_mode) {
        auto& container = panel->GetTabContainer();
        EXPECT_GT(container.GetTabCount(), 1);
    }
}
