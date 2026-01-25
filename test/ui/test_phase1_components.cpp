#include <gtest/gtest.h>
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include "ui/DashboardApplication.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include <memory>

using graph::MockGraphExecutor;

// ============================================================================
// DashboardApplication Tests
// ============================================================================

TEST(DashboardApplicationTest, ConstructsWithValidParameters) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;

    ASSERT_NO_THROW({
        DashboardApplication app(executor, heights);
    });
}

TEST(DashboardApplicationTest, ConstructsWithNullExecutorThrows) {
    WindowHeightConfig heights;

    EXPECT_THROW({
        DashboardApplication app(nullptr, heights);
    }, std::invalid_argument);
}

TEST(DashboardApplicationTest, ValidatesWindowHeights) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    heights.metrics_height_percent = 50;  // Invalid
    heights.logging_height_percent = 50;
    heights.command_height_percent = 0;

    DashboardApplication app(executor, heights);
    // Heights should be reset to defaults
    EXPECT_EQ(app.GetWindowHeights().metrics_height_percent, 40);
    EXPECT_EQ(app.GetWindowHeights().logging_height_percent, 35);
    EXPECT_EQ(app.GetWindowHeights().command_height_percent, 23);
    EXPECT_EQ(app.GetWindowHeights().status_height_percent, 2);
}

TEST(DashboardApplicationTest, HeightsResetToDefaultsWhenInvalid) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig invalid_heights;
    invalid_heights.metrics_height_percent = 50;  // Will sum to 107% with other defaults

    DashboardApplication app(executor, invalid_heights);
    // Constructor should validate and reset to defaults
    EXPECT_EQ(app.GetWindowHeights().metrics_height_percent, 40);
    EXPECT_EQ(app.GetWindowHeights().logging_height_percent, 35);
    EXPECT_EQ(app.GetWindowHeights().command_height_percent, 23);
    EXPECT_EQ(app.GetWindowHeights().status_height_percent, 2);
    EXPECT_TRUE(app.AreHeightsValid());
}

TEST(DashboardApplicationTest, InitializeCreatesComponents) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;

    DashboardApplication app(executor, heights);
    app.Initialize();

    EXPECT_NE(app.GetMetricsPanel(), nullptr);
    EXPECT_NE(app.GetLoggingWindow(), nullptr);
    EXPECT_NE(app.GetCommandWindow(), nullptr);
    EXPECT_NE(app.GetStatusBar(), nullptr);
}

// ============================================================================
// MetricsPanel Tests
// ============================================================================

TEST(MetricsPanelTest, ConstructsWithTitle) {
    MetricsPanel panel("Test Metrics");
    EXPECT_EQ(panel.GetTitle(), "Test Metrics");
}

TEST(MetricsPanelTest, HasInitialPlaceholders) {
    MetricsPanel panel;
    EXPECT_GT(panel.GetMetricCount(), 0);
}

TEST(MetricsPanelTest, AddPlaceholderMetric) {
    MetricsPanel panel;
    int initial_count = panel.GetMetricCount();

    panel.AddPlaceholderMetric("NewMetric", 42.0);
    EXPECT_EQ(panel.GetMetricCount(), initial_count + 1);
}

TEST(MetricsPanelTest, ClearPlaceholders) {
    MetricsPanel panel;
    EXPECT_GT(panel.GetMetricCount(), 0);

    panel.ClearPlaceholders();
    EXPECT_EQ(panel.GetMetricCount(), 0);
}

// ============================================================================
// LoggingWindow Tests
// ============================================================================

TEST(LoggingWindowTest, ConstructsWithTitle) {
    LoggingWindow window("Test Logs");
    EXPECT_EQ(window.GetTitle(), "Test Logs");
}

TEST(LoggingWindowTest, HasInitialLogs) {
    LoggingWindow window;
    EXPECT_GT(window.GetLogCount(), 0);
}

TEST(LoggingWindowTest, AddLogLine) {
    LoggingWindow window;
    size_t initial_count = window.GetLogCount();

    window.AddLogLine("Test log entry");
    EXPECT_EQ(window.GetLogCount(), initial_count + 1);
}

TEST(LoggingWindowTest, ClearLogs) {
    LoggingWindow window;
    EXPECT_GT(window.GetLogCount(), 0);

    window.ClearLogs();
    EXPECT_EQ(window.GetLogCount(), 0);
}

TEST(LoggingWindowTest, MaintainsMaxLineLimit) {
    LoggingWindow window;
    window.ClearLogs();

    // Add more than MAX_LINES
    for (int i = 0; i < 600; ++i) {
        window.AddLogLine("Log line " + std::to_string(i));
    }

    // Should maintain at most MAX_LINES (500)
    EXPECT_LE(window.GetLogCount(), 500);
}

// ============================================================================
// CommandWindow Tests
// ============================================================================

TEST(CommandWindowTest, ConstructsWithTitle) {
    CommandWindow window("Test Command");
    EXPECT_EQ(window.GetTitle(), "Test Command");
}

TEST(CommandWindowTest, HasInitialOutput) {
    CommandWindow window;
    EXPECT_GT(window.GetOutputCount(), 0);
}

TEST(CommandWindowTest, AddOutputLine) {
    CommandWindow window;
    size_t initial_count = window.GetOutputCount();

    window.AddOutputLine("Output message");
    EXPECT_EQ(window.GetOutputCount(), initial_count + 1);
}

TEST(CommandWindowTest, ClearOutput) {
    CommandWindow window;
    EXPECT_GT(window.GetOutputCount(), 0);

    window.ClearOutput();
    EXPECT_EQ(window.GetOutputCount(), 0);
}

// ============================================================================
// StatusBar Tests
// ============================================================================

TEST(StatusBarTest, ConstructsWithDefaults) {
    StatusBar bar;
    EXPECT_EQ(bar.GetStatus(), "READY");
    EXPECT_EQ(bar.GetActiveNodes(), 0);
    EXPECT_EQ(bar.GetTotalNodes(), 0);
    EXPECT_EQ(bar.GetErrorCount(), 0);
}

TEST(StatusBarTest, UpdatesStatus) {
    StatusBar bar;
    bar.SetStatus("RUNNING");
    EXPECT_EQ(bar.GetStatus(), "RUNNING");
}

TEST(StatusBarTest, UpdatesNodeCount) {
    StatusBar bar;
    bar.SetNodeCount(5, 10);
    EXPECT_EQ(bar.GetActiveNodes(), 5);
    EXPECT_EQ(bar.GetTotalNodes(), 10);
}

TEST(StatusBarTest, UpdatesErrorCount) {
    StatusBar bar;
    bar.SetErrorCount(3);
    EXPECT_EQ(bar.GetErrorCount(), 3);
}
