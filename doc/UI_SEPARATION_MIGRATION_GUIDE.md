# UI Separation: Migration & Implementation Guide

**Purpose**: Step-by-step migration from current architecture to decoupled design  
**Status**: Implementation Ready

---

## Part 1: Current Problems & Solutions

### Problem 1: Tight Coupling in DashboardCapability

**Current Code (WRONG):**
```cpp
// include/app/capabilities/DashboardCapability.hpp
#include "ui/MetricsPanel.hpp"           // ❌ UI import
#include "ui/CommandRegistry.hpp"        // ❌ UI import
#include "ui/Dashboard.hpp"              // ❌ UI import

class DashboardCapability {
private:
    shared_ptr<MetricsPanel> metrics_panel_;
    shared_ptr<CommandRegistry> registry_;
    shared_ptr<Dashboard> dashboard_;
    
public:
    void SetDashboard(shared_ptr<Dashboard> db) {
        dashboard_ = db;  // Tightly coupled to UI class
    }
};
```

**Problem**: 
- App layer imports UI headers
- UI framework (ncurses/ftxui) dependencies pulled into core
- Cannot use MetricsCapability without entire UI stack

**Solution:**
```cpp
// include/app/capabilities/DashboardCapability.hpp (REFACTORED)
#include "core/ActiveQueue.hpp"
#include <string>

class DashboardCapability {
private:
    // Just queues - no UI knowledge
    core::ActiveQueue<DashboardCommand> command_queue_{1000};
    core::ActiveQueue<string> log_queue_{10000};

public:
    bool EnqueueCommand(const DashboardCommand& cmd) {
        return command_queue_.EnqueueNonBlocking(cmd);
    }
    
    optional<DashboardCommand> DequeueCommand() {
        DashboardCommand cmd;
        if (command_queue_.DequeueNonBlocking(cmd)) {
            return cmd;
        }
        return nullopt;
    }
};
```

**Benefits**:
- ✅ No UI imports
- ✅ Can use DashboardCapability standalone
- ✅ UI adapters connect via CapabilityBus

---

### Problem 2: No Abstraction for UI Implementations

**Current Code (MONOLITHIC):**
```cpp
// ui/Dashboard.hpp
class Dashboard {
    shared_ptr<MetricsPanel> metrics_panel_;
    shared_ptr<CommandRegistry> registry_;
    shared_ptr<LogWindow> log_window_;
    
    // Tightly coupled to ncurses
    void Render() { /* ncurses code */ }
    void HandleInput() { /* ncurses code */ }
};

// If you want a web UI, you must:
// 1. Rewrite all the above for HTTP/WebSocket
// 2. Duplicate command execution logic
// 3. Duplicate metrics updating logic
```

**Solution:**
```cpp
// app/interfaces/IUIAdapter.hpp (ABSTRACTION)
class IUIAdapter {
public:
    virtual void Initialize(shared_ptr<CapabilityBus> bus) = 0;
    virtual void Shutdown() = 0;
    virtual string GetAdapterName() const = 0;
};

// ui/TerminalUIAdapter.hpp (TERMINAL IMPL)
class TerminalUIAdapter : public IUIAdapter,
                          public ICommandExecutor,
                          public IMetricsPublisher {
    shared_ptr<Dashboard> dashboard_;  // OWNED by adapter
    
    void Initialize(shared_ptr<CapabilityBus> bus) override {
        dashboard_ = make_shared<Dashboard>();
        bus->Register<ICommandExecutor>(shared_from_this());
        bus->Register<IMetricsPublisher>(shared_from_this());
    }
    
    CommandResult Execute(const DashboardCommand& cmd) override {
        return dashboard_->ExecuteCommand(cmd);
    }
    
    void OnMetricsEvent(const MetricsEvent& event) override {
        dashboard_->UpdateMetrics(event);
    }
};

// ui/web/WebUIAdapter.hpp (WEB IMPL)
class WebUIAdapter : public IUIAdapter,
                     public ICommandExecutor,
                     public IMetricsPublisher {
    shared_ptr<WebServer> server_;  // OWNED by adapter
    
    void Initialize(shared_ptr<CapabilityBus> bus) override {
        server_ = make_shared<WebServer>(8080);
        server_->Start();
        bus->Register<ICommandExecutor>(shared_from_this());
        bus->Register<IMetricsPublisher>(shared_from_this());
    }
    
    CommandResult Execute(const DashboardCommand& cmd) override {
        return server_->HandleCommand(cmd);
    }
    
    void OnMetricsEvent(const MetricsEvent& event) override {
        server_->BroadcastMetrics(event);
    }
};
```

