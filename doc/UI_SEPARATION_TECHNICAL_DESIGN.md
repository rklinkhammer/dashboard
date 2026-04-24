# UI Separation: Technical Design & Code Examples

**Purpose**: Detailed specifications for separating UI from business logic  
**Audience**: Implementation team  
**Status**: Design Phase

---

## 1. Core Interface Definitions

### 1.1 Command Execution Interface

**File: `include/app/interfaces/ICommandExecutor.hpp`**

```cpp
#pragma once

#include <string>
#include <vector>

namespace app::interfaces {

/**
 * @enum CommandStatus
 * Represents the result of command execution
 */
enum class CommandStatus {
    Success = 0,
    UnknownCommand = 1,
    InvalidArguments = 2,
    ExecutionFailed = 3,
    NotSupported = 4,
};

/**
 * @struct DashboardCommand
 * Encapsulates a user command with arguments
 */
struct DashboardCommand {
    /// Command name (e.g., "pause", "resume", "status")
    std::string name;
    
    /// Command arguments
    std::vector<std::string> args;
    
    /// Optional context metadata (UI-specific)
    std::string context_id;  // e.g., "metrics_panel_3"
    
    DashboardCommand() = default;
    DashboardCommand(const std::string& n, const std::vector<std::string>& a = {})
        : name(n), args(a) {}
};

/**
 * @struct CommandResult
 * Result of command execution
 */
struct CommandResult {
    /// Execution status
    CommandStatus status = CommandStatus::Success;
    
    /// Human-readable result message
    std::string message;
    
    /// Result data (JSON format for complex results)
    std::string data;
    
    /// Execution time in milliseconds
    uint64_t execution_time_ms = 0;
    
    CommandResult() = default;
    CommandResult(CommandStatus s, const std::string& msg = "")
        : status(s), message(msg) {}
    
    bool IsSuccess() const { return status == CommandStatus::Success; }
};

/**
 * @class ICommandExecutor
 * Abstract interface for executing dashboard commands
 * 
 * Implemented by:
 * - TerminalUIAdapter (routes through CommandRegistry)
 * - WebUIAdapter (routes through REST handlers)
 * - CLIAdapter (routes through CLI parser)
 * 
 * Thread Safety: Must be thread-safe (called from multiple UI threads)
 */
class ICommandExecutor {
public:
    virtual ~ICommandExecutor() = default;
    
    /**
     * Execute a dashboard command
     * @param cmd Command to execute
     * @return CommandResult with status and message
     * 
     * Supported commands (shared across all UIs):
     * 
     * System Control:
     *   - "pause"              - Pause graph execution
     *   - "resume"             - Resume graph execution
     *   - "stop"               - Stop and shutdown graph
     *   - "status"             - Get current execution status
     *   - "help" [command]     - Get help (all or specific command)
     * 
     * Metrics:
     *   - "metrics show" [node] - Show metrics for node(s)
     *   - "metrics clear"      - Clear metrics
     *   - "metrics export" <format> <file>  - Export to CSV/JSON
     * 
     * Logging:
     *   - "logs show" [count]  - Show recent logs
     *   - "logs filter" <pattern>  - Filter logs
     *   - "logs clear"         - Clear log buffer
     * 
     * Graph:
     *   - "graph show"         - Display graph structure
     *   - "graph export" <format> - Export as DOT/JSON/PNG
     * 
     * Node Control:
     *   - "node list"          - List all nodes
     *   - "node status" <node_id>  - Get node status
     *   - "node inject" <node_id> <data>  - Inject test data
     */
    virtual CommandResult Execute(const DashboardCommand& cmd) = 0;
};

}  // namespace app::interfaces
```

### 1.2 Metrics Publishing Interface

**File: `include/app/interfaces/IMetricsPublisher.hpp`**

