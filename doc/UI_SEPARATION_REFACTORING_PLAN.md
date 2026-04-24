# UI Separation & Refactoring Analysis Report

**Date**: April 24, 2026  
**Scope**: Decouple UI from GraphManager and business logic  
**Goal**: Support web UI, CLI, and future implementations

---

## Executive Summary

The current architecture has **strong separation** between business logic (nodes, edges, execution) and UI. However, there are **two coupling issues** that prevent easy adoption of alternative UIs:

1. **DashboardCapability** imports UI headers (`MetricsPanel.hpp`, `CommandRegistry.hpp`)
2. **Dashboard class** is monolithic—difficult to adapt for web/CLI without code duplication

**Recommended Approach:**
- **Clean Separation**: Remove UI imports from capabilities layer
- **UI-Agnostic Bus**: Let Dashboard directly access MetricsCapability
- **Abstract Input/Output**: Define clear interfaces for command execution and metrics delivery
- **Framework-Agnostic Core**: GraphExecutor, policies, and metrics remain independent of UI framework

---

## Current Architecture Assessment

### ✅ What Works Well

```
┌─────────────────────────────────┐
│     Business Logic Layer        │  ← No UI dependencies ✓
│ (Nodes, Edges, GraphManager)    │  ← No ftxui imports ✓
│ (GraphExecutor, Policies)       │  ← Clean abstractions ✓
└─────────────────────────────────┘
              ▲
              │ defined by interfaces
              │ (IExecutionPolicy, IMetricsCallback)
              │
┌─────────────────────────────────┐
│  Capability Bus Layer           │  ← Service locator ✓
│ (CapabilityBus, MetricsCapab.)  │  ← Type-safe registry ✓
│ (GraphCapab., DataInjectionCap.)│  ← Pub/sub pattern ✓
└─────────────────────────────────┘
              ▲
              │ BUT: DashboardCapability imports UI ✗
              │
┌─────────────────────────────────┐
│    Terminal UI Layer            │  ← ncurses/ftxui
│  (Dashboard, MetricsPanel)      │
│  (CommandRegistry, BuiltinCommands)
└─────────────────────────────────┘
```

### ❌ Current Coupling Problem

**DashboardCapability.hpp:**
```cpp
#include "ui/MetricsPanel.hpp"      // ❌ WRONG DIRECTION
#include "ui/CommandRegistry.hpp"   // ❌ Business layer shouldn't know about UI

class DashboardCapability {
    shared_ptr<MetricsPanel> metrics_panel_;      // UI object in capability
    shared_ptr<CommandRegistry> command_registry_;  // UI object in capability
    
    void SetDashboard(shared_ptr<Dashboard> db) { /* ... */ }  // Expects UI class
};
```

**Impact:**
- Capability layer tightly coupled to ncurses/ftxui
- Cannot use MetricsCapability without UI framework
- Forces all UI implementations to follow Terminal UI pattern
- Violates dependency inversion principle

---

## Proposed Refactored Architecture

### Layer 1: Business Logic (No Changes Needed)

```
┌────────────────────────────────────────────────────┐
│            BUSINESS LOGIC LAYER                    │
│                                                    │
│  • GraphManager - Graph lifecycle                  │
│  • Nodes, Edges - Dataflow primitives             │
│  • GraphExecutor - Execution orchestration        │
│  • ExecutionPolicyChain - Customization           │
│                                                    │
│  Dependencies: core/ only                          │
│  UI Dependencies: NONE ✓                           │
│  Framework: Framework-agnostic ✓                   │
└────────────────────────────────────────────────────┘
```

### Layer 2: Capability Bus (Minor Refactoring)

**Remove UI imports from DashboardCapability:**

