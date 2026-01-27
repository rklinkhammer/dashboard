# Copilot Instructions for gdashboard

**Project**: Real-time metrics visualization dashboard for graph execution engines  
**Status**: Phase 4 Complete (223 tests passing, MVP ready)  
**Tech Stack**: FTXUI 6.1.9, C++23, CMake 3.23+, nlohmann/json, log4cxx

---

## Architecture Overview

### Three-Layer Design

**Layer 1: Graph Execution** (`src/graph/`, `src/plugins/`)
- `GraphExecutor`: Orchestrates node execution via execution policies
- `MockGraphExecutor`: Development implementation, reads CSV for testing
- Plugins loaded dynamically; 10+ node types (altitude fusion, CSV sensors, flight FSM, etc.)
- **Data source**: MetricsCapability exposes metrics via discovery + callback pattern

**Layer 2: Dashboard Application** (`src/gdashboard/`, `src/ui/`, `src/app/`)
- `Dashboard`: Main application class, manages 4-panel layout and event loop
- `MetricsTilePanel`: Consolidated storage and rendering of node metrics
- `CommandRegistry`: Extensible command system (5+ built-in commands)
- **Lifecycle**: Initialize → Subscribe to metrics → Run 30 FPS event loop → Shutdown

**Layer 3: UI Components** (FTXUI-based)
- `MetricsPanel` (40% height): Grid of metric tiles per node
- `LoggingWindow` (15% height): log4cxx events via custom appender
- `CommandWindow` (15% height): User input + command output
- `StatusBar` (2% height): Runtime state indicators

### Critical Data Flow

```
GraphExecutor (executing)
    ↓
MetricsCapability::OnMetricsUpdate() [callback thread]
    ↓
MetricsPanel::SetLatestValue() [thread-safe with mutex]
    ↓
UpdateAllMetrics() routes values to NodeMetricsTiles
    ↓
Dashboard::Run() [main thread, 30 FPS]
    ├─ Screen refresh → MetricsPanel::Render()
    ├─ Handle input events (keyboard, tab switching)
    └─ Update status bar
```

**Thread Model**: Metrics callbacks from executor thread → thread-safe queuing → main loop processes

---

## Essential Workflows

### Building & Testing
```bash
cd build && make                    # Full build (configured for Release)
ctest -R Phase0.*                   # Run Phase 0 tests (unit + integration)
ctest -R Phase2.*                   # Run Phase 2 tests (metrics consolidation)
ctest -R Phase3.*                   # Run Phase 3 tests (commands, enhanced UI)
ctest -R Phase4.*                   # Run Phase 4 tests (complete system)
ctest -VV --output-on-failure       # Verbose test output with errors
```

**Build Prerequisites** (macOS homebrew):
- FTXUI, nlohmann-json, log4cxx, Eigen3, GTest
- C++23 compiler (clang 17+)

### Running the Dashboard
```bash
./build/bin/gdashboard --graph-config config/mock_graph.json
./build/bin/gdashboard --graph-config config/mock_graph.json \
    --logging-height 40 --command-height 20
```

### Key Commands (In-Dashboard)
- `q`: Quit  
- `F1`: Toggle debug panel  
- Left/Right arrows: Switch between metric node tabs  
- `status`: Display dashboard state  
- `run`: Start graph execution  
- `pause`/`stop`: Pause/stop execution  

---

## Project Conventions & Patterns

### 1. **Namespace Organization**
```cpp
// graph:: for execution engine
namespace graph { class GraphExecutor; class MockGraphExecutor; }

// app::capabilities:: for metrics interface
namespace app::capabilities { class MetricsCapability; struct MetricsEvent; }

// ui:: for all UI components
namespace ui { class MetricsPanel; class CommandWindow; }
```

### 2. **Metrics System (Critical Pattern)**

