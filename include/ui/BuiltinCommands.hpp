#pragma once

#include "CommandRegistry.hpp"
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

CommandResult CmdStatus(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdRunGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdPauseGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdStopGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdResetLayout(Dashboard* app, const std::vector<std::string>& args);

}  // namespace cmd

}  // namespace commands
