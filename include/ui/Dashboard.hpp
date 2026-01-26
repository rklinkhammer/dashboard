#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include "graph/GraphExecutor.hpp"
#include "app/capabilities/MetricsCapability.hpp"

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

class Dashboard {
public:
    // Constructor receives executor and window heights
    explicit Dashboard(
        std::shared_ptr<app::capabilities::MetricsCapability> capability,
        const WindowHeightConfig& heights);

    // Destructor - cleanup resources
    ~Dashboard();

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
    std::shared_ptr<app::capabilities::MetricsCapability> capability_;

    // Window components (created in Initialize())
    std::shared_ptr<MetricsPanel> metrics_panel_;      // 40%
    std::shared_ptr<LoggingWindow> logging_window_;    // 35%
    std::shared_ptr<CommandWindow> command_window_;    // 23%
    std::shared_ptr<StatusBar> status_bar_;            // 2%

    // Layout state
    WindowHeightConfig window_heights_;
    bool should_exit_ = false;
    bool initialized_ = false;

    // Event loop state
    std::chrono::high_resolution_clock::time_point last_frame_;
    std::chrono::milliseconds frame_time_{33};  // ~30 FPS (33ms per frame)
    
    // Screen dimensions for resize detection
    int last_screen_width_ = -1;
    int last_screen_height_ = -1;

    // Private methods
    void ValidateHeights();
    void ApplyHeights();
    
    // Build the complete FTXUI layout from all window components
    ftxui::Element BuildLayout() const;
    
    // Event loop methods
    void HandleKeyEvent(const ftxui::Event& event);
    bool CheckForTerminalResize();
    void RecalculateLayout();
    
    // Getters for testing
    int GetScreenHeight() const { return last_screen_height_; }
    int GetScreenWidth() const { return last_screen_width_; }
};
