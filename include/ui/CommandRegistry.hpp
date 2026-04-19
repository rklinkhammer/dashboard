// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include "app/capabilities/GraphCapability.hpp"

// Forward declaration
class CommandRegistry;
class Dashboard;

/**
 * @struct CommandResult
 * @brief Result of executing a dashboard command
 *
 * Indicates whether the command succeeded and provides output/error message.
 */
struct CommandResult {
    bool success;       ///< Whether command execution succeeded
    std::string message;///< Output message or error description
    
    /**
     * @brief Construct a command result
     *
     * @param ok Success status
     * @param msg Output or error message (optional)
     */
    CommandResult(bool ok, const std::string& msg="") 
        : success(ok), message(msg) {}
};

/**
 * @typedef CommandHandler
 * @brief Function signature for command implementations
 *
 * Takes a vector of arguments (first element is command name) and returns
 * a CommandResult indicating success and output message.
 *
 * @param args Command and arguments (args[0] is command name)
 * @return CommandResult with execution status and output
 *
 * @example
 *   CommandHandler handler = [&](const std::vector<std::string>& args) {
 *       if (args.size() < 2) return CommandResult(false, "Missing argument");
 *       int value = std::stoi(args[1]);
 *       return CommandResult(true, "Set to " + std::to_string(value));
 *   };
 */
using CommandHandler = std::function<CommandResult(const std::vector<std::string>&)>;

/**
 * @struct CommandInfo
 * @brief Metadata for a registered command
 *
 * Contains command name, description, usage information, and the handler function.
 */
struct CommandInfo {
    std::string name;           ///< Command name (e.g., "help", "run")
    std::string description;    ///< One-line description of what command does
    std::string usage;          ///< Usage format (e.g., "run_graph [timeout_ms]")
    CommandHandler handler;     ///< Function to execute when command is invoked
};

/**
 * @class CommandRegistry
 * @brief Extensible command registry for dashboard user input
 *
 * CommandRegistry manages a set of available commands that users can execute
 * from the dashboard's command window. Commands are registered with metadata
 * (name, description, usage) and a handler function.
 *
 * Built-in commands include:
 * - `help` - Display available commands
 * - `run` - Start graph execution
 * - `stop` - Stop graph execution
 * - `status` - Display execution status
 * - `filter_metrics` - Filter displayed metrics by pattern
 * - And others
 *
 * Custom commands can be registered programmatically. The registry is designed
 * to be extensible for domain-specific needs.
 *
 * @see CommandResult, CommandInfo
 *
 * @example
 *   auto registry = std::make_shared<CommandRegistry>();
 *   
 *   // Register built-in commands
 *   registry->RegisterBuiltinCommands(dashboard);
 *   
 *   // Register custom command
 *   registry->RegisterCommand(
 *       "my_command",
 *       "Does something custom",
 *       "Usage: my_command <arg>",
 *       [](const auto& args) {
 *           return CommandResult(true, "Command executed");
 *       }
 *   );
 *   
 *   // Execute command
 *   auto result = registry->ExecuteCommand("my_command arg1 arg2");
 */
class CommandRegistry {
public:
    /**
     * @brief Construct an empty command registry
     *
     * Use RegisterBuiltinCommands() and RegisterCommand() to add commands.
     */
    CommandRegistry() = default;
    
    /**
     * @brief Destructor
     */
    ~CommandRegistry() = default;

    // Register a command with handler
    // Returns false if command already exists
    bool RegisterCommand(
        const std::string& name,
        const std::string& description,
        const std::string& usage,
        CommandHandler handler);

    // Execute a command by name with arguments
    CommandResult ExecuteCommand(
        const std::string& name,
        const std::vector<std::string>& args);

    // Get all registered commands
    std::vector<CommandInfo> GetAllCommands() const;

    // Check if command exists
    bool HasCommand(const std::string& name) const;

    // Get command info
    const CommandInfo* GetCommandInfo(const std::string& name) const;

    // Generate help text for all commands
    void GenerateHelpText(Dashboard* dashboard) const;

private:
    // Store commands in insertion order
    std::map<std::string, CommandInfo> commands_;
};