```cpp
#pragma once

#include "graph/GraphMetrics.hpp"
#include <memory>

namespace app::interfaces {

/**
 * @class IMetricsPublisher
 * Abstract interface for publishing metrics to UI
 * 
 * Implemented by:
 * - TerminalUIAdapter (updates Dashboard UI)
 * - WebUIAdapter (sends WebSocket messages)
 * - CLIAdapter (prints to stdout)
 * 
 * Lifecycle:
 * 1. OnMetricsEvent() called by MetricsCapability (thread-safe)
 * 2. Publisher queues/buffers metrics
 * 3. FlushMetrics() called periodically to push to UI
 * 
 * Thread Safety: 
 * - OnMetricsEvent() called from MetricsPolicy thread
 * - FlushMetrics() called from UI thread
 * - Must use atomic or lock-free mechanisms
 */
class IMetricsPublisher {
public:
    virtual ~IMetricsPublisher() = default;
    
    /**
     * Called when a metrics event occurs
     * 
     * Called from: MetricsPolicy background thread
     * Frequency: Every metrics update (adjustable via config)
     * Thread Safety: MUST be thread-safe (called from different thread)
     * Latency Requirement: < 1ms (lock-free preferred)
     * 
     * @param event Metrics event with node metrics, edge metrics, timing
     */
    virtual void OnMetricsEvent(const graph::MetricsEvent& event) = 0;
    
    /**
     * Flush buffered metrics to UI
     * 
     * Called from: UI render thread (Terminal, Web, or CLI)
     * Frequency: Every UI update cycle (e.g., 100ms for terminal)
     * Thread Safety: Must coordinate with OnMetricsEvent
     * 
     * Purpose: Batch updates and reduce UI redraw frequency
     * 
     * After this call, metrics should be visible in:
     * - Dashboard render (Terminal)
     * - Latest JSON in API (Web)
     * - Console output (CLI)
     */
    virtual void FlushMetrics() = 0;
    
    /**
     * Optional: Get latest metrics without flushing
     * 
     * Used by: Web API to serve /metrics endpoint
     * Returns: Most recent MetricsEvent snapshot
     */
    virtual graph::MetricsEvent GetLatestMetrics() const = 0;
};

}  // namespace app::interfaces
```

### 1.3 UI Adapter Interface

**File: `include/app/interfaces/IUIAdapter.hpp`**

```cpp
#pragma once

#include <memory>

namespace graph { class CapabilityBus; }

namespace app::interfaces {

/**
 * @class IUIAdapter
 * Abstract interface for UI implementations
 * 
 * Lifecycle:
 * 1. Create adapter instance
 * 2. Call Initialize(bus) - adapter registers itself with bus
 * 3. Call Run() or block on UI event loop
 * 4. Call Shutdown() on exit
 * 
 * Responsibilities:
 * - Register ICommandExecutor implementation
 * - Register IMetricsPublisher implementation
 * - Manage UI-specific resources (windows, network, threads)
 * - Handle user input and produce DashboardCommand objects
 * - Display metrics to user
 */
class IUIAdapter {
public:
    virtual ~IUIAdapter() = default;
    
    /**
     * Initialize UI adapter with capability bus
     * 
     * Called once at startup. Adapter should:
     * 1. Register itself as ICommandExecutor in bus
     * 2. Register itself as IMetricsPublisher in bus
     * 3. Subscribe to MetricsCapability if needed
     * 4. Load configuration (port numbers, themes, etc.)
     * 5. Create UI-specific resources (windows, sockets, etc.)
     * 
     * @param bus CapabilityBus to register with
     * @throws std::runtime_error if initialization fails
     */
    virtual void Initialize(std::shared_ptr<graph::CapabilityBus> bus) = 0;
    
    /**
     * Check if adapter is running
     * @return true if initialized and not shutdown
     */
    virtual bool IsRunning() const = 0;
    
    /**
     * Graceful shutdown
     * 
     * Called once at exit. Adapter should:
     * 1. Unregister from CapabilityBus
     * 2. Close UI resources (windows, sockets, etc.)
     * 3. Stop any background threads
     * 4. Flush any remaining output
     * 
     * Safe to call multiple times (idempotent)
     */
    virtual void Shutdown() = 0;
    
    /**
     * Get adapter name for logging
     * @return String like "TerminalUIAdapter", "WebUIAdapter", "CLIAdapter"
     */
    virtual std::string GetAdapterName() const = 0;
};

}  // namespace app::interfaces
```

---

## 2. Refactored DashboardCapability

### 2.1 Current Implementation (WRONG - with UI coupling)

**Before: `include/app/capabilities/DashboardCapability.hpp`**

```cpp
// ❌ BAD: Imports UI headers
#include "ui/MetricsPanel.hpp"
#include "ui/CommandRegistry.hpp"

class DashboardCapability {
private:
    shared_ptr<MetricsPanel> metrics_panel_;          // ❌ UI object
    shared_ptr<CommandRegistry> command_registry_;     // ❌ UI object
    shared_ptr<Dashboard> dashboard_;                  // ❌ UI object
};
```

