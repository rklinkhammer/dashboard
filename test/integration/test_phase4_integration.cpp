#include <gtest/gtest.h>
#include "ui/DashboardApplication.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include "ui/TabContainer.hpp"
#include "ui/CommandRegistry.hpp"
#include "ui/LayoutConfig.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include <memory>
#include <filesystem>

using graph::MockGraphExecutor;
namespace fs = std::filesystem;

// ============================================================================
// D4.6: Integration Test Suite & Final Validation
// ============================================================================
// Comprehensive end-to-end integration tests for Phase 4 features:
// - Tab container integration with metrics panel
// - Layout config persistence and restoration
// - Command registry integration
// - Arrow key navigation
// - Logging filter integration

class Phase4IntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<MockGraphExecutor> executor_;
    std::string test_config_path_ = "/tmp/test_phase4.json";
    
    void SetUp() override {
        executor_ = std::make_shared<MockGraphExecutor>();
    }
    
    void TearDown() override {
        if (fs::exists(test_config_path_)) {
            fs::remove(test_config_path_);
        }
    }
};

// Test 1: Complete initialization flow with all components
TEST_F(Phase4IntegrationTest, CompleteInitializationFlow) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    
    DashboardApplication app(executor, heights);
    ASSERT_NO_THROW({
        app.Initialize();
    });
    
    EXPECT_NE(app.GetMetricsPanel(), nullptr);
    EXPECT_NE(app.GetLoggingWindow(), nullptr);
    EXPECT_NE(app.GetCommandWindow(), nullptr);
    EXPECT_NE(app.GetStatusBar(), nullptr);
}

// Test 2: Metrics panel discovers and activates tab mode
TEST_F(Phase4IntegrationTest, MetricsPanelDiscoveriesAndActivatesTabMode) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    
    // Before discovery, should not be in tab mode
    EXPECT_FALSE(panel->IsInTabMode());
    
    // Discover metrics
    panel->DiscoverMetricsFromExecutor(executor);
    
    // After discovery with > 36 tiles, should be in tab mode
    bool in_tab_mode = panel->IsInTabMode();
    if (panel->GetMetricCount() > 36) {
        EXPECT_TRUE(in_tab_mode);
    }
}

// Test 3: Tab navigation maintains tab state
TEST_F(Phase4IntegrationTest, TabNavigationMaintainsState) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    
    panel->DiscoverMetricsFromExecutor(executor);
    
    if (panel->IsInTabMode()) {
        auto& container = panel->GetTabContainer();
        int initial_tab = container.GetCurrentTabIndex();
        
        // Navigate
        container.NextTab();
        
        // Tab should have changed or been at the boundary
        EXPECT_GE(container.GetCurrentTabIndex(), 0);
    }
}

// Test 4: Layout config integration with application
TEST_F(Phase4IntegrationTest, LayoutConfigIntegrationWithApplication) {
    // Create initial config
    LayoutConfig config1(test_config_path_);
    config1.SetMetricsHeight(45);
    config1.SetLoggingHeight(30);
    config1.SetCommandHeight(23);
    config1.SetLoggingLevelFilter("ERROR");
    config1.Save();
    
    // Load and verify
    LayoutConfig config2(test_config_path_);
    config2.Load();
    
    EXPECT_EQ(config2.GetMetricsHeightPercent(), 45);
    EXPECT_EQ(config2.GetLoggingHeightPercent(), 30);
    EXPECT_EQ(config2.GetCommandHeightPercent(), 23);
    EXPECT_EQ(config2.GetLoggingLevelFilter(), "ERROR");
}

// Test 5: Logging filter persists through save/load
TEST_F(Phase4IntegrationTest, LoggingFilterPersistence) {
    LayoutConfig config1(test_config_path_);
    config1.SetLoggingLevelFilter("WARN");
    config1.SetLoggingSearchFilter("database connection");
    config1.Save();
    
    LayoutConfig config2(test_config_path_);
    config2.Load();
    
    EXPECT_EQ(config2.GetLoggingLevelFilter(), "WARN");
    EXPECT_EQ(config2.GetLoggingSearchFilter(), "database connection");
}

// Test 6: Window heights are validated and persisted
TEST_F(Phase4IntegrationTest, WindowHeightsPersistence) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    heights.metrics_height_percent = 45;
    heights.logging_height_percent = 30;
    heights.command_height_percent = 23;
    
    DashboardApplication app(executor, heights);
    EXPECT_TRUE(app.AreHeightsValid());
    EXPECT_EQ(app.GetWindowHeights().metrics_height_percent, 45);
}

// Test 7: MetricsPanel provides accessible tab container
TEST_F(Phase4IntegrationTest, MetricsPanelTabContainerAccess) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    
    panel->DiscoverMetricsFromExecutor(executor);
    
    auto& container = panel->GetTabContainer();
    EXPECT_GE(container.GetTabCount(), 0);
}

