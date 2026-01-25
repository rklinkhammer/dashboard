#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <vector>

// Forward declaration
class CommandRegistry;

// Result from executing a command
struct CommandResult {
    bool success;
    std::string message;
    
    CommandResult(bool ok, const std::string& msg) 
        : success(ok), message(msg) {}
};

// Command handler signature
using CommandHandler = std::function<CommandResult(const std::vector<std::string>&)>;

// Command metadata
struct CommandInfo {
    std::string name;
    std::string description;
    std::string usage;  // e.g., "run_graph [timeout_ms]"
    CommandHandler handler;
};

// Extensible command registry for dashboard
class CommandRegistry {
public:
    CommandRegistry() = default;
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
    std::string GenerateHelpText() const;

private:
    // Store commands in insertion order
    std::map<std::string, CommandInfo> commands_;
};