### 2.2 Refactored Implementation (CORRECT - UI-agnostic)

**After: `include/app/capabilities/DashboardCapability.hpp`**

```cpp
#pragma once

#include "core/ActiveQueue.hpp"
#include <string>
#include <optional>
#include <memory>
#include <mutex>

namespace app::interfaces {
    struct DashboardCommand;
}

namespace app::capabilities {

/**
 * @class DashboardCapability
 * Manages command and log queues between business logic and UI
 * 
 * Completely UI-agnostic:
 * - No ncurses, ftxui, or web framework dependencies
 * - Just thread-safe queues
 * - UI adapters (Terminal, Web, CLI) connect to these queues
 * 
 * Responsibilities:
 * - Dequeue commands from UI and route to business logic
 * - Enqueue logs from business logic to UI
 * 
 * Owned by: CapabilityBus
 * Accessed by: GraphExecutor (to dequeue commands), UI adapters (to enqueue logs)
 */
class DashboardCapability {
private:
    /// Command queue: UI → Business Logic
    /// Capacity: 1000 commands (sufficient for any UI)
    core::ActiveQueue<app::interfaces::DashboardCommand> command_queue_{1000};
    
    /// Log queue: Business Logic → UI
    /// Capacity: 10000 log messages (allows buffering during heavy output)
    core::ActiveQueue<std::string> log_queue_{10000};
    
    /// Protection for metrics access (if stored)
    mutable std::mutex metrics_mutex_;

public:
    DashboardCapability() = default;
    ~DashboardCapability() = default;
    
    // ========== COMMAND QUEUE INTERFACE ==========
    
    /**
     * Enqueue a command from UI
     * 
     * Non-blocking. Called by UI adapters when user executes command.
     * Safe for concurrent calls from multiple UI threads.
     * 
     * @param cmd Command to execute
     * @return true if enqueued successfully, false if queue full
     */
    bool EnqueueCommand(const app::interfaces::DashboardCommand& cmd) {
        return command_queue_.EnqueueNonBlocking(cmd);
    }
    
    /**
     * Dequeue next pending command
     * 
     * Called by GraphExecutor or business logic to get UI commands.
     * Returns immediately if queue empty (non-blocking).
     * 
     * @return Optional<DashboardCommand> if available, nullopt otherwise
     */
    std::optional<app::interfaces::DashboardCommand> DequeueCommand() {
        app::interfaces::DashboardCommand cmd;
        if (command_queue_.DequeueNonBlocking(cmd)) {
            return cmd;
        }
        return std::nullopt;
    }
    
    /**
     * Get command queue size (for metrics)
     * @return Current number of pending commands
     */
    size_t GetCommandQueueSize() const {
        return command_queue_.GetSize();
    }
    
    /**
     * Drain all pending commands
     * @return Vector of all commands in queue (queue emptied)
     */
    std::vector<app::interfaces::DashboardCommand> DrainCommands() {
        std::vector<app::interfaces::DashboardCommand> results;
        app::interfaces::DashboardCommand cmd;
        while (command_queue_.DequeueNonBlocking(cmd)) {
            results.push_back(cmd);
        }
        return results;
    }
    
    // ========== LOG QUEUE INTERFACE ==========
    
    /**
     * Enqueue a log message from business logic
     * 
     * Non-blocking. Called by any component that wants to log to UI.
     * Safe for concurrent calls.
     * 
     * @param message Log message to display
     * @return true if enqueued, false if queue full
     */
    bool AddLog(const std::string& message) {
        return log_queue_.EnqueueNonBlocking(message);
    }
    
    /**
     * Dequeue next log message
     * 
     * Called by UI adapters to fetch logs for display.
     * 
     * @return Optional<string> if available, nullopt otherwise
     */
    std::optional<std::string> DequeueLog() {
        std::string msg;
        if (log_queue_.DequeueNonBlocking(msg)) {
            return msg;
        }
        return std::nullopt;
    }
    
    /**
     * Get all pending logs
     * @return Vector of all log messages (queue emptied)
     */
    std::vector<std::string> DrainLogs() {
        std::vector<std::string> results;
        std::string msg;
        while (log_queue_.DequeueNonBlocking(msg)) {
            results.push_back(msg);
        }
        return results;
    }
    
    /**
     * Get log queue size
     * @return Current number of pending log messages
     */
    size_t GetLogQueueSize() const {
        return log_queue_.GetSize();
    }
    
    // ========== METRICS INTERFACE (OPTIONAL) ==========
    
    /**
     * Get queue statistics (for diagnostics)
     */
    struct QueueStats {
        size_t command_queue_size;
        size_t command_queue_capacity;
        size_t log_queue_size;
        size_t log_queue_capacity;
    };
    
    QueueStats GetQueueStats() const {
        return {
            command_queue_.GetSize(),
            command_queue_.GetCapacity(),
            log_queue_.GetSize(),
            log_queue_.GetCapacity(),
        };
    }
};

}  // namespace app::capabilities
```