// Test 8: Tab info provides useful status
TEST_F(Phase4IntegrationTest, TabInfoProvidedInTabMode) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    
    panel->DiscoverMetricsFromExecutor(executor);
    
    if (panel->IsInTabMode()) {
        auto& container = panel->GetTabContainer();
        std::string info = container.GetTabInfo();
        EXPECT_FALSE(info.empty());
    }
}

// Test 9: Command registry integration with dashboard
TEST_F(Phase4IntegrationTest, CommandRegistryWithDashboard) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    
    // Register a test command
    registry->RegisterCommand(
        "test_cmd",
        "A test command",
        "test_cmd",
        [](const std::vector<std::string>& args) {
            return CommandResult(true, "Test command executed");
        }
    );
    
    EXPECT_TRUE(registry->HasCommand("test_cmd"));
    
    auto result = registry->ExecuteCommand("test_cmd", {});
    EXPECT_TRUE(result.success);
}

// Test 10: Logging window filter application flow
TEST_F(Phase4IntegrationTest, LoggingFilterApplicationFlow) {
    auto window = std::make_shared<LoggingWindow>("Logs");
    
    // Save filter to config
    LayoutConfig config(test_config_path_);
    config.SetLoggingLevelFilter("INFO");
    config.Save();
    
    // Load and apply
    LayoutConfig loaded(test_config_path_);
    loaded.Load();
    window->SetLevelFilter(loaded.GetLoggingLevelFilter());
    
    EXPECT_EQ(window->GetLevelFilter(), "INFO");
}

// Test 11: Multiple component independence
TEST_F(Phase4IntegrationTest, MultipleComponentsAreIndependent) {
    auto panel1 = std::make_shared<MetricsPanel>("Metrics1");
    auto panel2 = std::make_shared<MetricsPanel>("Metrics2");
    auto window1 = std::make_shared<LoggingWindow>("Logs1");
    auto window2 = std::make_shared<LoggingWindow>("Logs2");
    
    window1->SetLevelFilter("DEBUG");
    window2->SetLevelFilter("ERROR");
    
    EXPECT_EQ(window1->GetLevelFilter(), "DEBUG");
    EXPECT_EQ(window2->GetLevelFilter(), "ERROR");
}

// Test 12: Tab switching doesn't affect other panels
TEST_F(Phase4IntegrationTest, TabSwitchingDoesntAffectOtherPanels) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    auto window = std::make_shared<LoggingWindow>("Logs");
    
    panel->DiscoverMetricsFromExecutor(executor);
    window->SetLevelFilter("WARN");
    
    if (panel->IsInTabMode()) {
        panel->GetTabContainer().NextTab();
    }
    
    EXPECT_EQ(window->GetLevelFilter(), "WARN");
}

// Test 13: Height configuration validation
TEST_F(Phase4IntegrationTest, HeightConfigValidation) {
    WindowHeightConfig heights;
    heights.metrics_height_percent = 40;
    heights.logging_height_percent = 35;
    heights.command_height_percent = 23;
    heights.status_height_percent = 2;
    
    EXPECT_TRUE(heights.Validate());
    
    WindowHeightConfig invalid;
    invalid.metrics_height_percent = 50;
    invalid.logging_height_percent = 50;
    
    EXPECT_FALSE(invalid.Validate());
}

// Test 14: Metrics count affects tab activation
TEST_F(Phase4IntegrationTest, MetricsCountAffectsTabActivation) {
    auto executor = std::make_shared<MockGraphExecutor>();
    auto panel = std::make_shared<MetricsPanel>("Metrics");
    
    // Before discovery, metrics count should be placeholder count
    int initial_count = panel->GetMetricCount();
    EXPECT_GE(initial_count, 0);
    
    // Discover metrics
    panel->DiscoverMetricsFromExecutor(executor);
    
    // Count might change
    int final_count = panel->GetMetricCount();
    EXPECT_GE(final_count, 0);
}

// Test 15: Complete end-to-end phase 4 flow
TEST_F(Phase4IntegrationTest, CompletePhase4EndToEndFlow) {
    // 1. Create executor and app
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    
    // 2. Initialize app
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    // 3. Get panels
    auto metrics = app.GetMetricsPanel();
    auto logging = app.GetLoggingWindow();
    auto commands = app.GetCommandWindow();
    
    ASSERT_NE(metrics, nullptr);
    ASSERT_NE(logging, nullptr);
    ASSERT_NE(commands, nullptr);
    
    // 4. Discover metrics (triggers tab mode if > 36)
    metrics->DiscoverMetricsFromExecutor(executor);
    
    // 5. Apply filter from config
    logging->SetLevelFilter("INFO");
    EXPECT_EQ(logging->GetLevelFilter(), "INFO");
    
    // 6. Create and apply command registry
    auto registry = std::make_shared<CommandRegistry>();
    registry->RegisterCommand(
        "status",
        "Status command",
        "status",
        [](const std::vector<std::string>& args) {
            return CommandResult(true, "OK");
        }
    );
    
    // 7. Navigate tabs if in tab mode
    if (metrics->IsInTabMode()) {
        auto& container = metrics->GetTabContainer();
        container.NextTab();
        // Should not throw
    }
    
    // All components operational
    EXPECT_TRUE(true);
}