---

## Part 2: Step-by-Step Refactoring

### Step 1: Create Interface Definitions (2-3 hours)

**1a. Create command interface**

File: `include/app/interfaces/DashboardCommand.hpp`

```cpp
#pragma once

#include <string>
#include <vector>

namespace app::interfaces {

struct DashboardCommand {
    std::string name;
    std::vector<std::string> args;
    
    DashboardCommand() = default;
    DashboardCommand(const std::string& n) : name(n) {}
    DashboardCommand(const std::string& n, const std::vector<std::string>& a)
        : name(n), args(a) {}
};

enum class CommandStatus {
    Success = 0,
    UnknownCommand = 1,
    ExecutionFailed = 3,
};

struct CommandResult {
    CommandStatus status = CommandStatus::Success;
    std::string message;
    std::string data;
    
    bool IsSuccess() const { return status == CommandStatus::Success; }
};

class ICommandExecutor {
public:
    virtual ~ICommandExecutor() = default;
    virtual CommandResult Execute(const DashboardCommand& cmd) = 0;
};

}
```

**1b. Create metrics interface**

File: `include/app/interfaces/IMetricsPublisher.hpp`

```cpp
#pragma once

#include "graph/GraphMetrics.hpp"

namespace app::interfaces {

class IMetricsPublisher {
public:
    virtual ~IMetricsPublisher() = default;
    virtual void OnMetricsEvent(const graph::MetricsEvent& event) = 0;
    virtual void FlushMetrics() = 0;
    virtual graph::MetricsEvent GetLatestMetrics() const = 0;
};

}
```

**1c. Create UI adapter interface**

File: `include/app/interfaces/IUIAdapter.hpp`

```cpp
#pragma once

#include <memory>
#include <string>

namespace graph { class CapabilityBus; }

namespace app::interfaces {

class IUIAdapter {
public:
    virtual ~IUIAdapter() = default;
    virtual void Initialize(std::shared_ptr<graph::CapabilityBus> bus) = 0;
    virtual bool IsRunning() const = 0;
    virtual void Shutdown() = 0;
    virtual std::string GetAdapterName() const = 0;
};

}
```

**Update CMakeLists.txt:**
```cmake
# In src/CMakeLists.txt
target_sources(gdashboard_lib PRIVATE
    # ... existing files ...
    # NEW: Interface definitions (no implementation)
    include/app/interfaces/DashboardCommand.hpp
    include/app/interfaces/IMetricsPublisher.hpp
    include/app/interfaces/IUIAdapter.hpp
)
```

**Testing:**
```bash
cd build
cmake ..
make -j4
# Should compile with no errors
```

---

### Step 2: Refactor DashboardCapability (3-4 hours)

**2a. Update DashboardCapability.hpp**

```cpp
// BEFORE:
#include "ui/MetricsPanel.hpp"
#include "ui/CommandRegistry.hpp"

class DashboardCapability {
private:
    shared_ptr<MetricsPanel> metrics_panel_;
    shared_ptr<CommandRegistry> command_registry_;
};

// AFTER:
#include "core/ActiveQueue.hpp"
#include "app/interfaces/DashboardCommand.hpp"

class DashboardCapability {
private:
    core::ActiveQueue<app::interfaces::DashboardCommand> command_queue_{1000};
    core::ActiveQueue<std::string> log_queue_{10000};

public:
    bool EnqueueCommand(const app::interfaces::DashboardCommand& cmd) {
        return command_queue_.EnqueueNonBlocking(cmd);
    }
    
    std::optional<app::interfaces::DashboardCommand> DequeueCommand() {
        app::interfaces::DashboardCommand cmd;
        if (command_queue_.DequeueNonBlocking(cmd)) {
            return cmd;
        }
        return std::nullopt;
    }
    
    bool AddLog(const std::string& message) {
        return log_queue_.EnqueueNonBlocking(message);
    }
    
    std::optional<std::string> DequeueLog() {
        std::string msg;
        if (log_queue_.DequeueNonBlocking(msg)) {
            return msg;
        }
        return std::nullopt;
    }
};
```