---

## 3. Terminal UI Adapter Implementation

### 3.1 Terminal UI Adapter Header

**File: `include/ui/TerminalUIAdapter.hpp`**

```cpp
#pragma once

#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include "app/interfaces/IUIAdapter.hpp"
#include "graph/GraphMetrics.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace ui {

// Forward declarations (to avoid circular includes)
class Dashboard;

/**
 * @class TerminalUIAdapter
 * Adapts the existing Terminal UI (ncurses/ftxui) to the IUIAdapter interface
 * 
 * Responsibilities:
 * 1. Implement ICommandExecutor - Route user commands through Dashboard
 * 2. Implement IMetricsPublisher - Update Dashboard with metrics
 * 3. Implement IUIAdapter - Lifecycle management
 * 4. Own Dashboard, MetricsPanel, CommandRegistry instances
 * 
 * Thread Safety:
 * - OnMetricsEvent: Called from MetricsPolicy thread
 * - Execute: Called from various threads (UI input)
 * - FlushMetrics: Called from UI render thread
 * Uses: mutex for dashboard updates, atomic for state flags
 * 
 * Pattern: Facade pattern wrapping Dashboard+related components
 */
class TerminalUIAdapter : 
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    // UI Components (owned by adapter)
    std::shared_ptr<Dashboard> dashboard_;
    
    // State
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_initialized_{false};
    
    // Thread synchronization
    mutable std::mutex dashboard_mutex_;
    
    // Cached latest metrics
    mutable std::mutex metrics_mutex_;
    graph::MetricsEvent latest_metrics_;
    
    // CapabilityBus reference (for publishing commands)
    std::shared_ptr<graph::CapabilityBus> capability_bus_;

public:
    TerminalUIAdapter();
    virtual ~TerminalUIAdapter();
    
    // ========== IUIAdapter Implementation ==========
    
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "TerminalUIAdapter"; }
    
    // ========== ICommandExecutor Implementation ==========
    
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    // ========== IMetricsPublisher Implementation ==========
    
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
    
    // ========== UI Loop (Terminal-specific) ==========
    
    /**
     * Run the main event loop
     * Blocks until Shutdown() is called from another thread
     * 
     * @throws std::runtime_error if not initialized
     */
    void RunEventLoop();
    
    /**
     * Request exit (idempotent)
     * Called from command handler or signal handler
     */
    void RequestExit();

private:
    // Helper methods
    void ValidateInitialization() const;
    void SetupDashboard();
    void TeardownDashboard();
};

}  // namespace ui
```

### 3.2 Terminal UI Adapter Implementation

**File: `src/ui/TerminalUIAdapter.cpp` (Excerpt)**

