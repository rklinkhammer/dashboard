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
        std::shared_ptr<app::capabilities::MetricsCapability> capabilitys);

    // Destructor - cleanup resources
    ~Dashboard();

    // Initialization: create UI panels, validate heights, setup layout
    void Initialize() {}

    // Main event loop: 30 FPS rendering until user exits
    void Run();

private:
    // Executor reference
    std::shared_ptr<app::capabilities::MetricsCapability> capability_;
    ftxui::ScreenInteractive screen_;
};