**NodeMetricsSchema** defines what metrics exist for a node:
```cpp
// From capability discovery
std::vector<MetricDescriptor> descriptors = capability->GetNodeMetrics("NodeName");
// Each descriptor: name, unit, min, max, alert_levels, display_type

// Create tile from schema
NodeMetricsTile tile;
tile.name = "NodeName";
for (auto& desc : descriptors) {
    tile.metrics[desc.name] = MetricValue{desc.unit, 0.0, AlertLevel::OK};
}
```

**Update Flow**:
```cpp
MetricsPanel::SetLatestValue("NodeName::metric_name", 123.45);  // Thread-safe
MetricsPanel::UpdateAllMetrics();  // Routes to correct NodeMetricsTile
MetricsTilePanel::Render() → displays updated values
```

### 3. **Command Registration Pattern**
```cpp
// In CommandRegistry::RegisterBuiltinCommands()
registry->RegisterCommand(
    "filter_metrics",     // Command name
    "Filter metrics by name pattern",
    "Usage: filter_metrics <pattern>",
    [app](const std::vector<std::string>& args) {
        // args[0] is pattern
        app->metrics_panel_->FilterByPattern(args[0]);
        return CommandResult(true, "Filtered to: " + args[0]);
    }
);
```

### 4. **Layout Calculations**
```cpp
// Window heights must sum to 100%
struct WindowHeightConfig {
    int metrics_height_percent = 68;       // Fixed (40 was original, now flexible)
    int logging_height_percent = 15;
    int command_height_percent = 15;
    int status_height_percent = 2;
    bool Validate() { return sum == 100; }  // Always validate!
};
```

### 5. **Thread Safety in Metrics**
```cpp
// Always use mutex when updating from callback thread
class MetricsPanel {
private:
    std::mutex metrics_mutex_;
    std::map<std::string, double> latest_values_;
    
public:
    void SetLatestValue(const std::string& id, double value) {
        std::lock_guard lock(metrics_mutex_);
        latest_values_[id] = value;  // Safe from callback thread
    }
};
```

---

## Key Files & Their Roles

| File | Purpose | Key Classes |
|------|---------|------------|
| [include/ui/Dashboard.hpp](include/ui/Dashboard.hpp) | Main app controller | `Dashboard` (40 lines) |
| [include/ui/MetricsTilePanel.hpp](include/ui/MetricsTilePanel.hpp) | Metric aggregation | `MetricsTilePanel`, `NodeMetricsTile` |
| [include/ui/MetricsTileWindow.hpp](include/ui/MetricsTileWindow.hpp) | Individual metric display | `MetricsTileWindow` (value/gauge/sparkline) |
| [include/ui/CommandWindow.hpp](include/ui/CommandWindow.hpp) | User input + output | `CommandWindow`, `CommandRegistry` |
| [include/graph/GraphExecutor.hpp](include/graph/GraphExecutor.hpp) | Execution engine | `GraphExecutor`, `ExecutionPolicyChain` |
| [include/app/capabilities/MetricsCapability.hpp](include/app/capabilities/MetricsCapability.hpp) | Metrics API | `MetricsCapability`, `MetricsEvent` |
| [include/config/NodeMetricsSchema.hpp](include/config/NodeMetricsSchema.hpp) | Metric definitions | `MetricDescriptor`, `NodeMetricsSchema` |

---

## Common Task Patterns

### Adding a New Command
1. Declare handler in [include/ui/BuiltinCommands.hpp](include/ui/BuiltinCommands.hpp)
2. Register in `CommandRegistry::RegisterBuiltinCommands()` with pattern shown above
3. Implement handler with access to `Dashboard* app`
4. Test in [test/ui/test_command_registry.cpp](test/ui/test_command_registry.cpp)

### Adding a New Metric Display Type
1. Add to `enum class MetricDisplayType` in [include/config/NodeMetricsSchema.hpp](include/config/NodeMetricsSchema.hpp)
2. Implement rendering in `MetricsTileWindow::Render()` with FTXUI elements
3. Example types: `VALUE`, `GAUGE`, `SPARKLINE`, `STATE`

