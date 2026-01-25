#pragma once

#include <memory>
#include <string>
#include "graph/mock/MockGraphExecutor.hpp"

using graph::MockGraphExecutor;

// Forward declarations
class MetricsPanel;
class LoggingWindow;
class CommandWindow;
class StatusBar;

struct WindowHeightConfig {
    int metrics_height_percent = 40;       // Fixed
    int logging_height_percent = 35;       // Adjustable
    int command_height_percent = 23;       // Adjustable (40+35+23+2=100)
    int status_height_percent = 2;         // Fixed

    bool Validate() const {
        return (metrics_height_percent + logging_height_percent +
                command_height_percent + status_height_percent) == 100;
    }

    std::string DebugString() const {
        return "Metrics:" + std::to_string(metrics_height_percent) + "% " +
               "Logging:" + std::to_string(logging_height_percent) + "% " +
               "Command:" + std::to_string(command_height_percent) + "% " +
               "Status:" + std::to_string(status_height_percent) + "%";
    }
};

class DashboardApplication {
public:
    // Constructor receives executor and window heights
    explicit DashboardApplication(
        std::shared_ptr<graph::MockGraphExecutor> executor,
        const WindowHeightConfig& heights);

    // Destructor - cleanup resources
    ~DashboardApplication();

    // Initialization: create UI panels, validate heights, setup layout
    void Initialize();

    // Main event loop: 30 FPS rendering until user exits
    void Run();

    // Getters for testing/debugging
    const std::shared_ptr<MetricsPanel>& GetMetricsPanel() const;
    const std::shared_ptr<LoggingWindow>& GetLoggingWindow() const;
    const std::shared_ptr<CommandWindow>& GetCommandWindow() const;
    const std::shared_ptr<StatusBar>& GetStatusBar() const;

    // For unit testing
    const WindowHeightConfig& GetWindowHeights() const;
    bool AreHeightsValid() const;

private:
    // Executor reference
    std::shared_ptr<graph::MockGraphExecutor> executor_;

    // Window components (created in Initialize())
    std::shared_ptr<MetricsPanel> metrics_panel_;      // 40%
    std::shared_ptr<LoggingWindow> logging_window_;    // 35%
    std::shared_ptr<CommandWindow> command_window_;    // 18%
    std::shared_ptr<StatusBar> status_bar_;            // 2%

    // Layout state
    WindowHeightConfig window_heights_;
    bool should_exit_ = false;
    bool initialized_ = false;

    // Private methods
    void ValidateHeights();
    void ApplyHeights();
};
