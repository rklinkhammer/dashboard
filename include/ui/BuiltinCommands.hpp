#pragma once

#include "ui/CommandRegistry.hpp"
#include <memory>
#include <string>

// Forward declarations
class Dashboard;

namespace commands {

/**
 * Register all built-in commands with the command registry.
 * These commands provide core dashboard functionality:
 * - help: Display available commands
 * - clear: Clear command window output
 * - status: Display dashboard status
 * - run_graph: Execute the computation graph
 * - pause: Pause graph execution
 * - stop: Stop graph execution
 * - reset_layout: Reset window layout to defaults
 */
void RegisterBuiltinCommands(
    std::shared_ptr<CommandRegistry> registry,
    Dashboard* app);

// Individual command implementations
namespace cmd {

// System commands
CommandResult CmdStatus(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdListMetrics(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdToggleSparklines(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdClearFilter(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdFilterMetrics(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdSetConfidence(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdShowEvents(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdShowHistory(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdSetMetric(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdClearMetrics(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdExportMetrics(Dashboard* app, const std::vector<std::string>& args);

}  // namespace cmd

}  // namespace commands
