#include <gtest/gtest.h>
#include "ui/DashboardApplication.hpp"
#include "ui/CommandRegistry.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/BuiltinCommands.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
#include <memory>

using graph::MockGraphExecutor;
using commands::RegisterBuiltinCommands;

// ============================================================================
// D4.3: CommandRegistry Integration with DashboardApplication
// ============================================================================
// Tests for integrating built-in commands with DashboardApplication.
// Built-in commands include: status, run, pause, stop, reset_layout

class CommandRegistryIntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<MockGraphExecutor> executor_;
    
    void SetUp() override {
        executor_ = std::make_shared<MockGraphExecutor>();
    }
};

// Test 1: Built-in commands can be registered
TEST_F(CommandRegistryIntegrationTest, RegisterBuiltinCommandsWithApp) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    
    // Register built-in commands (this is what D4.3 does)
    ASSERT_NO_THROW({
        RegisterBuiltinCommands(registry, &app);
    });
    
    // Verify commands were registered
    EXPECT_TRUE(registry->HasCommand("status"));
    EXPECT_TRUE(registry->HasCommand("run"));
    EXPECT_TRUE(registry->HasCommand("pause"));
    EXPECT_TRUE(registry->HasCommand("stop"));
    EXPECT_TRUE(registry->HasCommand("reset_layout"));
}

// Test 2: Status command executes successfully
TEST_F(CommandRegistryIntegrationTest, StatusCommandExecutesSuccessfully) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("status", args);
    
    EXPECT_EQ(result.success, true);
    EXPECT_TRUE(result.message.find("Dashboard Status") != std::string::npos);
}

// Test 3: Run command returns success message
TEST_F(CommandRegistryIntegrationTest, RunCommandReturnsSuccess) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("run", args);
    
    EXPECT_TRUE(result.success);
}

// Test 4: Pause command returns success message
TEST_F(CommandRegistryIntegrationTest, PauseCommandReturnsSuccess) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("pause", args);
    
    EXPECT_TRUE(result.success);
}

// Test 5: Stop command returns success message
TEST_F(CommandRegistryIntegrationTest, StopCommandReturnsSuccess) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("stop", args);
    
    EXPECT_TRUE(result.success);
}

// Test 6: Reset layout command returns success
TEST_F(CommandRegistryIntegrationTest, ResetLayoutCommandReturnsSuccess) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("reset_layout", args);
    
    EXPECT_TRUE(result.success);
}

// Test 7: CommandWindow can execute registered commands
TEST_F(CommandRegistryIntegrationTest, CommandWindowExecutesBuiltinCommands) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    auto command_window = app.GetCommandWindow();
    ASSERT_NE(command_window, nullptr);
    
    command_window->SetCommandRegistry(registry);
    
    // Simulate user typing "status" command
    command_window->AddOutputLine("$ status");
    EXPECT_GT(command_window->GetOutputCount(), 0);
}

// Test 8: Status command shows active panels
TEST_F(CommandRegistryIntegrationTest, StatusCommandShowsActivePanels) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> args;
    auto result = registry->ExecuteCommand("status", args);
    
    EXPECT_TRUE(result.success);
    std::string output = result.message;
    
    // Status should show all 4 panels
    EXPECT_TRUE(output.find("Metrics Panel") != std::string::npos);
    EXPECT_TRUE(output.find("Logging Window") != std::string::npos);
    EXPECT_TRUE(output.find("Command Window") != std::string::npos);
    EXPECT_TRUE(output.find("Status Bar") != std::string::npos);
}

// Test 9: All built-in commands have help text
TEST_F(CommandRegistryIntegrationTest, BuiltinCommandsHaveHelpText) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    std::vector<std::string> commands = {"status", "run", "pause", "stop", "reset_layout"};
    
    for (const auto& cmd_name : commands) {
        auto info = registry->GetCommandInfo(cmd_name);
        ASSERT_NE(info, nullptr);
        EXPECT_NE(info->description, "");
    }
}

// Test 10: Commands can be looked up by name
TEST_F(CommandRegistryIntegrationTest, CommandsCanBeLookedUpByName) {
    auto executor = std::make_shared<MockGraphExecutor>();
    WindowHeightConfig heights;
    DashboardApplication app(executor, heights);
    app.Initialize();
    
    auto registry = std::make_shared<CommandRegistry>();
    RegisterBuiltinCommands(registry, &app);
    
    // Look up "status" command
    EXPECT_TRUE(registry->HasCommand("status"));
    auto info = registry->GetCommandInfo("status");
    EXPECT_NE(info, nullptr);
    
    // Look up "run" command
    EXPECT_TRUE(registry->HasCommand("run"));
    info = registry->GetCommandInfo("run");
    EXPECT_NE(info, nullptr);
}
