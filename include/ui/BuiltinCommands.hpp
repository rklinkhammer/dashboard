#pragma once

#include "CommandRegistry.hpp"
#include <memory>
#include <string>

// Forward declarations
class DashboardApplication;

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
    DashboardApplication* app);

// Individual command implementations
namespace cmd {

CommandResult CmdStatus(DashboardApplication* app, const std::vector<std::string>& args);
CommandResult CmdRunGraph(DashboardApplication* app, const std::vector<std::string>& args);
CommandResult CmdPauseGraph(DashboardApplication* app, const std::vector<std::string>& args);
CommandResult CmdStopGraph(DashboardApplication* app, const std::vector<std::string>& args);
CommandResult CmdResetLayout(DashboardApplication* app, const std::vector<std::string>& args);

}  // namespace cmd

}  // namespace commands
