#include "ui/CommandRegistry.hpp"
#include "ui/Dashboard.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

bool CommandRegistry::RegisterCommand(
    const std::string& name,
    const std::string& description,
    const std::string& usage,
    CommandHandler handler) {
    
    // Check if command already exists
    if (commands_.find(name) != commands_.end()) {
        std::cerr << "[CommandRegistry] Warning: Command '" << name << "' already registered\n";
        return false;
    }
    
    // Check for empty name or handler
    if (name.empty() || !handler) {
        std::cerr << "[CommandRegistry] Error: Invalid command name or handler\n";
        return false;
    }
    
    // Register command
    CommandInfo cmd{name, description, usage, handler};
    commands_[name] = cmd;
    
    std::cerr << "[CommandRegistry] Registered command: " << name << "\n";
    return true;
}

CommandResult CommandRegistry::ExecuteCommand(
    const std::string& name,
    const std::vector<std::string>& args) {
    
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        return CommandResult(false, "Command not found: " + name + " 'help' for available commands");
    }
    
    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        return CommandResult(false, std::string("Error executing command: ") + e.what());
    }
}

std::vector<CommandInfo> CommandRegistry::GetAllCommands() const {
    std::vector<CommandInfo> result;
    for (const auto& [name, info] : commands_) {
        result.push_back(info);
    }
    return result;
}

bool CommandRegistry::HasCommand(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

const CommandInfo* CommandRegistry::GetCommandInfo(const std::string& name) const {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return &it->second;
    }
    return nullptr;
}

void CommandRegistry::GenerateHelpText(Dashboard* dashboard) const {
     
    for (const auto& [name, info] : commands_) {
        dashboard->AddLog(name + " - " + info.description);
        dashboard->AddLog("   Usage: " + info.usage);
    }
}