```cpp
// BEFORE (BAD):
#include "ui/MetricsPanel.hpp"
class DashboardCapability {
    shared_ptr<MetricsPanel> metrics_panel_;
    shared_ptr<CommandRegistry> command_registry_;
};

// AFTER (GOOD):
class DashboardCapability {
    // Just queues - no UI knowledge
    core::ActiveQueue<DashboardCommand> command_queue_;
    core::ActiveQueue<string> log_queue_;
};
```

**Create new abstraction layer:**

```cpp
// NEW: app/interfaces/ICommandExecutor.hpp
class ICommandExecutor {
public:
    virtual ~ICommandExecutor() = default;
    virtual CommandResult Execute(const DashboardCommand& cmd) = 0;
};

// NEW: app/interfaces/IMetricsPublisher.hpp
class IMetricsPublisher {
public:
    virtual ~IMetricsPublisher() = default;
    virtual void PublishMetrics(const MetricsEvent& event) = 0;
};
```

**Refactored DashboardCapability:**

```cpp
class DashboardCapability {
private:
    core::ActiveQueue<DashboardCommand> command_queue_;
    core::ActiveQueue<string> log_queue_;
    
    // Owned by CapabilityBus, not DashboardCapability
    shared_ptr<ICommandExecutor> command_executor_;
    shared_ptr<IMetricsPublisher> metrics_publisher_;

public:
    // Commands
    void EnqueueCommand(const DashboardCommand& cmd) {
        command_queue_.EnqueueNonBlocking(cmd);
    }
    
    optional<DashboardCommand> DequeueCommand() {
        DashboardCommand cmd;
        if (command_queue_.DequeueNonBlocking(cmd)) {
            return cmd;
        }
        return nullopt;
    }
    
    // Logs
    void AddLog(const string& message) {
        log_queue_.EnqueueNonBlocking(message);
    }
    
    optional<string> DequeueLog() {
        string msg;
        if (log_queue_.DequeueNonBlocking(msg)) {
            return msg;
        }
        return nullopt;
    }
    
    // NO direct UI references
};
```

### Layer 3: UI Adapter Layer (NEW)

Create **independent UI adapters** that implement interfaces:

```
┌──────────────────────────────────────────────────────┐
│           UI ADAPTER LAYER (NEW)                     │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │  Terminal UI Adapter (ncurses/ftxui)            │ │
│  │                                                  │ │
│  │  implements:                                     │ │
│  │  - ICommandExecutor ✓                           │ │
│  │  - IMetricsPublisher ✓                          │ │
│  │                                                  │ │
│  │  includes:                                       │ │
│  │  - Dashboard                                     │ │
│  │  - MetricsPanel                                  │ │
│  │  - CommandRegistry                               │ │
│  │  - BuiltinCommands                               │ │
│  │                                                  │ │
│  │  responsible for:                                │ │
│  │  - Rendering metrics                             │ │
│  │  - Handling user input                           │ │
│  │  - Executing commands                            │ │
│  └─────────────────────────────────────────────────┘ │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │  Web UI Adapter (React/Vue/Svelte)              │ │
│  │                                                  │ │
│  │  implements:                                     │ │
│  │  - ICommandExecutor ✓                           │ │
│  │  - IMetricsPublisher ✓                          │ │
│  │                                                  │ │
│  │  includes:                                       │ │
│  │  - WebServer (HTTP/WebSocket)                    │ │
│  │  - JSON serialization                            │ │
│  │  - REST endpoints                                │ │
│  │                                                  │ │
│  │  responsible for:                                │ │
│  │  - Serving web assets                            │ │
│  │  - Publishing metrics to browser                 │ │
│  │  - Processing user commands                      │ │
│  └─────────────────────────────────────────────────┘ │
│                                                      │
│  ┌─────────────────────────────────────────────────┐ │
│  │  CLI Adapter (Command Line)                      │ │
│  │                                                  │ │
│  │  implements:                                     │ │
│  │  - ICommandExecutor ✓                           │ │
│  │  - IMetricsPublisher ✓                          │ │
│  │                                                  │ │
│  │  includes:                                       │ │
│  │  - CLI parser (getopt/boost::program_options)    │ │
│  │  - Text-based output                             │ │
│  │                                                  │ │
│  │  responsible for:                                │ │
│  │  - Parsing command line args                     │ │
│  │  - Printing status/metrics to stdout             │ │
│  │  - Processing commands from shell                │ │
│  └─────────────────────────────────────────────────┘ │
│                                                      │
└──────────────────────────────────────────────────────┘
         ▲                    ▲                  ▲
         │                    │                  │
         │ implements         │ implements       │ implements
         │ interfaces         │ interfaces       │ interfaces
         │                    │                  │
    ┌────┴────────────────────┴──────────────────┴─────┐
    │                                                   │
    │  CAPABILITY BUS (UI-Agnostic)                    │
    │                                                   │
    │  CapabilityBus::Register<ICommandExecutor>(...)  │
    │  CapabilityBus::Register<IMetricsPublisher>(...) │
    │  CapabilityBus::Register<MetricsCapability>(...)  │
    │  CapabilityBus::Register<DashboardCapability>(..)│
    │                                                   │
    └────┬────────────────────────────────────────────┘
         │
         │ access through typed interfaces
         │
    ┌────┴──────────────────────────────────────────┐
    │     BUSINESS LOGIC LAYER                       │
    │  (Framework-agnostic, completely decoupled)    │
    │                                                │
    │  • GraphExecutor                               │
    │  • MetricsCapability                           │
    │  • DataInjectionCapability                     │
    │  • GraphCapability                             │
    │                                                │
    └────────────────────────────────────────────────┘
```

