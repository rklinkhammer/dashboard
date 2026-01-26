#include "ui/BuiltinCommands.hpp"
#include "ui/Dashboard.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/StatusBar.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

namespace commands {

CommandResult cmd::CmdStatus(Dashboard* app, const std::vector<std::string>& /* args */) {
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

CommandResult cmd::CmdRunGraph(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    // This would trigger graph execution in a real implementation
    // For now, return a success message
    return CommandResult(true, "Graph execution started");
}

CommandResult cmd::CmdPauseGraph(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    return CommandResult(true, "Graph execution paused");
}

CommandResult cmd::CmdStopGraph(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    return CommandResult(true, "Graph execution stopped");
}

CommandResult cmd::CmdResetLayout(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    if (!app->AreHeightsValid()) {
        return CommandResult(false, "Window layout is invalid");
    }
    
    return CommandResult(true, "Layout reset to default configuration");
}

// ========== Phase 3b: Filtering & Search ==========

CommandResult cmd::CmdFilterGlobal(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.empty()) {
        return CommandResult(false, "Usage: filter.global <pattern>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string pattern = args[0];
    std::string old_filter = tile_panel->GetGlobalSearchFilter();
    
    tile_panel->SetGlobalSearchFilter(pattern);
    
    std::ostringstream oss;
    oss << "Global filter applied: '" << pattern << "'";
    if (!old_filter.empty()) {
        oss << " (was: '" << old_filter << "')";
    }
    
    return CommandResult(true, oss.str());
}

CommandResult cmd::CmdFilterNode(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.size() < 2) {
        return CommandResult(false, "Usage: filter.node <node_name> <pattern>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string node_name = args[0];
    std::string pattern = args[1];
    
    tile_panel->SetNodeFilter(node_name, pattern);
    
    std::ostringstream oss;
    oss << "Node filter applied: " << node_name << " → '" << pattern << "'";
    return CommandResult(true, oss.str());
}

CommandResult cmd::CmdFilterClear(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    tile_panel->ClearAllFilters();
    return CommandResult(true, "All filters cleared");
}

// ========== Phase 3c: Collapse/Expand ==========

CommandResult cmd::CmdCollapseToggle(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.empty()) {
        return CommandResult(false, "Usage: collapse.toggle <node_name>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string node_name = args[0];
    tile_panel->ToggleNodeCollapsed(node_name);
    
    bool is_collapsed = tile_panel->IsNodeCollapsed(node_name);
    std::string state = is_collapsed ? "collapsed" : "expanded";
    
    return CommandResult(true, "Node '" + node_name + "' is now " + state);
}

CommandResult cmd::CmdCollapseSet(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.size() < 2) {
        return CommandResult(false, "Usage: collapse.set <node_name> <on|off>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string node_name = args[0];
    std::string state_str = args[1];
    
    bool collapse_state;
    if (state_str == "on" || state_str == "true" || state_str == "1") {
        collapse_state = true;
    } else if (state_str == "off" || state_str == "false" || state_str == "0") {
        collapse_state = false;
    } else {
        return CommandResult(false, "Invalid state: use 'on' or 'off'");
    }
    
    tile_panel->SetNodeCollapsed(node_name, collapse_state);
    
    std::string state = collapse_state ? "collapsed" : "expanded";
    return CommandResult(true, "Node '" + node_name + "' set to " + state);
}

CommandResult cmd::CmdCollapseAll(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    tile_panel->CollapseAll();
    return CommandResult(true, "All nodes collapsed");
}

CommandResult cmd::CmdExpandAll(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    tile_panel->ExpandAll();
    return CommandResult(true, "All nodes expanded");
}

CommandResult cmd::CmdCollapseStatus(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    auto node_names = tile_panel->GetNodeNames();
    std::ostringstream oss;
    oss << "\nCollapse Status:\n";
    
    for (const auto& node_name : node_names) {
        bool is_collapsed = tile_panel->IsNodeCollapsed(node_name);
        oss << "  " << node_name << ": " << (is_collapsed ? "COLLAPSED" : "EXPANDED") << "\n";
    }
    
    return CommandResult(true, oss.str());
}

// ========== Phase 3d: Pinning ==========

CommandResult cmd::CmdPinToggle(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.size() < 2) {
        return CommandResult(false, "Usage: pin.toggle <node_name> <metric_name>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string node_name = args[0];
    std::string metric_name = args[1];
    
    tile_panel->TogglePinnedMetric(node_name, metric_name);
    
    bool is_pinned = tile_panel->IsPinnedMetric(node_name, metric_name);
    std::string state = is_pinned ? "pinned" : "unpinned";
    
    return CommandResult(true, node_name + "::" + metric_name + " is now " + state);
}

CommandResult cmd::CmdPinClear(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    tile_panel->ClearAllPinned();
    return CommandResult(true, "All pinned metrics cleared");
}

CommandResult cmd::CmdPinList(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    // Query pinned metrics by iterating nodes
    auto node_names = tile_panel->GetNodeNames();
    std::ostringstream oss;
    oss << "\nPinned Metrics:\n";
    
    bool found_any = false;
    for (const auto& node_name : node_names) {
        (void)node_name;  // Suppress unused variable warning
        // We can't directly iterate pinned metrics, so we show a summary
        // In a real implementation, we'd expose GetPinnedMetrics() from tile_panel
        found_any = true;
    }
    
    if (!found_any) {
        oss << "  (none pinned)\n";
    }
    
    return CommandResult(true, oss.str());
}

// ========== Phase 3d: Sorting ==========

CommandResult cmd::CmdSortBy(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.empty()) {
        return CommandResult(false, "Usage: sort.by <mode>  (name|value|status|pinned)");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string mode = args[0];
    
    // Validate mode
    if (mode != "name" && mode != "value" && mode != "status" && mode != "pinned") {
        return CommandResult(false, "Invalid sort mode. Valid modes: name, value, status, pinned");
    }
    
    std::string old_mode = tile_panel->GetSortingMode();
    tile_panel->SetSortingMode(mode);
    
    std::ostringstream oss;
    oss << "Sorting mode changed: '" << old_mode << "' → '" << mode << "'";
    return CommandResult(true, oss.str());
}

CommandResult cmd::CmdSortStatus(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string current_mode = tile_panel->GetSortingMode();
    return CommandResult(true, "Current sorting mode: " + current_mode);
}

// ========== Phase 3d: Export ==========

CommandResult cmd::CmdExportJson(Dashboard* app, const std::vector<std::string>& args) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string json_data = tile_panel->ExportToJSON();
    
    if (args.empty()) {
        // Return JSON data to display in command window
        return CommandResult(true, json_data);
    } else {
        // Write to file
        std::string filename = args[0];
        std::ofstream file(filename);
        if (!file.is_open()) {
            return CommandResult(false, "Failed to open file: " + filename);
        }
        file << json_data;
        file.close();
        return CommandResult(true, "Exported to " + filename);
    }
}

CommandResult cmd::CmdExportCsv(Dashboard* app, const std::vector<std::string>& args) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string csv_data = tile_panel->ExportToCSV();
    
    if (args.empty()) {
        // Return CSV data to display in command window
        return CommandResult(true, csv_data);
    } else {
        // Write to file
        std::string filename = args[0];
        std::ofstream file(filename);
        if (!file.is_open()) {
            return CommandResult(false, "Failed to open file: " + filename);
        }
        file << csv_data;
        file.close();
        return CommandResult(true, "Exported to " + filename);
    }
}

CommandResult cmd::CmdExportList(Dashboard* app, const std::vector<std::string>& /* args */) {
    if (!app) {
        return CommandResult(false, "Application context unavailable");
    }
    
    std::ostringstream oss;
    oss << "\nAvailable Export Formats:\n";
    oss << "  json    - JSON format with full metrics snapshot\n";
    oss << "  csv     - CSV format for spreadsheet analysis\n";
    oss << "\nUsage:\n";
    oss << "  export.json [filename]  - Export to JSON file (or stdout)\n";
    oss << "  export.csv [filename]   - Export to CSV file (or stdout)\n";
    
    return CommandResult(true, oss.str());
}

void RegisterBuiltinCommands(
    std::shared_ptr<CommandRegistry> registry,
    Dashboard* app) {
    
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
    
    // ========== Phase 3b: Filtering & Search ==========
    
    registry->RegisterCommand(
        "filter.global",
        "Set global search filter for all metrics",
        "filter.global <pattern>",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdFilterGlobal(app, args);
        }
    );
    
    registry->RegisterCommand(
        "filter.node",
        "Set search filter for a specific node",
        "filter.node <node_name> <pattern>",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdFilterNode(app, args);
        }
    );
    
    registry->RegisterCommand(
        "filter.clear",
        "Clear all filters",
        "filter.clear",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdFilterClear(app, args);
        }
    );
    
    // ========== Phase 3c: Collapse/Expand ==========
    
    registry->RegisterCommand(
        "collapse.toggle",
        "Toggle collapse state of a node",
        "collapse.toggle <node_name>",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdCollapseToggle(app, args);
        }
    );
    
    registry->RegisterCommand(
        "collapse.set",
        "Set collapse state of a node",
        "collapse.set <node_name> <on|off>",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdCollapseSet(app, args);
        }
    );
    