```cpp
#include "ui/TerminalUIAdapter.hpp"
#include "ui/Dashboard.hpp"
#include "app/capabilities/DashboardCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <iostream>
#include <stdexcept>
#include <chrono>

namespace ui {

TerminalUIAdapter::TerminalUIAdapter()
    : is_running_(false), is_initialized_(false) {
}

TerminalUIAdapter::~TerminalUIAdapter() {
    if (is_running_) {
        Shutdown();
    }
}

void TerminalUIAdapter::Initialize(std::shared_ptr<graph::CapabilityBus> bus) {
    if (is_initialized_) {
        throw std::runtime_error("TerminalUIAdapter already initialized");
    }
    
    try {
        capability_bus_ = bus;
        
        // Create Dashboard (wraps ncurses/ftxui)
        SetupDashboard();
        
        // Register as ICommandExecutor
        bus->Register<app::interfaces::ICommandExecutor>(shared_from_this());
        
        // Register as IMetricsPublisher
        bus->Register<app::interfaces::IMetricsPublisher>(shared_from_this());
        
        is_initialized_ = true;
        is_running_ = true;
        
        std::cout << "TerminalUIAdapter initialized successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize TerminalUIAdapter: " << e.what() << "\n";
        throw;
    }
}

void TerminalUIAdapter::Shutdown() {
    if (!is_initialized_) {
        return;  // Idempotent
    }
    
    is_running_ = false;
    
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        if (dashboard_) {
            TeardownDashboard();
            dashboard_.reset();
        }
    }
    
    is_initialized_ = false;
}

void TerminalUIAdapter::SetupDashboard() {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    
    // Create Dashboard with ncurses
    dashboard_ = std::make_shared<Dashboard>();
    
    // Initialize UI components (MetricsPanel, CommandWindow, etc.)
    dashboard_->Initialize();
}

void TerminalUIAdapter::TeardownDashboard() {
    if (dashboard_) {
        dashboard_->Shutdown();
    }
}

// ========== ICommandExecutor Implementation ==========

app::interfaces::CommandResult TerminalUIAdapter::Execute(
    const app::interfaces::DashboardCommand& cmd) {
    
    ValidateInitialization();
    
    try {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        
        // Route through Dashboard's command registry
        return dashboard_->ExecuteCommand(cmd);
        
    } catch (const std::exception& e) {
        return app::interfaces::CommandResult(
            app::interfaces::CommandStatus::ExecutionFailed,
            std::string("Execution error: ") + e.what()
        );
    }
}

// ========== IMetricsPublisher Implementation ==========

void TerminalUIAdapter::OnMetricsEvent(const graph::MetricsEvent& event) {
    // Called from MetricsPolicy thread - MUST be fast and thread-safe
    
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        latest_metrics_ = event;
    }
    
    // Update dashboard (expensive operation deferred to FlushMetrics)
    // Don't call dashboard_->UpdateMetrics() here! (runs on metrics thread)
}

void TerminalUIAdapter::FlushMetrics() {
    // Called from UI render thread
    // Safe to do expensive operations here
    
    graph::MetricsEvent metrics_copy;
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        metrics_copy = latest_metrics_;
    }
    
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        if (dashboard_) {
            // Update metrics panels with latest data
            dashboard_->UpdateMetrics(metrics_copy);
        }
    }
}

graph::MetricsEvent TerminalUIAdapter::GetLatestMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return latest_metrics_;
}

void TerminalUIAdapter::RunEventLoop() {
    ValidateInitialization();
    
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    dashboard_->Run();  // Blocks until shutdown
}

void TerminalUIAdapter::RequestExit() {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    if (dashboard_) {
        dashboard_->RequestExit();
    }
}

void TerminalUIAdapter::ValidateInitialization() const {
    if (!is_initialized_) {
        throw std::runtime_error("TerminalUIAdapter not initialized");
    }
}

}  // namespace ui
```

---

## 4. Web UI Adapter Structure

### 4.1 Web Server Header

**File: `include/ui/web/WebUIAdapter.hpp`**

```cpp
#pragma once

#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include "app/interfaces/IUIAdapter.hpp"
#include "graph/GraphMetrics.hpp"
#include <memory>
#include <atomic>
#include <thread>
#include <deque>

namespace ui::web {

class WebServer;

/**
 * @class WebUIAdapter
 * Provides web-based UI via HTTP/WebSocket
 * 
 * Features:
 * - REST API for commands (/api/commands)
 * - WebSocket for real-time metrics streaming
 * - Static file serving (React/Vue frontend)
 * - Cross-platform (works on Linux, macOS, Windows)
 * 
 * Architecture:
 * - WebServer thread handles HTTP requests
 * - Metrics buffered and sent via WebSocket
 * - Commands queued and dequeued by business logic
 * 
 * Thread Safety:
 * - HTTP handler threads can call Execute/OnMetricsEvent concurrently
 * - Uses lock-free queue for metrics buffering
 */
class WebUIAdapter :
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    std::unique_ptr<WebServer> web_server_;
    std::atomic<bool> is_running_{false};
    
    // Metrics buffering (for WebSocket broadcasts)
    mutable std::mutex metrics_mutex_;
    std::deque<graph::MetricsEvent> metrics_buffer_;
    static constexpr size_t METRICS_BUFFER_SIZE = 1000;

public:
    explicit WebUIAdapter(uint16_t port = 8080);
    virtual ~WebUIAdapter();
    
    // ========== IUIAdapter Implementation ==========
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "WebUIAdapter"; }
    
    // ========== ICommandExecutor Implementation ==========
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    // ========== IMetricsPublisher Implementation ==========
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
};

}  // namespace ui::web
```