---

## Implementation Plan: Phase 5 (UI Refactoring)

### Phase 5.1: Remove UI Coupling from DashboardCapability

**Files to Modify:**
- `include/app/capabilities/DashboardCapability.hpp` - Remove MetricsPanel, CommandRegistry imports
- `src/app/capabilities/DashboardCapability.cpp` - Remove UI-specific implementations

**Changes:**
```cpp
// DashboardCapability becomes a simple queue manager
class DashboardCapability {
private:
    core::ActiveQueue<DashboardCommand> command_queue_{1000};
    core::ActiveQueue<string> log_queue_{10000};

public:
    // Command queue interface
    void EnqueueCommand(const DashboardCommand& cmd) { command_queue_.EnqueueNonBlocking(cmd); }
    optional<DashboardCommand> DequeueCommand() { /* ... */ }
    
    // Log queue interface  
    void AddLog(const string& msg) { log_queue_.EnqueueNonBlocking(msg); }
    optional<string> DequeueLog() { /* ... */ }
};
```

**Time Estimate**: 2-3 hours

---

### Phase 5.2: Create UI Interface Abstractions

**New Files:**
- `include/app/interfaces/ICommandExecutor.hpp`
- `include/app/interfaces/IMetricsPublisher.hpp`
- `include/app/interfaces/IUIAdapter.hpp`

**Interface Definitions:**
```cpp
namespace app::interfaces {

// Commands
struct DashboardCommand {
    string name;
    vector<string> args;
    
    // Common commands:
    // - "pause", "resume", "stop", "status", "help"
    // - "graph show", "metrics show <node>", "logs filter <pattern>"
};

struct CommandResult {
    bool success = true;
    string message;
};

class ICommandExecutor {
public:
    virtual ~ICommandExecutor() = default;
    virtual CommandResult Execute(const DashboardCommand& cmd) = 0;
};

// Metrics
class IMetricsPublisher {
public:
    virtual ~IMetricsPublisher() = default;
    
    // Called when metrics event occurs
    virtual void OnMetricsEvent(const MetricsEvent& event) = 0;
    
    // Called periodically to flush buffered metrics
    virtual void FlushMetrics() = 0;
};

// UI Adapter (optional, for coordinating startup/shutdown)
class IUIAdapter {
public:
    virtual ~IUIAdapter() = default;
    
    virtual void Initialize(shared_ptr<CapabilityBus> bus) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsRunning() const = 0;
};

}
```