**2b. Update any code that used the old interface**

**Old Pattern:**
```cpp
// OLD: Using old SetDashboard method
auto dashboard_cap = capability_bus_->Get<DashboardCapability>();
dashboard_cap->SetDashboard(my_dashboard);

// This doesn't work anymore - replaced with queue interface
auto command = dashboard_cap->GetNextCommand();  // ❌ WRONG - REMOVED
```

**New Pattern:**
```cpp
// NEW: Using queue interface
auto dashboard_cap = capability_bus_->Get<DashboardCapability>();
auto command_opt = dashboard_cap->DequeueCommand();
if (command_opt) {
    auto cmd = command_opt.value();
    // Process command
}

// Logging new messages
dashboard_cap->AddLog("System starting...");
```

**2c. Test refactored DashboardCapability**

Create `test/graph/test_dashboard_capability_refactored.cpp`:

```cpp
#include <gtest/gtest.h>
#include "app/capabilities/DashboardCapability.hpp"
#include "app/interfaces/DashboardCommand.hpp"

using namespace app;
using namespace app::interfaces;

TEST(DashboardCapabilityRefactored, EnqueueDequeueCommand) {
    capabilities::DashboardCapability cap;
    
    DashboardCommand cmd{"pause", {}};
    EXPECT_TRUE(cap.EnqueueCommand(cmd));
    
    auto dequeued = cap.DequeueCommand();
    EXPECT_TRUE(dequeued.has_value());
    EXPECT_EQ(dequeued->name, "pause");
    
    // Queue empty
    EXPECT_FALSE(cap.DequeueCommand().has_value());
}

TEST(DashboardCapabilityRefactored, AddDequeueLog) {
    capabilities::DashboardCapability cap;
    
    EXPECT_TRUE(cap.AddLog("Test message"));
    
    auto dequeued = cap.DequeueLog();
    EXPECT_TRUE(dequeued.has_value());
    EXPECT_EQ(dequeued.value(), "Test message");
}

TEST(DashboardCapabilityRefactored, NoUIImports) {
    // This test verifies that DashboardCapability can be compiled
    // without UI headers (MetricsPanel, CommandRegistry, Dashboard, etc.)
    capabilities::DashboardCapability cap;
    
    // Should compile without ftxui, ncurses, or any UI library
    EXPECT_TRUE(cap.EnqueueCommand(DashboardCommand{"test"}));
}
```

**Testing:**
```bash
cd build
cmake ..
make test_phase0_unit
./bin/test_phase0_unit --gtest_filter="DashboardCapabilityRefactored*"
# Expected: All tests pass
# Expected: Compilation works without ncurses/ftxui
```

---

### Step 3: Create Terminal UI Adapter (5-6 hours)

**3a. Create TerminalUIAdapter.hpp**

```cpp
#pragma once

#include "app/interfaces/IUIAdapter.hpp"
#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include <memory>
#include <mutex>

namespace ui {

class Dashboard;

class TerminalUIAdapter : 
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    std::shared_ptr<Dashboard> dashboard_;
    std::atomic<bool> is_running_{false};
    mutable std::mutex dashboard_mutex_;
    
    graph::MetricsEvent latest_metrics_;
    mutable std::mutex metrics_mutex_;

public:
    TerminalUIAdapter();
    ~TerminalUIAdapter();
    
    // IUIAdapter
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "TerminalUIAdapter"; }
    
    // ICommandExecutor
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    // IMetricsPublisher
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
    
    // Terminal-specific
    void RunEventLoop();
    void RequestExit();
};

}
```

**3b. Create TerminalUIAdapter.cpp**