### 4.2 REST Endpoints

**File: `include/ui/web/RestEndpoints.hpp`**

```cpp
#pragma once

#include "app/interfaces/ICommandExecutor.hpp"
#include "graph/GraphMetrics.hpp"
#include <string>
#include <memory>

namespace ui::web {

class RestEndpoints {
private:
    std::shared_ptr<app::interfaces::ICommandExecutor> executor_;
    std::shared_ptr<graph::CapabilityBus> bus_;

public:
    RestEndpoints(
        std::shared_ptr<app::interfaces::ICommandExecutor> executor,
        std::shared_ptr<graph::CapabilityBus> bus);
    
    // ========== Endpoint Handlers ==========
    
    /**
     * GET /api/metrics
     * Returns current metrics for all nodes as JSON
     */
    std::string HandleGetMetrics() const;
    
    /**
     * GET /api/metrics/<node_id>
     * Returns metrics for specific node
     */
    std::string HandleGetNodeMetrics(const std::string& node_id) const;
    
    /**
     * POST /api/commands
     * Executes a command (JSON body: {"name": "pause", "args": [...]})
     * Returns: {"status": "success", "message": "...", "data": "..."}
     */
    std::string HandlePostCommand(const std::string& json_body);
    
    /**
     * GET /api/commands/help
     * Lists all available commands
     */
    std::string HandleGetCommandsHelp() const;
    
    /**
     * GET /api/graph
     * Returns graph structure as DOT format
     */
    std::string HandleGetGraph() const;
    
    /**
     * GET /api/logs
     * Returns recent logs (JSON array)
     */
    std::string HandleGetLogs(size_t limit = 100) const;
    
    /**
     * GET /api/status
     * Returns current execution status
     */
    std::string HandleGetStatus() const;
};

}  // namespace ui::web
```

---

## 5. CLI Adapter Structure

### 5.1 CLI Adapter Header

**File: `include/app/cli/CLIAdapter.hpp`**

```cpp
#pragma once

#include "app/interfaces/ICommandExecutor.hpp"
#include "app/interfaces/IMetricsPublisher.hpp"
#include "app/interfaces/IUIAdapter.hpp"
#include <memory>
#include <atomic>

namespace app::cli {

/**
 * @class CLIAdapter
 * Command-line interface for graph monitoring and control
 * 
 * Modes:
 * 1. Interactive: Run graph and accept stdin commands
 * 2. Batch: Run single command and exit
 * 3. Headless: Run graph without any output (server mode)
 * 
 * Features:
 * - Tab completion for commands
 * - Colored output (ANSI)
 * - Real-time metrics display
 * - Command history
 */
class CLIAdapter :
    public app::interfaces::IUIAdapter,
    public app::interfaces::ICommandExecutor,
    public app::interfaces::IMetricsPublisher {
    
private:
    struct Options {
        bool interactive = true;
        std::string single_command;
        bool headless = false;
        std::string output_format = "text";  // or "json"
    };
    
    Options options_;
    std::atomic<bool> is_running_{false};

public:
    // Parse command line arguments
    explicit CLIAdapter(int argc, char** argv);
    
    // ========== IUIAdapter Implementation ==========
    void Initialize(std::shared_ptr<graph::CapabilityBus> bus) override;
    bool IsRunning() const override { return is_running_.load(); }
    void Shutdown() override;
    std::string GetAdapterName() const override { return "CLIAdapter"; }
    
    // ========== ICommandExecutor Implementation ==========
    app::interfaces::CommandResult Execute(
        const app::interfaces::DashboardCommand& cmd) override;
    
    // ========== IMetricsPublisher Implementation ==========
    void OnMetricsEvent(const graph::MetricsEvent& event) override;
    void FlushMetrics() override;
    graph::MetricsEvent GetLatestMetrics() const override;
    
    // ========== CLI-specific Methods ==========
    
    /**
     * Run interactive CLI loop
     * Blocks until user exits
     */
    void RunInteractive();
    
    /**
     * Run single command and return result
     */
    app::interfaces::CommandResult RunSingleCommand();

private:
    void PrintPrompt();
    void PrintMetrics(const graph::MetricsEvent& event);
    std::string FormatOutput(const graph::MetricsEvent& event) const;
};

}  // namespace app::cli
```