**Time Estimate**: 2-3 hours

---

### Phase 5.3: Refactor Terminal UI to Implement Interfaces

**Files to Modify:**
- `include/ui/Dashboard.hpp` - Make command executor
- `include/ui/MetricsPanel.hpp` - Make metrics publisher

**New Files:**
- `include/ui/TerminalUIAdapter.hpp` - Implements IUIAdapter

**Implementation:**
```cpp
// Terminal UI Adapter
namespace ui {

class TerminalUIAdapter : public app::interfaces::IUIAdapter,
                        public app::interfaces::ICommandExecutor,
                        public app::interfaces::IMetricsPublisher {
private:
    shared_ptr<Dashboard> dashboard_;
    shared_ptr<CapabilityBus> capability_bus_;

public:
    void Initialize(shared_ptr<CapabilityBus> bus) override {
        capability_bus_ = bus;
        
        // Create Dashboard with ncurses
        dashboard_ = make_shared<Dashboard>();
        
        // Register as metrics publisher
        bus->Register<IMetricsPublisher>(shared_from_this());
        
        // Register as command executor
        bus->Register<ICommandExecutor>(shared_from_this());
    }
    
    CommandResult Execute(const DashboardCommand& cmd) override {
        // Route command via Dashboard
        return dashboard_->ExecuteCommand(cmd);
    }
    
    void OnMetricsEvent(const MetricsEvent& event) override {
        // Update Dashboard with new metrics
        dashboard_->UpdateMetrics(event);
    }
    
    void Shutdown() override {
        if (dashboard_) {
            dashboard_->Stop();
        }
    }
};

}
```

**Time Estimate**: 4-5 hours

---

### Phase 5.4: Create Web UI Adapter (React/Vue)

**New Directory Structure:**
```
src/ui/web/
├── backend/
│   ├── WebUIAdapter.hpp          # HTTP server + WebSocket
│   ├── WebUIAdapter.cpp
│   ├── RestEndpoints.hpp         # GET /metrics, POST /commands
│   └── RestEndpoints.cpp
├── frontend/
│   ├── public/index.html
│   ├── src/
│   │   ├── App.tsx               # Main component
│   │   ├── components/
│   │   │   ├── MetricsChart.tsx  # Metrics visualization
│   │   │   ├── CommandPanel.tsx   # Command input
│   │   │   └── LogViewer.tsx      # Log display
│   │   ├── services/
│   │   │   └── api.ts            # HTTP/WebSocket client
│   │   └── index.tsx
│   └── package.json
└── README.md
```

**Backend Implementation:**
```cpp
// WebUIAdapter.hpp
namespace ui::web {

class WebUIAdapter : public app::interfaces::IUIAdapter,
                    public app::interfaces::ICommandExecutor,
                    public app::interfaces::IMetricsPublisher {
private:
    shared_ptr<http::WebServer> server_;
    
    // REST endpoints
    http::Response HandleMetricsRequest(const http::Request& req);
    http::Response HandleCommandRequest(const http::Request& req);
    
    // WebSocket for live metrics
    void OnMetricsUpdate(const MetricsEvent& event);

public:
    void Initialize(shared_ptr<CapabilityBus> bus) override;
    CommandResult Execute(const DashboardCommand& cmd) override;
    void OnMetricsEvent(const MetricsEvent& event) override;
    void Shutdown() override;
};

}
```

**API Endpoints:**
```
GET  /api/metrics                  # Get all current metrics
GET  /api/metrics/<node_id>        # Get metrics for specific node
POST /api/commands                 # Execute command
GET  /api/commands/help            # List available commands
GET  /api/graph                    # Get graph structure (DOT format)
GET  /api/logs                     # Get recent logs
WS   /api/metrics/stream           # WebSocket: metrics stream
WS   /api/logs/stream              # WebSocket: log stream
```

