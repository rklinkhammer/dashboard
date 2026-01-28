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

// Forward declaration
class CommandRegistry;
class Dashboard;

// Result from executing a command
struct CommandResult {
    bool success;
    std::string message;
    
    CommandResult(bool ok, const std::string& msg="") 
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
    void GenerateHelpText(Dashboard* dashboard) const;

private:
    // Store commands in insertion order
    std::map<std::string, CommandInfo> commands_;
};