---

## 6. Updated Application Entry Point

### 6.1 Main Function with UI Selection

**File: `src/gdashboard/main.cpp`**

```cpp
#include "app/GraphBuilder.hpp"
#include "ui/TerminalUIAdapter.hpp"
#include "ui/web/WebUIAdapter.hpp"
#include "app/cli/CLIAdapter.hpp"
#include "graph/CapabilityBus.hpp"
#include <iostream>
#include <memory>
#include <unistd.h>  // for getopt

int main(int argc, char** argv) {
    using namespace std;
    
    // Parse command line arguments
    const char* config_file = "config.json";
    string ui_mode = "terminal";
    uint16_t web_port = 8080;
    bool help = false;
    
    int opt;
    while ((opt = getopt(argc, argv, "c:m:p:h")) != -1) {
        switch (opt) {
            case 'c': config_file = optarg; break;
            case 'm': ui_mode = optarg; break;
            case 'p': web_port = static_cast<uint16_t>(atoi(optarg)); break;
            case 'h': help = true; break;
            default:
                cerr << "Unknown option: " << (char)opt << "\n";
                return 1;
        }
    }
    
    if (help) {
        cout << "Usage: " << argv[0] << " [options]\n";
        cout << "  -c <file>   Config file (default: config.json)\n";
        cout << "  -m <mode>   UI mode: terminal, web, cli (default: terminal)\n";
        cout << "  -p <port>   Web server port (default: 8080)\n";
        cout << "  -h          Show this help\n";
        return 0;
    }
    
    try {
        // 1. Build and initialize graph
        cout << "Building graph from " << config_file << "...\n";
        app::GraphBuilder builder;
        auto graph = builder.Build(config_file);
        
        // 2. Create capability bus
        auto capability_bus = make_shared<graph::CapabilityBus>();
        
        // 3. Register graph and factory in bus
        auto graph_capability = make_shared<app::capabilities::GraphCapability>();
        graph_capability->SetGraphManager(graph);
        capability_bus->Register(graph_capability);
        
        // 4. Create UI adapter based on mode
        shared_ptr<app::interfaces::IUIAdapter> ui_adapter;
        
        if (ui_mode == "web") {
            cout << "Starting Web UI on http://localhost:" << web_port << "/\n";
            ui_adapter = make_shared<ui::web::WebUIAdapter>(web_port);
        } else if (ui_mode == "cli") {
            cout << "Starting CLI interface...\n";
            ui_adapter = make_shared<app::cli::CLIAdapter>(argc, argv);
        } else {
            cout << "Starting Terminal UI...\n";
            ui_adapter = make_shared<ui::TerminalUIAdapter>();
        }
        
        // 5. Initialize UI adapter (registers implementations with bus)
        ui_adapter->Initialize(capability_bus);
        
        // 6. Create and start execution
        auto executor = make_shared<graph::GraphExecutor>();
        capability_bus->Register(executor);
        
        cout << "Initializing graph...\n";
        executor->Init(graph);
        
        cout << "Starting graph execution...\n";
        executor->Start();
        
        // 7. Run selected UI
        if (ui_mode == "terminal") {
            auto terminal_adapter = 
                static_pointer_cast<ui::TerminalUIAdapter>(ui_adapter);
            terminal_adapter->RunEventLoop();  // Blocks until exit
        } else if (ui_mode == "web") {
            auto web_adapter = 
                static_pointer_cast<ui::web::WebUIAdapter>(ui_adapter);
            // Web adapter runs server; this would typically block on signal handler
            pause();  // Wait for SIGINT
        } else if (ui_mode == "cli") {
            auto cli_adapter = 
                static_pointer_cast<app::cli::CLIAdapter>(ui_adapter);
            cli_adapter->RunInteractive();
        }
        
        // 8. Shutdown
        cout << "\nShutting down...\n";
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

---

## 7. Migration Checklist

### Phase 5.1: DashboardCapability Cleanup

- [ ] Remove `#include "ui/MetricsPanel.hpp"` from DashboardCapability.hpp
- [ ] Remove `#include "ui/CommandRegistry.hpp"` from DashboardCapability.hpp
- [ ] Remove MetricsPanel member variable
- [ ] Remove CommandRegistry member variable
- [ ] Replace with simple ActiveQueue members
- [ ] Update method implementations to use queues
- [ ] Verify no compilation errors in non-UI code
- [ ] Update unit tests for DashboardCapability