**Time Estimate**: 15-20 hours (includes React/Vue frontend)

---

### Phase 5.5: Create CLI Adapter (Command Line)

**New Files:**
- `include/app/cli/CLIAdapter.hpp`
- `src/app/cli/CLIAdapter.cpp`
- `src/app/cli/CLICommand.hpp` - Command parsing

**Implementation:**
```cpp
namespace app::cli {

class CLIAdapter : public app::interfaces::IUIAdapter,
                  public app::interfaces::ICommandExecutor,
                  public app::interfaces::IMetricsPublisher {
private:
    // Command line parsing
    boost::program_options::options_description desc_;
    
    // Output formatting
    void PrintMetrics(const MetricsEvent& event);
    void PrintStatus();
    void PrintHelp();

public:
    CLIAdapter(int argc, char** argv);
    
    void Initialize(shared_ptr<CapabilityBus> bus) override;
    CommandResult Execute(const DashboardCommand& cmd) override;
    void OnMetricsEvent(const MetricsEvent& event) override;
    void Run(); // Main CLI loop
};

}
```

**CLI Features:**
```bash
# Run graph with auto-mode
./gdashboard --config pipeline.json --mode auto

# Run with CLI monitoring
./gdashboard --config pipeline.json --mode cli

# Show current status
./gdashboard status

# Monitor specific node
./gdashboard metrics --node acceleration_fusion

# Send command
./gdashboard command pause

# Export metrics to CSV
./gdashboard export --format csv --output metrics.csv

# Generate graph visualization
./gdashboard graph --format png --output graph.png
```

**Time Estimate**: 6-8 hours

---

## Architecture Diagram: Post-Refactoring

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         APPLICATION STARTUP                             │
│                                                                          │
│  main() {                                                                │
│      // 1. Create capability bus                                         │
│      auto bus = make_shared<CapabilityBus>();                          │
│                                                                          │
│      // 2. Initialize business logic (UI-agnostic)                       │
│      auto executor = make_shared<GraphExecutor>();                      │
│      bus->Register<GraphExecutor>(executor);                             │
│                                                                          │
│      // 3. Choose and initialize UI adapter                              │
│      if (args.mode == "terminal") {                                      │
│          auto ui_adapter = make_shared<ui::TerminalUIAdapter>();       │
│          ui_adapter->Initialize(bus);                                    │
│          RunTerminalMode();                                              │
│      }                                                                    │
│      else if (args.mode == "web") {                                      │
│          auto web_adapter = make_shared<ui::web::WebUIAdapter>();      │
│          web_adapter->Initialize(bus);                                   │
│          web_adapter->Run(args.port);                                    │
│      }                                                                    │
│      else if (args.mode == "cli") {                                      │
│          auto cli_adapter = make_shared<app::cli::CLIAdapter>(argc, argv); │
│          cli_adapter->Initialize(bus);                                   │
│          cli_adapter->Run();                                             │
│      }                                                                    │
│  }                                                                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                         EXECUTION LAYER                                  │
│  (Unchanged - No UI Framework Dependencies)                              │
│                                                                          │
│  GraphExecutor ──uses──> Nodes, Edges, Policies                         │
│                                                                          │
│  PublishMetrics() ─────> MetricsCapability                              │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
              │
              │ calls ExecuteCommand() on
              │ bus->Get<ICommandExecutor>()
              │