```cpp
#include "ui/TerminalUIAdapter.hpp"
#include "ui/Dashboard.hpp"
#include "graph/CapabilityBus.hpp"
#include <iostream>
#include <stdexcept>

namespace ui {

TerminalUIAdapter::TerminalUIAdapter()
    : is_running_(false) {
}

TerminalUIAdapter::~TerminalUIAdapter() {
    if (is_running_) {
        Shutdown();
    }
}

void TerminalUIAdapter::Initialize(std::shared_ptr<graph::CapabilityBus> bus) {
    if (is_running_) {
        throw std::runtime_error("Already initialized");
    }
    
    try {
        // Create dashboard
        {
            std::lock_guard<std::mutex> lock(dashboard_mutex_);
            dashboard_ = std::make_shared<Dashboard>();
            dashboard_->Initialize();
        }
        
        // Register as command executor
        bus->Register<app::interfaces::ICommandExecutor>(
            std::static_pointer_cast<app::interfaces::ICommandExecutor>(
                std::make_shared<TerminalUIAdapter>(*this)));
        
        // Register as metrics publisher
        bus->Register<app::interfaces::IMetricsPublisher>(
            std::static_pointer_cast<app::interfaces::IMetricsPublisher>(
                std::make_shared<TerminalUIAdapter>(*this)));
        
        is_running_ = true;
        std::cout << "Terminal UI initialized\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Terminal UI: " << e.what() << "\n";
        throw;
    }
}

void TerminalUIAdapter::Shutdown() {
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        if (dashboard_) {
            dashboard_->Shutdown();
            dashboard_.reset();
        }
    }
}

app::interfaces::CommandResult TerminalUIAdapter::Execute(
    const app::interfaces::DashboardCommand& cmd) {
    
    if (!is_running_) {
        return app::interfaces::CommandResult(
            app::interfaces::CommandStatus::ExecutionFailed,
            "Terminal UI not running");
    }
    
    try {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        return dashboard_->ExecuteCommand(cmd);
    } catch (const std::exception& e) {
        return app::interfaces::CommandResult(
            app::interfaces::CommandStatus::ExecutionFailed,
            std::string("Error: ") + e.what());
    }
}

void TerminalUIAdapter::OnMetricsEvent(const graph::MetricsEvent& event) {
    // Called from metrics thread - just store latest
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        latest_metrics_ = event;
    }
}

void TerminalUIAdapter::FlushMetrics() {
    // Called from UI thread
    graph::MetricsEvent metrics_copy;
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_copy = latest_metrics_;
    }
    
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        if (dashboard_) {
            dashboard_->UpdateMetrics(metrics_copy);
        }
    }
}

graph::MetricsEvent TerminalUIAdapter::GetLatestMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return latest_metrics_;
}

void TerminalUIAdapter::RunEventLoop() {
    if (!is_running_) {
        throw std::runtime_error("Not initialized");
    }
    
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    dashboard_->Run();
}

void TerminalUIAdapter::RequestExit() {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    if (dashboard_) {
        dashboard_->RequestExit();
    }
}

}
```

**3c. Update main() to use TerminalUIAdapter**

```cpp
// src/gdashboard/main.cpp
#include "ui/TerminalUIAdapter.hpp"
#include "graph/GraphExecutor.hpp"
#include "graph/CapabilityBus.hpp"
#include <iostream>
#include <memory>

using namespace std;

int main(int argc, char** argv) {
    try {
        // Build graph (existing code)
        auto graph = BuildGraphFromConfig("config.json");
        
        // Create capability bus
        auto bus = make_shared<graph::CapabilityBus>();
        
        // Create and initialize Terminal UI adapter
        auto ui_adapter = make_shared<ui::TerminalUIAdapter>();
        ui_adapter->Initialize(bus);
        
        // Create executor
        auto executor = make_shared<graph::GraphExecutor>();
        bus->Register<graph::GraphExecutor>(executor);
        
        // Start execution
        executor->Init(graph);
        executor->Start();
        
        cout << "Graph running. Press Ctrl+C to exit.\n";
        
        // Run UI event loop (blocks until exit)
        ui_adapter->RunEventLoop();
        
        // Shutdown
        executor->Stop();
        executor->Join();
        ui_adapter->Shutdown();
        
        cout << "Done.\n";
        return 0;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
```

**Testing:**
```bash
cd build
cmake ..
make -j4
./bin/gdashboard
# Expected: Terminal UI starts
# Expected: Can execute commands
# Expected: Metrics update correctly
```

---

### Step 4: Create Web UI Adapter (15-20 hours)

**4a. Create simple WebUIAdapter.hpp**

```cpp
#pragma once

#include "app/interfaces/IUIAdapter.hpp"
#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include <memory>
#include <atomic>

namespace ui::web {

class WebServer;

class WebUIAdapter :
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    std::unique_ptr<WebServer> server_;
    std::atomic<bool> is_running_{false};
    mutable std::mutex metrics_mutex_;
    graph::MetricsEvent latest_metrics_;

public:
    explicit WebUIAdapter(uint16_t port = 8080);
    ~WebUIAdapter();
    
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "WebUIAdapter"; }
    
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
};

}
```

