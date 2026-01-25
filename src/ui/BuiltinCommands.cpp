#include "ui/BuiltinCommands.hpp"
#include "ui/DashboardApplication.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/StatusBar.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace commands {

CommandResult cmd::CmdStatus(DashboardApplication* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    std::ostringstream oss;
    oss << "\n╔════════════════════════════════════════════════════════╗\n";
    oss << "║         Dashboard Status                               ║\n";
    oss << "╠════════════════════════════════════════════════════════╣\n";
    
    auto metrics_panel = app->GetMetricsPanel();
    auto logging_window = app->GetLoggingWindow();
    auto command_window = app->GetCommandWindow();
    auto status_bar = app->GetStatusBar();
    
    oss << "║ Metrics Panel:";
    if (metrics_panel) {
        oss << " " << std::setw(39) << "Active";
    } else {
        oss << " " << std::setw(39) << "Inactive";
    }
    oss << " ║\n";
    
    oss << "║ Logging Window:";
    if (logging_window) {
        oss << " " << std::setw(38) << "Active";
    } else {
        oss << " " << std::setw(38) << "Inactive";
    }
    oss << " ║\n";
    
    oss << "║ Command Window:";
    if (command_window) {
        oss << " " << std::setw(38) << "Active";
    } else {
        oss << " " << std::setw(38) << "Inactive";
    }
    oss << " ║\n";
    
    oss << "║ Status Bar:";
    if (status_bar) {
        oss << " " << std::setw(42) << "Active";
    } else {
        oss << " " << std::setw(42) << "Inactive";
    }
    oss << " ║\n";
    
    oss << "╚════════════════════════════════════════════════════════╝\n";
    
    return CommandResult(true, oss.str());
}

CommandResult cmd::CmdRunGraph(DashboardApplication* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    // This would trigger graph execution in a real implementation
    // For now, return a success message
    return CommandResult(true, "Graph execution started");
}

CommandResult cmd::CmdPauseGraph(DashboardApplication* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    return CommandResult(true, "Graph execution paused");
}

CommandResult cmd::CmdStopGraph(DashboardApplication* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    return CommandResult(true, "Graph execution stopped");
}

CommandResult cmd::CmdResetLayout(DashboardApplication* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    if (!app->AreHeightsValid()) {
        return CommandResult(false, "Window layout is invalid");
    }
    
    return CommandResult(true, "Layout reset to default configuration");
}

void RegisterBuiltinCommands(
    std::shared_ptr<CommandRegistry> registry,
    DashboardApplication* app) {
    
    if (!registry || !app) {
        std::cerr << "[BuiltinCommands] Error: Invalid registry or app pointer\n";
        return;
    }
    
    // Status command - display current dashboard state
    registry->RegisterCommand(
        "status",
        "Display current dashboard status",
        "status",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdStatus(app, args);
        }
    );
    
    // Run graph command - execute the computation graph
    registry->RegisterCommand(
        "run",
        "Execute the computation graph",
        "run [--iterations N]",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdRunGraph(app, args);
        }
    );
    
    // Pause graph command
    registry->RegisterCommand(
        "pause",
        "Pause the computation graph",
        "pause",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdPauseGraph(app, args);
        }
    );
    
    // Stop graph command
    registry->RegisterCommand(
        "stop",
        "Stop the computation graph",
        "stop",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdStopGraph(app, args);
        }
    );
    
    // Reset layout command - reset window layout to defaults
    registry->RegisterCommand(
        "reset_layout",
        "Reset window layout to default configuration",
        "reset_layout",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdResetLayout(app, args);
        }
    );
    
    std::cerr << "[BuiltinCommands] Registered 5 built-in commands\n";
}

}  // namespace commands
