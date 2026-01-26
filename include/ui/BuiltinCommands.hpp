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

// System commands
CommandResult CmdStatus(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdRunGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdPauseGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdStopGraph(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdResetLayout(Dashboard* app, const std::vector<std::string>& args);

// Phase 3b: Filtering & Search
CommandResult CmdFilterGlobal(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdFilterNode(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdFilterClear(Dashboard* app, const std::vector<std::string>& args);

// Phase 3c: Collapse/Expand
CommandResult CmdCollapseToggle(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdCollapseSet(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdCollapseAll(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdExpandAll(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdCollapseStatus(Dashboard* app, const std::vector<std::string>& args);

// Phase 3d: Pinning
CommandResult CmdPinToggle(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdPinClear(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdPinList(Dashboard* app, const std::vector<std::string>& args);

// Phase 3d: Sorting
CommandResult CmdSortBy(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdSortStatus(Dashboard* app, const std::vector<std::string>& args);

// Phase 3d: Export
CommandResult CmdExportJson(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdExportCsv(Dashboard* app, const std::vector<std::string>& args);
CommandResult CmdExportList(Dashboard* app, const std::vector<std::string>& args);

}  // namespace cmd

}  // namespace commands