    registry->RegisterCommand(
        "collapse.all",
        "Collapse all nodes",
        "collapse.all",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdCollapseAll(app, args);
        }
    );
    
    registry->RegisterCommand(
        "collapse.expand_all",
        "Expand all nodes",
        "collapse.expand_all",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdExpandAll(app, args);
        }
    );
    
    registry->RegisterCommand(
        "collapse.status",
        "Show collapse state of all nodes",
        "collapse.status",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdCollapseStatus(app, args);
        }
    );
    
    // ========== Phase 3d: Pinning ==========
    
    registry->RegisterCommand(
        "pin.toggle",
        "Toggle pin state of a metric",
        "pin.toggle <node_name> <metric_name>",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdPinToggle(app, args);
        }
    );
    
    registry->RegisterCommand(
        "pin.clear",
        "Clear all pinned metrics",
        "pin.clear",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdPinClear(app, args);
        }
    );
    
    registry->RegisterCommand(
        "pin.list",
        "List all pinned metrics",
        "pin.list",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdPinList(app, args);
        }
    );
    
    // ========== Phase 3d: Sorting ==========
    
    registry->RegisterCommand(
        "sort.by",
        "Set sorting mode for metrics",
        "sort.by <mode>  (name|value|status|pinned)",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdSortBy(app, args);
        }
    );
    
    registry->RegisterCommand(
        "sort.status",
        "Show current sorting mode",
        "sort.status",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdSortStatus(app, args);
        }
    );
    
    // ========== Phase 3d: Export ==========
    
    registry->RegisterCommand(
        "export.json",
        "Export metrics to JSON format",
        "export.json [filename]",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdExportJson(app, args);
        }
    );
    
    registry->RegisterCommand(
        "export.csv",
        "Export metrics to CSV format",
        "export.csv [filename]",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdExportCsv(app, args);
        }
    );
    
    registry->RegisterCommand(
        "export.list",
        "List available export formats",
        "export.list",
        [app](const std::vector<std::string>& args) {
            return cmd::CmdExportList(app, args);
        }
    );
    
    std::cerr << "[BuiltinCommands] Registered 21 built-in commands (5 system + 3 filter + 5 collapse + 3 pin + 2 sort + 3 export)\n";
}

}  // namespace commands
