#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include "graph/GraphExecutor.hpp"
#include "app/capabilities/MetricsCapability.hpp"

// Forward declarations
class MetricsPanel;
class LoggingWindow;
class CommandWindow;
class StatusBar;
class CommandRegistry;

struct WindowHeightConfig {
    int metrics_height_percent = 68;       // Fixed
    int logging_height_percent = 15;       // Adjustable
    int command_height_percent = 15;       // Adjustable (40+20+38+2=100)
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
    const std::shared_ptr<CommandRegistry>& GetCommandRegistry() const;
    const std::shared_ptr<StatusBar>& GetStatusBar() const;

    // For unit testing
    const WindowHeightConfig& GetWindowHeights() const;
    bool AreHeightsValid() const;

    // Signal that screen needs redraw (called when log messages arrive)
    void MarkScreenDirty() { screen_dirty_ = true; }

private:
    // Executor reference
    std::shared_ptr<app::capabilities::MetricsCapability> capability_;

    // Window components (created in Initialize())
    std::shared_ptr<MetricsPanel> metrics_panel_;      // 40%
    std::shared_ptr<LoggingWindow> logging_window_;    // 20%
    std::shared_ptr<CommandWindow> command_window_;    // 38%
    std::shared_ptr<StatusBar> status_bar_;            // 2%
    std::shared_ptr<CommandRegistry> command_registry_; // Command execution system

    ftxui::ScreenInteractive screen_;

    // Layout state
    WindowHeightConfig window_heights_;
    bool should_exit_ = false;
    bool initialized_ = false;
    bool show_debug_overlay_ = false;

    // Event loop state
    std::chrono::high_resolution_clock::time_point last_frame_;
    std::chrono::milliseconds frame_time_{33};  // ~30 FPS (33ms per frame)
    
    // Screen dimensions for resize detection
    int last_screen_width_ = -1;
    int last_screen_height_ = -1;
    
    // Dirty flag for screen rendering optimization
    // Only redraw when screen needs updating to reduce flicker in quiescent state
    bool screen_dirty_ = true;  // Initial render always needed

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