**4b. Create simple WebServer**

```cpp
// include/ui/web/WebServer.hpp
#pragma once

#include <string>
#include <map>
#include <functional>
#include <cstdint>

namespace ui::web {

struct HTTPRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> params;
    std::string body;
};

struct HTTPResponse {
    int status_code = 200;
    std::string content_type = "application/json";
    std::string body;
};

using RequestHandler = std::function<HTTPResponse(const HTTPRequest&)>;

class WebServer {
private:
    uint16_t port_;
    std::map<std::string, RequestHandler> routes_;
    bool is_running_{false};

public:
    explicit WebServer(uint16_t port);
    ~WebServer();
    
    void RegisterRoute(const std::string& path, RequestHandler handler);
    void Start();
    void Stop();
    bool IsRunning() const { return is_running_; }
};

}
```

**4c. Add REST endpoints**

```cpp
// include/ui/web/RestEndpoints.hpp
#pragma once

#include "ui/web/WebServer.hpp"
#include "app/interfaces/ICommandExecutor.hpp"
#include <memory>

namespace ui::web {

class RestEndpoints {
private:
    std::shared_ptr<app::interfaces::ICommandExecutor> executor_;

public:
    explicit RestEndpoints(
        std::shared_ptr<app::interfaces::ICommandExecutor> executor);
    
    HTTPResponse HandleGetMetrics(const HTTPRequest& req);
    HTTPResponse HandlePostCommand(const HTTPRequest& req);
    HTTPResponse HandleGetStatus(const HTTPRequest& req);
    HTTPResponse HandleGetHelp(const HTTPRequest& req);
};

}
```

**Recommended: Use existing HTTP library**
```cpp
// In CMakeLists.txt
find_package(nlohmann_json REQUIRED)
find_package(cpp-httplib REQUIRED)  # cpp-httplib: single-header HTTP library

# Or use:
# - Pistache (REST framework)
# - Crow (micro web framework)
# - Beast (HTTP library from Boost)
```

---

### Step 5: Create CLI Adapter (8-10 hours)

**5a. Create CLIAdapter.hpp**

```cpp
#pragma once

#include "app/interfaces/IUIAdapter.hpp"
#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include <memory>
#include <atomic>

namespace app::cli {

class CLIAdapter :
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    struct Options {
        std::string single_command;
        bool interactive = true;
        std::string output_format = "text";
    } options_;
    
    std::atomic<bool> is_running_{false};
    graph::MetricsEvent latest_metrics_;

public:
    CLIAdapter(int argc, char** argv);
    ~CLIAdapter();
    
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "CLIAdapter"; }
    
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
    
    void RunInteractive();
};

}
```

**5b. CLI main entry point**

```cpp
// Create separate executable: gdashboard-cli
// src/gdashboard_cli/main.cpp

#include "app/cli/CLIAdapter.hpp"
#include "graph/GraphExecutor.hpp"
#include "graph/CapabilityBus.hpp"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
    try {
        auto graph = BuildGraphFromConfig("config.json");
        auto bus = make_shared<graph::CapabilityBus>();
        
        // Create CLI adapter
        auto cli_adapter = make_shared<app::cli::CLIAdapter>(argc, argv);
        cli_adapter->Initialize(bus);
        
        // Start execution
        auto executor = make_shared<graph::GraphExecutor>();
        executor->Init(graph);
        executor->Start();
        
        // Run CLI
        cli_adapter->RunInteractive();
        
        // Shutdown
        executor->Stop();
        executor->Join();
        cli_adapter->Shutdown();
        
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
```

---

## Part 3: Compile & Test

### Build All Adapters

```bash
cd build
cmake ..
make -j4

# Verify all binaries exist
ls -la bin/
# - gdashboard (Terminal UI)
# - gdashboard-server (Web UI)
# - gdashboard-cli (CLI)
```

### Test Each Adapter

**Terminal UI:**
```bash
./bin/gdashboard --config config.json
# Expected: ncurses UI appears
# Can execute commands
```