### Integrating with Graph Execution
1. Implement `IExecutionPolicy` interface in `src/app/policies/`
2. Add to `ExecutionPolicyChain` in graph initialization
3. Call metrics callbacks via `capability->OnMetricsUpdate(event)`
4. Tests: [test/integration/](test/integration/)

---

## Critical Files to Read First

**For understanding the big picture** (1-2 hours):
1. [ARCHITECTURE.md](ARCHITECTURE.md) - Complete 3800-line spec (sections: Quick Reference, Core Architecture, Metrics Panel, Integration)
2. [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) - Phase breakdowns, design decisions, 223 test summary
3. [include/ui/Dashboard.hpp](include/ui/Dashboard.hpp) - Main class structure (40 lines, very readable)

**For working on metrics** (30 min):
1. [include/ui/MetricsTilePanel.hpp](include/ui/MetricsTilePanel.hpp) - Storage + update API
2. [include/ui/NodeMetricsTile.hpp](include/ui/NodeMetricsTile.hpp) - Data structure
3. [test/ui/test_metrics_tile_panel.cpp](test/ui/test_metrics_tile_panel.cpp) - Patterns in 13 tests

**For working on commands/UI** (30 min):
1. [DASHBOARD_COMMANDS.md](DASHBOARD_COMMANDS.md) - Command system architecture
2. [include/ui/CommandRegistry.hpp](include/ui/CommandRegistry.hpp) - Registration API
3. [src/ui/Dashboard.cpp](src/ui/Dashboard.cpp) - Event loop (Dashboard::CatchEvent)

---

## Testing Strategy

**123 Phase 0-1 tests**: GraphExecutor, metrics discovery, tile rendering
**22 Phase 2 tests**: Metric consolidation, NodeMetricsTile, thread-safe updates
**29 Phase 3 tests**: Commands, logging, layout, tab container, enhanced display
**49 Phase 4 tests**: Full system integration, configuration, export

**Running Tests**:
```bash
# All tests
ctest                       

# By phase
ctest -R "Phase[0-4].*"

# Specific test file
ctest -R "MetricsTilePanel"

# With failure details
ctest --output-on-failure
```

**Test File Structure**:
```cpp
#include <gtest/gtest.h>
#include "ui/MetricsTilePanel.hpp"

class MetricsTilePanelTest : public ::testing::Test {
    // Shared setup
};

TEST_F(MetricsTilePanelTest, UpdateAllMetrics_RoutesValuesToCorrectNodes) {
    // Arrange, Act, Assert pattern
}
```

---

## Known Constraints & Design Decisions

1. **FTXUI Only**: Dashboard uses FTXUI components directly (no custom UI framework)
2. **No Buffering in Capability**: MetricsCapability just publishes; dashboard manages history
3. **30 FPS Rendering**: Main loop refresh rate (hard requirement for responsiveness)
4. **Mutex + Callback Pattern**: Thread-safe metrics updates from executor thread
5. **4-Panel Fixed Layout**: Metrics/Logging/Command/Status percentages must sum to 100
6. **Phase 4 = MVP**: No additional UI features beyond what's in ARCHITECTURE.md Phase 4

---

## When Stuck: Debugging Checklist

- **Metrics not updating?** Check `SetLatestValue()` is called, mutex is locked, metric_id format is "NodeName::metric"
- **Layout broken?** Validate `WindowHeightConfig` sums to 100, check FTXUI flex layout
- **Command not working?** Verify registered in `RegisterBuiltinCommands()`, correct handler signature
- **Build fails?** Ensure FTXUI 6.1.9+ installed, C++23 enabled, run `cmake -B build`
- **Tests fail?** Check phase (Phase X tests only run X-related code), read test class setup

---

## External Dependencies

- **FTXUI 6.1.9**: Terminal UI framework (flexbox layout, event handling)
- **nlohmann/json**: JSON config parsing
- **log4cxx 0.13+**: Structured logging
- **GTest**: Unit testing framework
- **Eigen3**: Matrix operations for avionics
- **CMake 3.23+**: Build system