### Phase 5.2: Interface Definitions

- [ ] Create `include/app/interfaces/ICommandExecutor.hpp`
- [ ] Create `include/app/interfaces/IMetricsPublisher.hpp`
- [ ] Create `include/app/interfaces/IUIAdapter.hpp`
- [ ] Add to CMakeLists.txt
- [ ] Verify compilation

### Phase 5.3: Terminal UI Adapter

- [ ] Create `include/ui/TerminalUIAdapter.hpp`
- [ ] Create `src/ui/TerminalUIAdapter.cpp`
- [ ] Implement IUIAdapter interface
- [ ] Implement ICommandExecutor interface
- [ ] Implement IMetricsPublisher interface
- [ ] Integrate with existing Dashboard class
- [ ] Update main() to use TerminalUIAdapter
- [ ] Test with existing terminal UI

### Phase 5.4: Web UI Adapter

- [ ] Create web server infrastructure
- [ ] Create REST endpoints
- [ ] Create WebUIAdapter implementation
- [ ] Create React/Vue frontend
- [ ] Integrate with metrics publishing
- [ ] Serve static files
- [ ] Test HTTP endpoints
- [ ] Test WebSocket metrics streaming

### Phase 5.5: CLI Adapter

- [ ] Create CLIAdapter implementation
- [ ] Implement command line parsing
- [ ] Implement metrics printing
- [ ] Implement command execution
- [ ] Create CLI-mode main entry point
- [ ] Test all CLI commands

---

## 8. Testing Strategy

```cpp
// Unit test: ICommandExecutor interface
TEST(CommandExecutor, PauseCommand) {
    auto executor = make_shared<MockCommandExecutor>();
    
    DashboardCommand cmd{"pause", {}};
    auto result = executor->Execute(cmd);
    
    EXPECT_EQ(result.status, CommandStatus::Success);
    EXPECT_EQ(result.message, "Graph paused");
}

// Unit test: IMetricsPublisher interface
TEST(MetricsPublisher, OnMetricsEvent) {
    auto publisher = make_shared<MockMetricsPublisher>();
    
    MetricsEvent event = CreateTestEvent();
    publisher->OnMetricsEvent(event);
    
    EXPECT_EQ(publisher->GetLatestMetrics().timestamp, event.timestamp);
}

// Integration test: Terminal UI Adapter
TEST(TerminalUIAdapter, Integration) {
    auto bus = make_shared<CapabilityBus>();
    auto adapter = make_shared<TerminalUIAdapter>();
    
    adapter->Initialize(bus);
    EXPECT_TRUE(adapter->IsRunning());
    
    DashboardCommand cmd{"status", {}};
    auto result = adapter->Execute(cmd);
    EXPECT_TRUE(result.IsSuccess());
    
    adapter->Shutdown();
    EXPECT_FALSE(adapter->IsRunning());
}

// Integration test: Web UI Adapter
TEST(WebUIAdapter, HTTPEndpoint) {
    auto bus = make_shared<CapabilityBus>();
    auto adapter = make_shared<WebUIAdapter>(9999);
    
    adapter->Initialize(bus);
    
    // Make HTTP request to /api/status
    auto response = MakeHTTPRequest("GET", "http://localhost:9999/api/status");
    EXPECT_EQ(response.status_code, 200);
    
    adapter->Shutdown();
}
```

---

## 9. Summary

This design achieves complete UI separation by:

1. **Removing UI imports** from capability layer
2. **Defining clear interfaces** (ICommandExecutor, IMetricsPublisher, IUIAdapter)
3. **Creating adapters** that implement interfaces (Terminal, Web, CLI)
4. **Using dependency injection** (CapabilityBus registers implementations at runtime)
5. **Supporting multiple UIs** without code duplication

Benefits:
- ✅ Business logic completely independent of UI framework
- ✅ Easy to add new UI implementations
- ✅ Can deploy as Terminal, Web, or CLI without code changes
- ✅ Better testability (mock ICommandExecutor, IMetricsPublisher)
- ✅ Follows SOLID principles (Dependency Inversion, Interface Segregation)