┌─────────────────────────────────────────────────────────────────────────┐
│                       CAPABILITY BUS LAYER                               │
│  (UI-Agnostic Service Registry)                                          │
│                                                                          │
│  • MetricsCapability           (owns metrics subscribers list)            │
│  • DashboardCapability         (command/log queues)                      │
│  • GraphCapability             (graph metadata)                          │
│  • ICommandExecutor*           (injected by chosen adapter)              │
│  • IMetricsPublisher*          (injected by chosen adapter)              │
│                                                                          │
│  * Selected at runtime based on --mode                                   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
              ▲              ▲              ▲
              │              │              │
         ┌────┴────┐    ┌────┴────┐    ┌───┴─────┐
         │          │    │         │    │         │
    ┌─────────┐ ┌──────────┐  ┌───────────┐
    │Terminal │ │   Web    │  │    CLI    │
    │ UI      │ │   UI     │  │   UI      │
    │Adapter  │ │ Adapter  │  │  Adapter  │
    └─────────┘ └──────────┘  └───────────┘
         │          │             │
    implements implements    implements
    ICommandEx  ICommandEx    ICommandEx
    IMetricsP   IMetricsP     IMetricsP
```

---

## Migration Path & Backward Compatibility

### Phase 1: DashboardCapability Cleanup (Breaking Change - Minor)
- Remove UI imports from `DashboardCapability`
- Create `ICommandExecutor` and `IMetricsPublisher` interfaces
- **Backward Compatibility**: Dashboard class untouched - no impact on existing code

### Phase 2: TerminalUIAdapter Introduction (Non-breaking)
- Wrap existing Dashboard/MetricsPanel in `TerminalUIAdapter`
- Existing Dashboard still works as-is
- New adapter acts as a facade
- Can be adopted gradually

### Phase 3: Web UI Addition (Completely Optional)
- Pure addition - no modifications to existing code
- Opt-in via `--mode web`
- Existing Terminal UI remains unchanged

### Phase 4: CLI Addition (Completely Optional)
- Pure addition
- Opt-in via `--mode cli` or CLI-only binary

**Summary**: Highly incremental, backward compatible approach

---

## Benefits of Refactored Architecture

| Aspect | Current | After Refactoring |
|--------|---------|-------------------|
| **UI Framework Dependency** | Core → ncurses/ftxui (tight) | Adapter → ncurses/ftxui (loose) |
| **# of UIs Supported** | 1 (Terminal only) | 3+: Terminal, Web, CLI |
| **Code Reuse** | Business logic only | Full graph execution, metrics, capabilities |
| **Testing** | UI + business tests mixed | Separate unit/integration tests |
| **Scalability** | ncurses limited | Web UI unlimited clients |
| **Deployment** | Single executable | Multiple: terminal, server, CLI |
| **Framework Changes** | Requires core changes | Just adapter changes |
| **Latency** | ~50ms (screen redraw) | Web: configurable, CLI: instant |
| **Remote Access** | None (local terminal only) | Web: full remote access |

---

## Implementation Timeline

| Phase | Task | Effort | Dependencies |
|-------|------|--------|--------------|
| 5.1 | Remove DashboardCapability coupling | 2-3h | None |
| 5.2 | Create interface abstractions | 2-3h | 5.1 |
| 5.3 | Refactor Terminal UI adapter | 4-5h | 5.2 |
| 5.4 | Web UI adapter + React frontend | 15-20h | 5.2 |
| 5.5 | CLI adapter | 6-8h | 5.2 |
| **Total** | | **29-39h** | **~1 week** |

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| **Breaking DashboardCapability API** | Medium | Gradual migration, adapter pattern |
| **Complexity of refactoring** | Medium | Clear interface boundaries, tests |
| **Web UI frontend complexity** | High | Use framework (React/Vue with existing ecosystem) |
| **Thread safety across adapters** | Medium | Leverage existing lock-free queues, clear ownership |

---

## Conclusion

The current architecture is **well-designed** with excellent separation of concerns. The refactoring is **minimal and non-breaking**—primarily removing incorrect imports and adding adapter layers.

**Key Insight**: The hard work (metrics, execution, threading) is already done well. This refactoring just prevents incorrect dependencies from being pulled into the core.

**Recommendation**: Proceed with Phase 5 in stages, starting with DashboardCapability cleanup (low risk, immediate benefit).