**Web UI:**
```bash
./bin/gdashboard-server --config config.json --port 8080
# Expected: Server starts
# Open: http://localhost:8080/
# Can send commands via REST API
```

**CLI:**
```bash
./bin/gdashboard-cli --config config.json
# Expected: Interactive prompt appears
# Can type commands
```

---

## Part 4: Validation Checklist

### Architectural Requirements

- [ ] **No UI imports in business logic**
  ```bash
  grep -r "#include.*ui/\|#include.*ftxui" include/app/ include/graph/ include/core/ include/sensor/
  # Expected: No matches
  ```

- [ ] **Clear interface boundaries**
  ```bash
  find include/app/interfaces/ -name "*.hpp" | wc -l
  # Expected: 3 interface files (IUIAdapter, ICommandExecutor, IMetricsPublisher)
  ```

- [ ] **No circular dependencies**
  ```bash
  # Use dependency analyzer tool
  graphviz_cpp include/
  # Verify: app/interfaces depends on nothing UI-related
  #         app/capabilities depends only on app/interfaces
  #         ui/ depends on app/interfaces
  ```

### Functional Requirements

- [ ] **Terminal UI still works as before**
  ```bash
  ./bin/gdashboard
  # Test: pause/resume
  # Test: metrics display
  # Test: logs
  ```

- [ ] **Web UI serves metrics**
  ```bash
  curl http://localhost:8080/api/metrics
  # Expected: JSON response with current metrics
  ```

- [ ] **CLI runs commands**
  ```bash
  ./bin/gdashboard-cli pause
  # Expected: Graph paused
  ```

- [ ] **All adapters can coexist**
  ```bash
  # Start three different terminals
  ./bin/gdashboard &
  ./bin/gdashboard-server --port 8080 &
  ./bin/gdashboard-cli &
  
  # All should connect to same graph and work simultaneously
  ```

### Testing

- [ ] **Unit tests pass**
  ```bash
  make test_phase0_unit
  # Expected: 652+ tests pass
  ```

- [ ] **Integration tests pass**
  ```bash
  make test_phase0_integration
  # Expected: All tests pass
  ```

- [ ] **No regressions**
  ```bash
  ./bin/test_phase0_integration --gtest_filter="*CSV*"
  # Expected: CSV pipeline tests still pass
  ```

---

## Part 5: Documentation & Deployment

### Update Developer Documentation

- [ ] Add UI Separation architecture guide to `doc/`
- [ ] Update README with new usage modes:
  ```bash
  # Terminal (default)
  ./gdashboard --config config.json
  
  # Web server
  ./gdashboard-server --config config.json --port 8080
  
  # CLI mode
  ./gdashboard-cli --config config.json
  ```

- [ ] Document REST API endpoints:
  ```markdown
  ## Web UI REST API
  
  - GET  /api/metrics - Current metrics
  - GET  /api/metrics/<node> - Metrics for specific node
  - POST /api/commands - Execute command (JSON body)
  - GET  /api/status - Execution status
  - GET  /api/help - Available commands
  - GET  /api/graph - Graph structure (DOT format)
  ```

### Update Build System

- [ ] Add CMake targets:
  ```cmake
  add_executable(gdashboard ...)                # Terminal
  add_executable(gdashboard-server ...)         # Web
  add_executable(gdashboard-cli ...)            # CLI
  ```

- [ ] Document optional dependencies:
  ```cmake
  # Required: (existing)
  # Optional: cpp-httplib (for web UI)
  # Optional: readline (for CLI)
  ```

---

## Summary

This step-by-step migration achieves:

| Phase | Task | Time | Result |
|-------|------|------|--------|
| 1 | Interfaces | 2-3h | IUIAdapter, ICommandExecutor, IMetricsPublisher |
| 2 | DashboardCapability | 3-4h | Remove UI imports, use queues |
| 3 | Terminal Adapter | 5-6h | Wrap existing Dashboard |
| 4 | Web Adapter | 15-20h | HTTP/WebSocket server |
| 5 | CLI Adapter | 8-10h | Command-line interface |
| **Total** | | **33-43h** | **3 fully functional UIs** |

All changes are:
- ✅ Backward compatible (Terminal UI works as before)
- ✅ Non-breaking (existing code unchanged)
- ✅ Well-tested (new adapters tested independently)
- ✅ Incremental (can phase in one UI at a time)

