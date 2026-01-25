# FTXUI Dashboard Architecture Document

**Status**: ✅ Architecture Complete and Finalized (January 24, 2026)  
**Scope**: Complete specification for FTXUI-based real-time metrics dashboard integrated with MockGraphExecutor  
**Consolidated**: All architectural guidance, implementation plans, and design decisions are in this single document

---

## Document Consolidation Summary

**What Happened**:
- Initial attempt (Week 1) built dashboard without complete architecture → metrics didn't display → root cause was initialization order
- Complete architectural reset: deleted implementation, preserved only ARCHITECTURE.md
- Refined architecture with clear specifications for MockGraphExecutor integration, metrics panel configuration, and data flow
- **Now (Final)**: All architectural content consolidated into single 3798-line ARCHITECTURE.md document

**What This Document Contains** (Previously split across 6 supporting documents):

| Section | Content | Pages |
|---------|---------|-------|
| **Quick Reference** | 10-step initialization sequence, 3 core components, data structures, FTXUI components used, sizing algorithm, design decisions table | 20+ |
| **Core Architecture** | FTXUI layers, design principles, component list, real-time update pattern, state management | 8 |
| **Metrics Panel Configuration** | Display types (value/gauge/sparkline/state), sizing algorithm, implementation examples, NodeMetricsSchema integration | 25 |
| **Graph Execution Engine** | MockGraphExecutor pattern, capability extraction, executor lifecycle, dashboard control flow | 20 |
| **Dashboard Layout** | 4-section layout (40/35/18/2%), per-section implementation details | 40 |
| **Integration Specification** | Component roles, data structures, initialization sequence, discovery API, update callback pattern, thread safety, error cases | 40 |
| **Implementation Roadmap** | Phase 1 (window structure), Phase 2 (executor integration + metrics), Phase 3 (enhanced features), success criteria | 15 |
| **Design Patterns** | Observer, Composite, Strategy, Template Method, Factory patterns used | 5 |
| **Implementation Issues** | Thread safety model, state management, concurrency patterns, metrics queue architecture | 25 |

**Files Deleted** (Content Consolidated):
- ❌ ARCHITECTURE_REFINEMENT_SUMMARY.md (9.0K) - Design decisions → Quick Reference section
- ❌ IMPLEMENTATION_READY.md (5.0K) - Readiness checklist → Implementation Roadmap
- ❌ REFINEMENT_COMPLETE.md (6.8K) - Completion summary → Overview & Table of Contents
- ❌ DELIVERABLES.md (7.5K) - Document index → Table of Contents
- ❌ FRESH_START_SUMMARY.md (4.8K) - Lessons learned → Quick Reference
- ❌ QUICK_REFERENCE.md (10K) - Reference material → Quick Reference section

**Single Source of Truth**:
- ✅ ARCHITECTURE.md (137K, 3798 lines) - Complete specification, no external references needed
- Ready for implementation without context-switching between documents

---

## FTXUI Dashboard Architecture Document

**Status**: ✅ Architecture Complete and Finalized (January 24, 2026)  
**Scope**: Complete specification for FTXUI-based real-time metrics dashboard integrated with MockGraphExecutor  
**Consolidated**: All architectural guidance, implementation plans, and design decisions are in this single document

---

## Overview

This document defines the complete architectural framework for building **`gdashboard`** - the primary FTXUI-based real-time metrics visualization application for graph execution engines. 

### The `gdashboard` Application

**`gdashboard`** is a first-class CMake executable target that:
- Serves as the main entry point for the dashboard system
- Creates GraphExecutor (MockGraphExecutor for development)
- Creates and manages a `DashboardApplication` instance
- Orchestrates initialization: get metrics from capability, create panels with defaults, Init executor, Start executor, Run loop
- Runs the main 30 FPS event loop until user exits
- Integrates with GraphExecutor's MetricsCapability to display real-time metrics

**Location**: `src/gdashboard/` (new module)  
**Main Entry Point**: `src/gdashboard/dashboard_main.cpp`  
**Application Class**: `DashboardApplication` (defined in `include/ui/DashboardApplication.hpp`)

**Command-line Usage**:
```bash
./gdashboard --graph-config <config.json> [--log-config <log4cxx_config>] [--timeout <executor_timeout_ms>] 
             [--logging-height <percent>] [--command-height <percent>]

# Parameters:
#   --graph-config <file>       Path to JSON configuration file for the graph executor
#                                (e.g., config/mock_graph.json)
#   --log-config <file>         Path to log4cxx configuration file (optional)
#                                (e.g., config/logging.xml)
#   --timeout <milliseconds>    Executor timeout in milliseconds (optional)
#                                (e.g., 5000 for 5-second timeout)
#   --logging-height <percent>  Height of logging window as percentage of screen (optional, default 35%)
#                                (e.g., 40 for 40% height)
#   --command-height <percent>  Height of command window as percentage of screen (optional, default 18%)
#                                (e.g., 20 for 20% height)

# Examples:
./gdashboard --graph-config config/mock_graph.json
./gdashboard --graph-config config/mock_graph.json --log-config config/logging.xml
./gdashboard --graph-config config/mock_graph.json --logging-height 40 --command-height 20
./gdashboard --graph-config config/mock_graph.json --timeout 5000 --logging-height 45
```

**Note**: Window heights must sum to ≤100% with metrics panel (default 40%). If heights exceed 100%, error is logged and defaults are used.

The architecture has been refined and finalized with two major improvements:

1. **FTXUI-Direct Approach**: Uses FTXUI 6.1.9 components directly without custom wrapper classes
2. **Metrics-Driven Display Configuration**: Metric tiles are configured entirely from `NodeMetricsSchema` with support for 4 display types (value, gauge, sparkline, state)

---

## Critical Invariant: MockGraphExecutor vs GraphExecutor

### The Role of MockGraphExecutor

**MockGraphExecutor is a test harness**, not a permanent component of the dashboard system. Its sole purpose is to supply **pre-canned, synthetic data** to evaluate and validate the dashboard during development.

- **Purpose**: Simulate real graph execution with configurable metrics (constant values, linear trends, random fluctuations, sinusoidal patterns)
- **Lifetime**: Used during Phases 1-2 to validate dashboard functionality without requiring actual graph computation
- **Data**: Generates synthetic MetricsEvent objects that mimic the real GraphExecutor's event format
- **Rate**: Publishes metrics at ~199 Hz (5ms intervals) from a background thread
- **Files**: `src/graph/MockGraphExecutor.hpp/cpp` (implements GraphExecutor API)

### The Real GraphExecutor (Future)

**GraphExecutor is the final production class** that will replace MockGraphExecutor when the dashboard is stable enough for integration.

- **Implementation**: Will be provided by the actual graph computation framework
- **API Compatibility**: Must implement the same `GraphExecutor` interface as MockGraphExecutor
- **Integration Point**: Provides MetricsCapability via its capability interface
- **Transition**: Drop-in replacement - only the executor instantiation line changes in `dashboard_main.cpp`

### Critical Invariant: Dashboard Code Is Production Implementation

**When working with MockGraphExecutor, ALL dashboard and application code should be considered as the actual implementation** - not test code.

This means:
- ✅ All dashboard components, layout, rendering logic, and event handling are **production quality**
- ✅ All application code (DashboardApplication, gdashboard entry point) follows production standards
- ✅ The integration with MetricsCapability is **final and production-grade**
- ✅ Data structures (MetricsEvent, MetricsSnapshot, MetricDescriptor) are final
- ✅ Thread safety, concurrency patterns, and state management are production-ready
- ✅ Only MockGraphExecutor is temporary - it's a **test fixture**, not the final executor

**Consequence**: When the real GraphExecutor becomes available, integration is seamless:
1. Real GraphExecutor implements the same GraphExecutor interface as MockGraphExecutor
2. Real GraphExecutor provides MetricsCapability with the same API
3. DashboardApplication calls `executor_->GetCapability<MetricsCapability>()` - works with both
4. Metrics flow from real executor through same MetricsCapability interface

No changes needed to the dashboard itself, application logic, or UI rendering.

---

## GraphExecutor CapabilityBus Architecture

### The CapabilityBus Pattern

**GraphExecutor** (both MockGraphExecutor during development and the real GraphExecutor in production) provides access to its capabilities via a **CapabilityBus** - a central registry that exposes the executor's features and data sources to the dashboard and other clients.

**Location**: `include/graph/CapabilityBus.hpp`

**Purpose**: Decouple the dashboard from direct dependencies on executor internals. The dashboard discovers and integrates with executor capabilities dynamically, rather than hard-coding assumptions about what the executor provides.

### Dashboard-Relevant Capabilities

The GraphExecutor exposes the following capabilities that are critical to dashboard operation:

#### 1. **MetricsCapability**

**What it provides**:
- Complete set of `NodeMetricsSchema` definitions for all nodes in the graph (with default starting values)
- Methods for discovering available metrics: which nodes have metrics, what metric names exist, metric types, ranges, and defaults
- Callback registration for subscribing to `MetricsEvent` objects as they occur
- **Internal buffering**: Maintains thread-safe circular buffer of recent MetricsEvent objects
- **Query methods**: Expose current and historical metric values without external buffering
- Metrics are published asynchronously as the graph executes

**Usage by Dashboard**:
```cpp
// Get MetricsCapability from executor (before Init())
auto metrics_cap = executor->GetCapability<MetricsCapability>();

// Discover all available metrics with default values
const auto& descriptors = metrics_cap->GetDiscoveredMetrics();

// Create UI tiles with default values from schema
for (const auto& desc : descriptors) {
    auto tile = std::make_shared<MetricsTileWindow>(desc);
    metrics_panel_->AddTile(tile);
}

// Subscribe to metric events for live updates (after Start())
metrics_cap->RegisterMetricsCallback(
    [this](const MetricsEvent& event) {
        this->OnMetricsEvent(event);
    }
);

// Query latest value for any metric (each frame)
if (auto snapshot = metrics_cap->GetLatestMetricValue("NodeName", "metric_name")) {
    tile->UpdateValue(snapshot->current_value);
}
```

**Integration Points**:
- `GetDiscoveredMetrics()` called BEFORE `executor.Init()` to create UI tiles with default values
- `RegisterMetricsCallback()` called to receive live streaming updates
- `GetLatestMetricValue()` called each frame (30 FPS) to update tile display

#### 2. **DataInjectionCapability**

**What it provides**:
- List of all nodes in the graph that can accept injected data
- APIs for discovering which nodes support data injection
- Methods for injecting data into the graph at runtime

**Usage by Dashboard** (Future enhancement - Phase 3):
```cpp
// Get DataInjectionCapability from executor
auto inject_cap = executor->GetCapability<DataInjectionCapability>();

// Discover injectable nodes
const auto& injectable_nodes = inject_cap->GetInjectableNodes();  // Returns list of node names

// (Future) Dashboard CommandWindow could allow user to inject data into graph nodes
```

**Integration Point**: Reserved for future Phase 3 CommandWindow implementation - allows the dashboard to send data into the graph.

### Design Benefit: MockGraphExecutor ↔ GraphExecutor Compatibility

Both MockGraphExecutor and the real GraphExecutor implement the **same CapabilityBus interface**:
- MockGraphExecutor provides MetricsCapability and DataInjectionCapability (synthetic/test data)
- Real GraphExecutor provides MetricsCapability and DataInjectionCapability (real graph data)
- **Dashboard code never knows which executor is running** - it just queries the CapabilityBus

This ensures:
- ✅ Dashboard code works identically with MockGraphExecutor (Phase 1-2) and real GraphExecutor (production)
- ✅ No changes to dashboard, application logic, or UI when switching executors
- ✅ Future capabilities can be added without affecting dashboard (e.g., ProfilingCapability, DebugCapability)

---

### Architecture Principles

The `gdashboard` application emphasizes:

- **Modularity**: Independent, reusable components (MetricsTileWindow, MetricsPanel, CommandWindow, LoggingWindow, StatusBar)
- **Composability**: Hierarchical FTXUI component composition (vbox, hbox, flexbox, window, etc.)
- **Thread Safety**: Clear separation between executor thread (writes metrics) and dashboard thread (reads metrics)
- **Real-time Performance**: 30 FPS pull-based updates from capability buffer, non-blocking UI
- **Scalability**: Support for 48+ metrics with automatic layout strategies (grid, vertical, horizontal, tabbed)
- **Maintainability**: Clear separation of concerns with explicit data flow

---

## Finalized Design Decisions (January 24, 2026)

**Decision 1: Metrics Panel Optimization**
- No premature optimization for metric panel sizing/layout
- Typical deployments will not have excessive metrics
- If throttling becomes necessary, will be implemented at source (executor) rather than dashboard
- Fallback to tabbed mode remains automatic when metrics exceed visible space

**Decision 2: Auto-Sizing Metric Tiles**
- Metric tiles automatically size to accommodate displayed text from `NodeMetricsSchema`
- Display types determine minimum tile height (value: 3L, gauge: 5L, sparkline: 7L, state: 3L)
- Text content expands tiles as needed (e.g., long metric names or descriptions)
- Grid layout adapts row heights based on maximum tile height in each row

**Decision 3: IMetricsSubscriber Callback Threading Model**
- `IMetricsSubscriber::OnMetricsEvent()` callbacks are invoked in **MetricsPublisher thread context** (dedicated executor thread)
- MetricsPublisher continuously reads from internal `ActiveQueue<MetricsEvent>`
- Callbacks must be fast (<1ms) to avoid blocking other subscribers and the metrics publishing pipeline
- Dashboard implements `IMetricsSubscriber` and receives callbacks on executor's publishing thread
- Dashboard uses atomic operations or mutexes to safely update tile state from callback

**Decision 4: Extensible Command System**
- CommandWindow supports pluggable, extensible command handlers
- Commands are registered dynamically via `RegisterCommand(name, description, handler)` pattern
- Dashboard core provides built-in commands (run_graph, pause, stop, help, status, etc.)
- New commands can be added without modifying CommandWindow implementation
- Commands are dispatcher through a central registry (map-based lookup)

**Decision 5: Log4cxx Integration**
- Once dashboard is active, all log4cxx output directed to stdout is captured and routed to LoggingWindow
- Custom log4cxx appender (`FTXUILog4cxxAppender`) redirects log entries to LoggingWindow display
- LoggingWindow maintains searchable, scrollable log buffer with filtering by log level
- Log entries retain timestamps, level, logger name, and message for diagnostics

**Decision 6: Executor Timeout Behavior**
- When executor timeout is triggered, graph execution stops (graceful shutdown via Stop() + Join())
- Dashboard does **NOT** exit; remains open for user interaction
- User can inspect final state, logs, and metrics from completed graph execution
- If user attempts to exit dashboard while graph is still running: show confirmation dialog
- Dialog allows user to confirm exit (which stops executor gracefully) or cancel

**Decision 7: Dashboard Layout Persistence**
- Dashboard remembers and persists layout preferences between sessions
- Layout state includes: selected tabs, window sizes, scroll positions, command history
- Persisted to config file (e.g., ~/.gdashboard/layout.json)
- On startup, restored from saved layout (or defaults if first run)
- User can reset to defaults via command or configuration option

**Decision 8: Callback Cleanup Timing**
- Callbacks remain registered for the **lifetime of the graph execution** (from Start() to Join())
- After `executor.Join()` completes (all threads cleaned up), callbacks are automatically unregistered
- Dashboard cleanup in destructor ensures proper resource release
- No manual callback deregistration required; lifecycle is managed by executor

**Decision 9: Error Reporting**
- No dedicated modal dialogs or popups for error reporting
- Errors are logged via log4cxx at ERROR/FATAL levels
- Error messages appear in LoggingWindow (same as all other log messages)
- Critical errors may also be printed to console (before dashboard window opens)
- Status bar can display "Error" state with brief indicator
- Users diagnose errors by reviewing LoggingWindow with filter set to ERROR/FATAL

**Decision 10: Callback Lifecycle**
- Callbacks are installed once during `DashboardApplication::Initialize()` (after `executor.Start()`)
- Callbacks remain **active for the entire lifetime of the graph execution**
- Callbacks are automatically cleaned up after `executor.Join()` completes
- Dashboard exit while graph running: gracefully stops executor, allowing callbacks to drain
- No memory leaks: executor's destructor ensures callback cleanup

---

## Table of Contents

**For Quick Start**:
- [Quick Reference: Essential Implementation Guidance](#quick-reference-essential-implementation-guidance) - Start here for the 10-step initialization, key components, and design decisions
- [Implementation Roadmap](#implementation-roadmap) - Phase-by-phase breakdown for building the dashboard

**For Complete Understanding**:
1. [Core Architecture](#core-architecture) - Layers, design principles, FTXUI components
2. [Metrics Panel Configuration from NodeMetricsSchema](#metrics-panel-configuration-from-nodemetricsschema) - Display types, sizing algorithm, examples
3. [Graph Execution Engine Dashboard Layout](#graph-execution-engine-dashboard-layout) - Section layout, window implementations
4. [Dashboard-Executor Integration Specification](#dashboard-executor-integration-specification) - Data flow, APIs, initialization
5. [Design Patterns](#design-patterns) - Architectural patterns used
6. [Implementation and Architectural Issues](#implementation-and-architectural-issues) - Thread safety, state management

**For Reference**:
- [Component Hierarchy](#component-hierarchy) - Window organization and lifecycle
- [Data Flow](#data-flow) - State management patterns
- [Phase 0 (Archive)](#phase-0-archive-mockgraphexecutor-for-dashboard-validation) - Historical context

---

## Core Architecture

### Architecture Layers

```
┌────────────────────────────────────────────┐
│  DashboardApplication (Main)               │
│  - Orchestrates initialization & run       │
├────────────────────────────────────────────┤
│  FTXUI Components & Layout                 │
│  - Metrics tiles (via MetricsPanel)        │
│  - Command window (input/output)           │
│  - Logging window (log display)            │
│  - Status bar (executor state)             │
├────────────────────────────────────────────┤
│  MetricsCapability (CapabilityBus)         │
│  - Buffers MetricsEvent from executor      │
│  - Exposes discovery & value APIs          │
│  - Provides default values from schema     │
├────────────────────────────────────────────┤
│  MockGraphExecutor / Real GraphExecutor    │
│  - Publishes MetricsEvent via callback     │
├────────────────────────────────────────────┤
│  FTXUI Core (DOM, Components, Screen)      │
└────────────────────────────────────────────┘
```

### Design Principles

1. **FTXUI-Native**: Use FTXUI components directly (vbox, hbox, text, gauge, etc)
2. **Reactive Rendering**: Components render based on state changes each frame
3. **Data Flow**: Executor → MetricsCapability → Dashboard (callback model via IMetricsSubscriber)
4. **Metrics-Driven UI**: Metric schemas define tile size and display type
5. **Thread-Safe**: Executor publishes metrics via callbacks, dashboard receives via OnMetricsEvent()

---

## FTXUI Component Architecture

### Using FTXUI Directly

This dashboard is built using FTXUI 6.1.9 components without custom wrapper classes. Key FTXUI elements used:

**Layout Components**:
- `ftxui::vbox()` - Vertical stack of elements
- `ftxui::hbox()` - Horizontal stack of elements
- `ftxui::flexbox()` - Flexible layout with sizing
- `ftxui::window()` - Bordered containers
- `ftxui::gauge()` - Progress/value gauges
- `ftxui::text()` - Text content
- `ftxui::border()` - Decorative borders
- `ftxui::color()` - Color styling
- `ftxui::xflex()`, `ftxui::yflex()` - Dimension control

**Interactive Components**:
- `ftxui::Input()` - Text input field
- `ftxui::Button()` - Clickable buttons  
- `ftxui::Renderer()` - Custom element rendering
- `ftxui::Container()` - Manages focus and events

**Real-time Update Pattern**:
```cpp
// Dashboard main loop - 30 FPS
void DashboardApplication::Run() {
    while (!should_exit_) {
        // Poll capability for latest metric values
        for (auto& [metric_id, tile] : metric_tiles_) {
            auto snapshot = metrics_cap_.GetLatestMetricValue(
                metric_id.node_name, 
                metric_id.metric_name);
            tile->UpdateValue(snapshot.current_value);
        }
        
        // FTXUI renders screen automatically
        screen_.Post(Event::Custom);
        std::this_thread::sleep_for(33ms);  // ~30 FPS
    }
}
```

**State Management**:
- Main dashboard holds reference to `GraphExecutor` (with MetricsCapability)
- DashboardApplication implements `IMetricsSubscriber` interface
- Local state: `std::map<MetricId, MetricsTileWindow>` for tiles
- MetricsCapability invokes OnMetricsEvent() callback when metrics are published
- Tile state updated immediately on metric event (callback model)
- FTXUI renders at 30 FPS with latest tile values
- No polling - callback-driven updates

---

---

## Metrics Panel Configuration from NodeMetricsSchema

The metrics panel is configured entirely from `NodeMetricsSchema::Field` descriptors published by graph nodes. The schema defines both metric content and display characteristics including display type and sizing.

### Tile Display Types

Each metric field in the schema can specify a `display_type` that determines how the tile is rendered and sized:

| Display Type | FTXUI Rendering | Tile Height | Use Case |
|---|---|---|---|
| `"value"` | Text only with color | 3 lines | Simple gauge/counter (throughput, count) |
| `"gauge"` | `ftxui::gauge()` + value | 5 lines | Percentage/range (0-100) metrics |
| `"sparkline"` | Sparkline graph with value | 7 lines | Time-series trend (processing time, queue depth) |
| `"state"` | Boolean status indicator | 3 lines | On/Off, Active/Inactive (node_active) |

### Example NodeMetricsSchema with Display Types

```json
{
  "node_name": "DataValidator",
  "metrics": [
    {
      "name": "validation_errors",
      "type": "int",
      "unit": "count",
      "display_type": "gauge",
      "precision": 0,
      "description": "Validation errors detected",
      "alert_rule": {
        "ok": [0.0, 10.0],
        "warning": [10.0, 50.0],
        "critical": "outside"
      }
    },
    {
      "name": "processing_time_ms",
      "type": "double",
      "unit": "ms",
      "display_type": "sparkline",
      "precision": 2,
      "description": "Average processing time",
      "alert_rule": {
        "ok": [0.0, 50.0],
        "warning": [0.0, 100.0],
        "critical": "outside"
      }
    },
    {
      "name": "throughput_hz",
      "type": "double",
      "unit": "hz",
      "display_type": "value",
      "precision": 1,
      "description": "Events processed per second",
      "alert_rule": {
        "ok": [100.0, 500.0],
        "warning": [50.0, 600.0],
        "critical": "outside"
      }
    },
    {
      "name": "node_active",
      "type": "bool",
      "unit": "bool",
      "display_type": "state",
      "description": "Node execution status",
      "alert_rule": {
        "ok": [1.0, 1.0],
        "warning": [0.0, 1.0],
        "critical": "outside"
      }
    }
  ]
}
```

### Metrics Panel Sizing Algorithm

**Panel layout**: 3-column grid with dynamic row height

```cpp
// MetricsPanel sizing calculation
class MetricsPanel {
private:
    struct TileConfig {
        MetricDescriptor descriptor;
        std::string display_type;  // "value", "gauge", "sparkline", "state"
        int min_height_lines;      // 3, 5, 7, or 3
        int tile_width;            // Available width / 3 columns
    };
    
    // Calculate total height needed based on all tiles
    int CalculatePanelHeight(const std::vector<TileConfig>& tiles) const {
        int max_height_per_row = 3;  // Start with minimum (value/state)
        
        // Adjust for sparkline tiles (they need 7 lines)
        for (const auto& tile : tiles) {
            if (tile.display_type == "sparkline") {
                max_height_per_row = std::max(max_height_per_row, 7);
            }
        }
        
        int columns = 3;
        int rows = (tiles.size() + columns - 1) / columns;
        return rows * max_height_per_row + (rows * 2);  // +2 per row for padding
    }
};
```

**Panel Layout Rules**:
1. **Grid**: 3 columns per row
2. **Tile Width**: `(available_width - padding) / 3`
3. **Row Height**: Maximum of all tile heights in that row
4. **Value tiles**: 3 lines (title + value + empty)
5. **Gauge tiles**: 5 lines (title + gauge bar + value + margins)
6. **Sparkline tiles**: 7 lines (title + sparkline graph + value + margins)
7. **State tiles**: 3 lines (title + [●Active/●Inactive])

**Example**: 12-node graph with 48 mixed metrics (12 sparkline, 24 gauge, 12 value)
- 48 metrics ÷ 3 columns = 16 rows
- Since sparkline tiles are in every row: max row height = 7 lines
- Panel height: 16 rows × 7 lines + 32 lines padding = **144 lines** (dense mode)
- Available screen height ~30 lines, so panel scrolls (shows 4-5 rows at a time)

###MetricsTileWindow Implementation

```cpp
class MetricsTileWindow {
public:
    MetricsTileWindow(
        const MetricDescriptor& descriptor,
        const nlohmann::json& field_spec);
    
    // Update with latest value from capability
    void UpdateValue(double new_value);
    
    // Render as FTXUI element
    ftxui::Element Render() const;
    
    // Get tile sizing
    int GetMinHeightLines() const;
    std::string GetDisplayType() const;
    
private:
    MetricDescriptor descriptor_;
    nlohmann::json field_spec_;
    double current_value_ = 0.0;
    std::deque<double> history_;  // For sparkline
    
    ftxui::Element RenderValue() const;
    ftxui::Element RenderGauge() const;
    ftxui::Element RenderSparkline() const;
    ftxui::Element RenderState() const;
    
    ftxui::Color GetStatusColor(double value) const;
};
```

**Rendering Examples**:

Value tile (3 lines):
```
╭─ throughput_hz ─╮
│     523.4 Hz     │
╰──────────────────╯
```

Gauge tile (5 lines):
```
╭─ validation_errors ─╮
│ [████████░░░] 80%    │
│    82 / 100 errors   │
│                      │
╰──────────────────────╯
```

Sparkline tile (7 lines):
```
╭─ processing_time_ms ─╮
│ 45│     ╱─╲           │
│    │   ╱   ╲    mean  │
│ 25│ ╱       ╲─────── │
│  5│         ╲─╱      │
│   │  ← last 60 sec   │
│   │  48.2 ms (OK)    │
╰──────────────────────╯
```

State tile (3 lines):
```
╭─ node_active ─╮
│  ● Active     │
╰────────────────╯
```

### MetricsPanel Container

```cpp
class MetricsPanel {
public:
    MetricsPanel(std::shared_ptr<MetricsCapability> metrics_cap);
    
    // Called once during Initialize()
    void AddTile(std::shared_ptr<MetricsTileWindow> tile);
    
    // Called each frame during Run()
    void UpdateAllMetrics();
    
    // Render as FTXUI element
    ftxui::Element Render() const;
    
private:
    std::vector<std::shared_ptr<MetricsTileWindow>> tiles_;
    std::map<std::string, size_t> tile_index_;  // metric_id → tile index
    std::shared_ptr<MetricsCapability> metrics_cap_;
    
    ftxui::Element RenderGrid() const;  // 3-column grid layout
};
```

---

## Component Hierarchy

### Component Organization

```
RootComponent
├── HeaderComponent
│   └── StatusBar
├── MainContentComponent
│   ├── SidebarComponent
│   │   ├── MenuComponent
│   │   └── FilterComponent
│   └── ContentPaneComponent
│       ├── TabComponent
│       ├── ViewportComponent
│       │   ├── DataGrid
│       │   └── Chart
│       └── SearchComponent
└── FooterComponent
    └── HelpText
```

### Component Lifecycle

```
┌─────────────────────────────────────┐
│   Component Creation                │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   Initialize()                      │
│   - Load configuration              │
│   - Subscribe to data sources       │
│   - Create child components         │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   Active Loop                       │
│   ├─ Receive Events (HandleEvent)   │
│   ├─ Update State (Update)          │
│   └─ Render (Render)                │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│   Shutdown()                        │
│   - Cleanup resources               │
│   - Unsubscribe from data sources   │
└─────────────────────────────────────┘
```

---

## Data Flow

### State Management Pattern

```
┌──────────────────┐
│   Data Source    │  (External API, File, etc.)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Data Provider   │  (Fetching & Caching)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  State Manager   │  (Transformation & Storage)
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│  Window/Display  │  (Rendering)
└──────────────────┘
```

### Data Provider Pattern

```cpp
class DataProvider {
public:
    virtual ~DataProvider() = default;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;
    
    template<typename T>
    void Subscribe(std::function<void(const T&)> callback) {
        callbacks_.push_back(
            [callback](const std::any& data) {
                callback(std::any_cast<T>(data));
            });
    }
    
protected:
    void NotifySubscribers(const std::any& data) {
        for (auto& callback : callbacks_) {
            callback(data);
        }
    }
    
private:
    std::vector<std::function<void(const std::any&)>> callbacks_;
};
```

### Update Flow

```cpp
class Dashboard {
public:
    void Update() {
        // 1. Update data providers
        for (auto& provider : data_providers_) {
            provider->Update();
        }
        
        // 2. Update windows with new data
        for (auto& window : windows_) {
            window->Update();
        }
        
        // 3. Resolve constraints
        constraint_resolver_.ResolveAll(windows_, screen_width_, screen_height_);
        
        // 4. Mark dirty regions for re-rendering
        InvalidateDisplay();
    }
};
```

---

## Example Implementations

### Example 1: Simple Metrics Dashboard

```cpp
class MetricsDashboard {
public:
    MetricsDashboard() {
        // Create layout
        root_ = std::make_shared<LayoutWindow>(
            LayoutWindow::Orientation::Vertical);
        
        // Create header
        auto header = std::make_shared<StaticWindow>(
            "System Monitor", "Welcome to Dashboard");
        header->constraints_.height = SizeSpec::Fixed(3);
        root_->AddChild(header);
        
        // Create metrics container
        auto metrics_layout = std::make_shared<LayoutWindow>(
            LayoutWindow::Orientation::Horizontal);
        metrics_layout->constraints_.height = SizeSpec::Flexible();
        
        // Add metric windows
        auto cpu = std::make_shared<GaugeWindow>(
            "CPU Usage",
            []() { return GetCPUUsage(); });
        cpu->constraints_.width = SizeSpec::Percentage(33);
        metrics_layout->AddChild(cpu);
        
        auto memory = std::make_shared<GaugeWindow>(
            "Memory Usage",
            []() { return GetMemoryUsage(); });
        memory->constraints_.width = SizeSpec::Percentage(33);
        metrics_layout->AddChild(memory);
        
        auto disk = std::make_shared<GaugeWindow>(
            "Disk Usage",
            []() { return GetDiskUsage(); });
        disk->constraints_.width = SizeSpec::Percentage(34);
        metrics_layout->AddChild(disk);
        
        root_->AddChild(metrics_layout);
        
        // Create footer
        auto footer = std::make_shared<StaticWindow>(
            "", "Press 'q' to quit");
        footer->constraints_.height = SizeSpec::Fixed(1);
        root_->AddChild(footer);
    }
    
    std::shared_ptr<LayoutWindow> GetRoot() { return root_; }
    
private:
    std::shared_ptr<LayoutWindow> root_;
};
```

### Example 2: Data Table Dashboard

```cpp
class LogViewerDashboard {
public:
    LogViewerDashboard() {
        root_ = std::make_shared<LayoutWindow>(
            LayoutWindow::Orientation::Vertical);
        
        // Add search bar
        auto search = std::make_shared<TextInputWindow>("Search Logs");
        search->constraints_.height = SizeSpec::Fixed(3);
        root_->AddChild(search);
        
        // Add table
        std::vector<TableWindow::ColumnSpec> columns = {
            {"Timestamp", 20, TextAlignment::Left},
            {"Level", 10, TextAlignment::Center},
            {"Message", 50, TextAlignment::Left}
        };
        
        auto table = std::make_shared<TableWindow>(
            "Logs",
            columns,
            []() { return GetLogEntries(); });
        table->constraints_.height = SizeSpec::Flexible();
        root_->AddChild(table);
    }
    
    std::shared_ptr<LayoutWindow> GetRoot() { return root_; }
    
private:
    std::shared_ptr<LayoutWindow> root_;
};
```

### Example 3: Multi-View Dashboard with Navigation

```cpp
class DashboardApplication {
public:
    void Initialize() {
        // Create main container
        main_container_ = std::make_shared<LayoutWindow>(
            LayoutWindow::Orientation::Horizontal);
        
        // Create sidebar menu
        sidebar_ = std::make_shared<MenuWindow>(
            "Views",
            {"Overview", "Metrics", "Logs", "Settings"});
        sidebar_->constraints_.width = SizeSpec::Fixed(20);
        sidebar_->SetOnSelect([this](int idx) { SwitchView(idx); });
        
        main_container_->AddChild(sidebar_);
        
        // Create content area
        content_area_ = std::make_shared<LayoutWindow>(
            LayoutWindow::Orientation::Vertical);
        content_area_->constraints_.width = SizeSpec::Flexible();
        
        // Initialize views
        InitializeViews();
        
        // Show default view
        SwitchView(0);
        
        main_container_->AddChild(content_area_);
    }
    
private:
    void InitializeViews() {
        views_.push_back(CreateOverviewView());
        views_.push_back(CreateMetricsView());
        views_.push_back(CreateLogsView());
        views_.push_back(CreateSettingsView());
    }
    
    void SwitchView(int index) {
        content_area_->children_.clear();
        content_area_->AddChild(views_[index]);
    }
    
    std::shared_ptr<Window> CreateOverviewView() {
        // Implementation
        return nullptr;
    }
    
    std::shared_ptr<Window> CreateMetricsView() {
        // Implementation
        return nullptr;
    }
    
    std::shared_ptr<Window> CreateLogsView() {
        // Implementation
        return nullptr;
    }
    
    std::shared_ptr<Window> CreateSettingsView() {
        // Implementation
        return nullptr;
    }
    
    std::shared_ptr<LayoutWindow> main_container_;
    std::shared_ptr<MenuWindow> sidebar_;
    std::shared_ptr<LayoutWindow> content_area_;
    std::vector<std::shared_ptr<Window>> views_;
};
```

---

## Graph Execution Engine (Controlled Entity)

The dashboard is a frontend that controls a graph execution engine instance. The engine is responsible for building, initializing, and executing a computation graph based on configuration parameters and data inputs. The dashboard provides real-time monitoring, control, and diagnostics for this execution engine.

### Engine Initialization Flow

```
┌─────────────────────────────────────────────────────────────────┐
│  1. Command Line Parameters (Main Entry Point)                  │
│     - Graph configuration file (JSON)                           │
│     - CSV data files (sensor inputs, test data)                 │
│     - Timing parameters (sample rates, execution speed)         │
│     - Output configuration (file paths, formats)                │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│  2. GraphExecutorBuilder Pattern                                │
│     - Parse configuration parameters                            │
│     - Validate graph topology                                   │
│     - Instantiate graph nodes                                   │
│     - Connect node ports (edges)                                │
│     - Load CSV data providers                                   │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│  3. Capability Extraction                                       │
│     - Query node metrics schemas (NodeMetricsSchema)            │
│     - Identify data injection points                            │
│     - Register completion signals                               │
│     - Extract node metadata and constraints                     │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────────────┐
│  4. GraphExecutor Lifecycle Management                          │
│     - Init(): Initialize all nodes, allocate resources          │
│     - Start(): Begin event loop, activate schedulers            │
│     - Run(): Execute graph until completion/error/user stop     │
│     - Stop(): Signal graceful shutdown                          │
│     - Join(): Wait for all threads to complete                  │
└─────────────────────────────────────────────────────────────────┘
```

### GraphExecutorBuilder Pattern

The builder is responsible for constructing a fully-initialized graph execution engine from command-line configuration.

```cpp
class GraphExecutorBuilder {
public:
    /// Parse command-line parameters and configuration files
    GraphExecutorBuilder& ParseCommandLine(int argc, char* argv[]);
    
    /// Load graph topology from configuration
    GraphExecutorBuilder& LoadGraphConfig(const std::string& config_file);
    
    /// Load CSV data sources for injection into graph
    GraphExecutorBuilder& LoadDataSources(const std::vector<std::string>& csv_files);
    
    /// Configure timing and execution parameters
    GraphExecutorBuilder& ConfigureTiming(const TimingConfig& config);
    
    /// Build and return ready-to-initialize executor
    std::unique_ptr<GraphExecutor> Build();
    
private:
    nlohmann::json graph_config_;
    std::vector<std::string> data_sources_;
    TimingConfig timing_config_;
    
    void ValidateConfiguration() const;
};
```

### Capability Extraction

After building the executor, the dashboard queries its capabilities to configure dynamic UI elements and monitoring.

```cpp
class GraphCapabilities {
public:
    /// Get metrics schemas from all nodes in graph
    std::vector<app::metrics::NodeMetricsSchema> GetNodeMetricsSchemas() const;
    
    /// Get all data injection ports (ports that accept external data)
    std::vector<DataInjectionPort> GetDataInjectionPorts() const;
    
    /// Get completion signals (conditions that mark graph completion)
    std::vector<CompletionSignal> GetCompletionSignals() const;
    
    /// Get node topology information
    GraphTopology GetGraphTopology() const;
    
    /// Get node metadata (names, types, descriptions)
    std::map<std::string, NodeMetadata> GetNodeMetadata() const;
};
```

The dashboard uses these capabilities to:
- **Build metrics tiles** from node metric schemas
- **Configure data injection windows** for externally-provided data
- **Setup completion monitoring** to track graph progress
- **Visualize graph structure** in debugging views
- **Register metric callbacks** from nodes for real-time updates

### GraphExecutor Lifecycle

The dashboard controls the executor through a well-defined state machine:

```cpp
enum class ExecutorState {
    Uninitialized,  // Created but not initialized
    Initialized,    // Init() completed, ready to start
    Running,        // Start() called, graph executing
    Paused,         // Execution paused (optional)
    Stopping,       // Stop() called, graceful shutdown in progress
    Stopped,        // Stop completed, ready to join
    Joined,         // Join() completed, all threads cleaned up
    Error           // Error state, requires investigation
};

class GraphExecutor {
public:
    /// Initialize all nodes and prepare for execution
    /// - Allocates buffers and message queues
    /// - Calls Node::Init() for each node
    /// - Starts monitoring threads
    void Init();
    
    /// Start the graph execution
    /// - Activates message dispatch threads
    /// - Begins node scheduling
    /// - Starts data flow through the graph
    void Start();
    
    /// Run the graph to completion
    /// - Blocks until graph completes or error
    /// - Monitors completion signals
    /// - Handles data flow and node scheduling
    void Run();
    
    /// Signal graceful shutdown
    /// - Allows in-flight messages to complete
    /// - Signals nodes to finish processing
    /// - Non-blocking, call Join() to wait
    void Stop();
    
    /// Wait for all executor threads to complete
    /// - Blocks until all worker threads exit
    /// - Collects final statistics
    /// - Returns error code if execution failed
    int Join();
    
    /// Get current execution state
    ExecutorState GetState() const;
    
    /// Get execution statistics
    ExecutionStats GetStats() const;
    
    /// Get capabilities for UI configuration
    GraphCapabilities GetCapabilities() const;
    
    /// Register metrics callback for real-time updates
    void RegisterMetricsCallback(IMetricsCallback* callback);
};
```

### Dashboard Control Flow

```
┌─────────────────────────────┐
│  User Starts Dashboard      │
└────────────┬────────────────┘
             │
             ▼
┌─────────────────────────────┐
│  Load config & Build Graph  │
│  (GraphExecutorBuilder)     │
└────────────┬────────────────┘
             │
             ▼
┌─────────────────────────────┐
│  Extract Capabilities       │
│  (Build metric tiles, etc.) │
└────────────┬────────────────┘
             │
             ▼
┌─────────────────────────────┐
│  Call executor.Init()       │
│  Display ready status       │
└────────────┬────────────────┘
             │
             ▼
┌─────────────────────────────┐
│  User issues "run_graph"    │
│  Command via dashboard      │
└────────────┬────────────────┘
             │
             ▼
┌─────────────────────────────┐
│  Call executor.Start()      │
│  Update status to RUNNING   │
└────────────┬────────────────┘
             │
    ┌────────┴────────┐
    │                 │
    ▼                 ▼
┌──────────┐     ┌──────────────┐
│ Run()    │     │ Monitor Loop │
│ Blocks   │     │ Update tiles │
│ on graph │     │ from metrics │
└────┬─────┘     └──────┬───────┘
     │                  │
     │                  ▼
     │          ┌──────────────────┐
     │          │ User pauses or   │
     │          │ graph completes  │
     └──────────┤ Signal Stop()    │
                └────────┬─────────┘
                         │
                         ▼
                ┌──────────────────┐
                │  Call Join()     │
                │  Collect results │
                │  Display summary │
                └──────────────────┘
```

---

## Graph Execution Engine Dashboard Layout

This section describes a specific four-section dashboard layout optimized for real-time monitoring of graph node execution, metrics, and system state. The layout is designed as a generic frontend for any graph-based execution engine, supporting dynamic metrics visualization, execution logs, and node/graph management commands.

### Overall Layout Structure

```
┌────────────────────────────────────────────────────────────────┐
│                     METRICS TILE PANEL (40%)                   │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐           │
│  │ Tile: Metric │ │ Tile: Metric │ │ Tile: Metric │ ...       │
│  │      A       │ │      B       │ │      C       │           │
│  └──────────────┘ └──────────────┘ └──────────────┘           │
├────────────────────────────────────────────────────────────────┤
│              LOGGING WINDOW (35%)                              │
│  [2025-01-24 10:45:23.123] [INFO]  Graph execution started    │
│  [2025-01-24 10:45:24.456] [WARN]  Node ProcessorA delayed    │
│  [2025-01-24 10:45:25.789] [DEBUG] Output generated: 145.2    │
├────────────────────────────────────────────────────────────────┤
│              COMMAND WINDOW (18%)                              │
│  > run_graph                                                  │
│  > pause_execution                                            │
│  > help                                                        │
├────────────────────────────────────────────────────────────────┤
│  Status: RUNNING | Nodes: 12/15 | Queue: 5 | Errors: 0        │
└────────────────────────────────────────────────────────────────┘
```

### Metrics Panel Layout Strategies

The metrics panel supports multiple layout strategies to accommodate different numbers of metrics and screen sizes:

#### Layout Strategy Selection

| Strategy | Use Case | Behavior |
|----------|----------|----------|
| **Grid** | ≤48 metrics, all fit on screen | 3-column grid layout, scrollable if needed |
| **Vertical** | Few metrics (≤12) | Single-column stack, full width |
| **Horizontal** | Few metrics (≤6) | Single-row layout, scrollable horizontally |
| **Tabbed** | >48 metrics OR insufficient space | Grouped tabs (6-12 metrics per tab), select via navigation |

**Fallback Strategy**: If metrics cannot all be displayed in the selected layout strategy, the panel automatically falls back to **tabbed mode** with adaptive grouping:
- Each tab contains 6-12 metrics depending on available height
- Tabs are labeled by node name or metric category
- User navigates between tabs using arrow keys or navigation commands
- Current tab always shows all metrics for that tab

### Section 1: Metrics Tile Panel (Top, ~40% Height)

The top section displays metric panels using one of four layout strategies (grid, vertical, horizontal, or tabbed), each rendering one or more metrics from the system. Tiles are dynamically configured from `NodeMetricsSchema` JSON descriptors. The layout strategy is automatically selected based on the number of metrics and available screen space, with automatic fallback to tabbed mode if metrics cannot fit in the preferred layout.

#### Tile Configuration Format

Each metric tile is defined by a `NodeMetricsSchema::Field` descriptor from graph nodes. The schema defines metric value, display characteristics, and alert thresholds:

```json
{
  "name": "processing_time_ms",
  "type": "double",
  "unit": "ms",
  "display_type": "sparkline",
  "precision": 2,
  "description": "Average processing time per message",
  "alert_rule": {
    "ok": [0.0, 50.0],
    "warning": [0.0, 100.0],
    "critical": "outside"
  }
}
```

#### Field Schema Properties

| Property | Type | Purpose |
|----------|------|---------|
| `name` | string | Metric identifier (e.g., "node_throughput", "queue_depth") |
| `type` | string | Data type: "double", "int", "bool", "string" |
| `unit` | string | Display unit: "percent", "count", "ms", "bytes", "hz", etc. |
| `display_type` | string | Rendering: "value", "gauge", "sparkline", "state" (determines tile height) |
| `precision` | int | Decimal places for numeric display |
| `description` | string | Human-readable metric description |
| `alert_rule` | object | Status thresholds (see below) |

**Display Type Selection** (set in NodeMetricsSchema by node developer):
- Use `"value"` for simple numerics (throughput, count) → 3-line tile
- Use `"gauge"` for 0-100 ranges (percentage, health) → 5-line tile
- Use `"sparkline"` for trends (time-series data) → 7-line tile
- Use `"state"` for boolean status (active/inactive) → 3-line tile

#### Alert Rule Structure

```json
{
  "alert_rule": {
    "ok": [min_value, max_value],
    "warning": [min_value, max_value],
    "critical": "outside"
  }
}
```

Status determination:
- **OK (Green)**: Value in `ok` range
- **Warning (Yellow)**: Value in `warning` range but outside `ok`
- **Critical (Red)**: Value outside both ranges

#### Metric Tile Window Implementation (see Metrics Panel Configuration section above)

Implementation details with FTXUI rendering for each display type are in the "Metrics Panel Configuration from NodeMetricsSchema" section above.

#### Example Metric Configurations (Complete with display_type)

```json
{
  "metrics": [
    {
      "name": "node_throughput",
      "type": "double",
      "unit": "hz",
      "display_type": "value",
      "precision": 2,
      "description": "Node processing frequency",
      "alert_rule": {
        "ok": [50.0, 200.0],
        "warning": [25.0, 250.0],
        "critical": "outside"
      }
    },
    {
      "name": "processing_time_ms",
      "type": "double",
      "unit": "ms",
      "display_type": "sparkline",
      "precision": 2,
      "description": "Average processing time per message",
      "alert_rule": {
        "ok": [0.0, 50.0],
        "warning": [0.0, 100.0],
        "critical": "outside"
      }
    },
    {
      "name": "queue_depth",
      "type": "int",
      "unit": "count",
      "display_type": "gauge",
      "description": "Current input queue size",
      "alert_rule": {
        "ok": [0.0, 100.0],
        "warning": [0.0, 500.0],
        "critical": "outside"
      }
    },
    {
      "name": "error_count",
      "type": "int",
      "unit": "count",
      "display_type": "value",
      "description": "Total processing errors",
      "alert_rule": {
        "ok": [0.0, 0.0],
        "warning": [0.0, 10.0],
        "critical": "outside"
      }
    },
    {
      "name": "node_health",
      "type": "double",
      "unit": "percent",
      "display_type": "gauge",
      "precision": 1,
      "description": "Node health status (0-100%)",
      "alert_rule": {
        "ok": [80.0, 100.0],
        "warning": [50.0, 100.0],
        "critical": "outside"
      }
    },
    {
      "name": "node_active",
      "type": "bool",
      "unit": "bool",
      "display_type": "state",
      "description": "Node execution status",
      "alert_rule": {
        "ok": [1.0, 1.0],
        "warning": [0.0, 1.0],
        "critical": "outside"
      }
    }
  ]
}
```

---

### Section 2: Logging Window (Middle, ~35% Height)

Captures and displays log4cxx messages with real-time filtering and search capabilities. **Once the dashboard is active, all log4cxx output directed to stdout is automatically captured and routed to the LoggingWindow.**

#### LoggingWindow Implementation

```cpp
class LoggingWindow {
public:
    enum class LogLevel { Debug, Info, Warn, Error, Fatal };
    
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        LogLevel level;
        std::string logger_name;
        std::string message;
    };
    
    LoggingWindow(const std::string& title = "Logs");
    
    // Log4cxx integration - captures stdout during dashboard execution
    void RegisterLog4cxxAppender();
    void AddLogEntry(const LogEntry& entry);
    
    // Filtering and search
    void SetLevelFilter(LogLevel min_level);
    void SetSearchQuery(const std::string& query);
    
    ftxui::Element Render() const;
    bool HandleEvent(ftxui::Event event);
    
    // Configuration
    void SetMaxLogLines(size_t max_lines) { max_lines_ = max_lines; }
    void SetAutoScroll(bool enabled) { auto_scroll_ = enabled; }
    
    // Check if appender is registered (useful for diagnostics)
    bool IsAppenderRegistered() const { return appender_registered_; }
    
private:
    std::deque<LogEntry> log_entries_;
    size_t max_lines_ = 1000;
    bool auto_scroll_ = true;
    size_t scroll_offset_ = 0;
    bool appender_registered_ = false;
    
    LogLevel level_filter_ = LogLevel::Debug;
    std::string search_query_;
    
    // Rendering
    ftxui::Color GetLevelColor(LogLevel level) const;
    std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) const;
    std::string FormatLogLine(const LogEntry& entry) const;
    
    // Filtering
    bool MatchesFilter(const LogEntry& entry) const;
    std::vector<size_t> GetVisibleLines() const;
};
```

**Key Log4cxx Integration Points**:
- Custom `FTXUILog4cxxAppender` redirects log4cxx output from stdout to LoggingWindow
- Appender is registered during `DashboardApplication::Initialize()` (after window creation)
- All subsequent log messages appear in LoggingWindow with timestamp, level, logger name, and message
- Circular buffer maintains recent logs (default 1000 lines); older entries are automatically discarded
- Users can filter by log level (Debug/Info/Warn/Error/Fatal) and search by text
- Auto-scroll keeps latest messages visible; user can manually scroll to view earlier entries

#### Log4cxx Appender Integration

```cpp
class FTXUILog4cxxAppender : public log4cxx::AppenderSkeleton {
public:
    FTXUILog4cxxAppender(LoggingWindow* window) : window_(window) {}
    
    void append(const log4cxx::spi::LoggingEventPtr& event,
               log4cxx::helpers::Pool& pool) override {
        LoggingWindow::LogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.level = ConvertLevel(event->getLevel());
        entry.logger_name = event->getLoggerName();
        entry.message = event->getMessage();
        
        window_->AddLogEntry(entry);
    }
    
private:
    LoggingWindow* window_;
    
    LoggingWindow::LogLevel ConvertLevel(const log4cxx::LevelPtr& level) {
        // Map log4cxx levels to LoggingWindow levels
        if (level == log4cxx::Level::getDebug()) return LoggingWindow::LogLevel::Debug;
        if (level == log4cxx::Level::getInfo()) return LoggingWindow::LogLevel::Info;
        if (level == log4cxx::Level::getWarn()) return LoggingWindow::LogLevel::Warn;
        if (level == log4cxx::Level::getError()) return LoggingWindow::LogLevel::Error;
        return LoggingWindow::LogLevel::Fatal;
    }
};
```

### Section 3: Command Window (Lower-Middle, ~18% Height)

Interactive command input and execution window for real-time system control. Commands are **extensible and can be registered dynamically** by the dashboard or other components.

#### CommandWindow Implementation (Extensible Design)

```cpp
class CommandWindow : public InteractiveWindow {
public:
    using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
    
    CommandWindow(const std::string& title = "Command");
    
    // Command registration (extensible - commands can be added anytime)
    void RegisterCommand(const std::string& name,
                        const std::string& description,
                        CommandHandler handler);
    
    // Unregister commands (for cleanup or dynamic removal)
    void UnregisterCommand(const std::string& name);
    
    // Command execution
    std::string ExecuteCommand(const std::string& input);
    
    // Rendering and events
    ftxui::Element Render() const override;
    bool HandleEvent(ftxui::Event event) override;
    
    // History and auto-completion support
    void SetMaxHistory(size_t max) { max_history_ = max; }
    std::vector<std::string> GetCommandSuggestions(const std::string& prefix) const;
    std::vector<std::pair<std::string, std::string>> GetAllCommands() const;  // Returns (name, description)
    
    // Get help text with all registered commands
    std::string GetHelpText() const;
    
private:
    struct Command {
        std::string name;
        std::string description;
        CommandHandler handler;
    };
    
    std::map<std::string, Command> commands_;
    mutable std::shared_mutex commands_lock_;  // Protect commands_ for thread-safe registration
    
    std::deque<std::string> history_;
    size_t history_index_ = 0;
    size_t max_history_ = 100;
    
    std::string current_input_;
    std::string last_output_;
    
    // Parsing and formatting
    std::vector<std::string> ParseInput(const std::string& input) const;
    std::string FormatOutput(const std::string& result) const;
};
```

**Key Design Points for Extensibility**:
- Commands are stored in a map for O(1) lookup by name
- `RegisterCommand()` is public and can be called anytime (before or during execution)
- Thread-safe command registration using `std::shared_mutex` (multiple readers, exclusive writer)
- `UnregisterCommand()` allows dynamic removal (useful for conditional features)
- `GetAllCommands()` enables help system to list all available commands

#### Standard Commands (Built-in, Extensible)

```cpp
void InitializeStandardCommands(CommandWindow& cmd) {
    // System commands
    cmd.RegisterCommand("help", "Display available commands", 
        [&cmd](const auto& args) {
            // Dynamically lists all registered commands
            return cmd.GetHelpText();
        });
    
    cmd.RegisterCommand("clear", "Clear command history",
        [](const auto& args) { return "History cleared"; });
    
    // Graph execution commands
    cmd.RegisterCommand("run_graph", "Start graph execution",
        [](const auto& args) {
            if (args.size() > 1) {
                return "Running graph: " + args[1];
            }
            return "Usage: run_graph <graph_name>";
        });
    
    cmd.RegisterCommand("pause_execution", "Pause current graph execution",
        [](const auto& args) { return "Graph execution paused"; });
    
    cmd.RegisterCommand("resume_execution", "Resume paused graph execution",
        [](const auto& args) { return "Graph execution resumed"; });
    
    cmd.RegisterCommand("stop_execution", "Stop graph execution",
        [](const auto& args) { return "Graph execution stopped"; });
    
    // Diagnostic commands
    cmd.RegisterCommand("node_status", "Display node execution status",
        [](const auto& args) { return "All nodes healthy"; });
    
    cmd.RegisterCommand("graph_stats", "Display graph execution statistics",
        [](const auto& args) { 
            return "Throughput: 150 Hz | Errors: 0 | Queue: 5"; 
        });
    
    // Configuration commands
    cmd.RegisterCommand("reload_config", "Reload graph configuration",
        [](const auto& args) { return "Configuration reloaded"; });
}
```

### Section 4: Status Bar (Bottom, ~2% Height)

Fixed footer displaying critical system status and control information.

#### StatusBar Implementation

```cpp
class StatusBar : public StaticWindow {
public:
    struct StatusFields {
        std::string execution_state;   // RUNNING, PAUSED, STOPPED, ERROR
        std::string current_graph;     // Graph name/ID
        uint32_t active_nodes;         // 0-N
        uint32_t total_nodes;          // Total in graph
        uint32_t queue_depth;          // Messages queued
        uint32_t error_count;          // Total errors
    };
    
    StatusBar();
    
    void UpdateStatus(const StatusFields& fields);
    ftxui::Element Render() const override;
    
    // Formatting
    static std::string FormatSystemState(const std::string& state);
    static ftxui::Color GetStateColor(const std::string& state);
    
private:
    StatusFields current_status_;
    
    // Status indicators
    ftxui::Element RenderExecutionState() const;
    ftxui::Element RenderGraphInfo() const;
    ftxui::Element RenderNodeStatus() const;
    ftxui::Element RenderQueueStatus() const;
    ftxui::Element RenderErrorStatus() const;
};
```

#### Status Bar Display Format

```
Status: RUNNING | Graph: MultiSensorFusion | Nodes: 12/15 | Queue: 5 | Errors: 0
```

### Complete Dashboard Composition

```cpp
class GraphExecutionDashboard : public LayoutWindow {
public:
    GraphExecutionDashboard(const nlohmann::json& config);
    
    void Initialize() override;
    void Update() override;
    ftxui::Element Render() const override;
    bool HandleEvent(ftxui::Event event) override;
    
    // Layout persistence (Decision 7)
    void SaveLayoutState() const;          // Called on exit, saves to ~/.gdashboard/layout.json
    void RestoreLayoutState();             // Called on startup, restores from saved file
    void ResetLayoutToDefaults();          // Reset to default layout
    
    // Window height configuration (adjustable via CLI or config file)
    struct WindowHeights {
        int metrics_height_percent = 40;      // Default 40%
        int logging_height_percent = 35;      // Default 35%
        int command_height_percent = 18;      // Default 18%
        int status_height_percent = 2;        // Default 2%
        // Total should equal 100%
    };
    
    void SetWindowHeights(const WindowHeights& heights);
    const WindowHeights& GetWindowHeights() const { return window_heights_; }
    bool ValidateWindowHeights() const;  // Returns true if heights sum to 100%
    
private:
    // Section components
    std::shared_ptr<MetricsTilePanel> metrics_panel_;
    std::shared_ptr<LoggingWindow> logging_window_;
    std::shared_ptr<CommandWindow> command_window_;
    std::shared_ptr<StatusBar> status_bar_;
    
    // Window height configuration (adjustable via CLI or config file)
    WindowHeights window_heights_;
    
    // Layout state persistence
    struct LayoutState {
        std::vector<std::string> selected_tabs;  // Which tabs were active
        std::map<std::string, size_t> scroll_positions;  // Scroll offset per window
        std::deque<std::string> command_history;  // Saved commands
        size_t log_level_filter;           // Filtered log level
    };
    
    LayoutState current_layout_state_;
    std::string layout_config_path_ = "~/.gdashboard/layout.json";
    
    // Layout structure
    void BuildLayout();
    void ConfigureMetrics(const nlohmann::json& metrics_config);
    void RegisterCommands();
    void ConnectDataSources();
    void ApplyWindowHeights();  // Apply configured heights to FTXUI flexbox layout
};
```

**Window Height Configuration** (Adjustable):
- Heights are specified as percentages of total screen height
- Can be set via command-line parameters: `--logging-height <percent>`, `--command-height <percent>`
- Can be set via configuration file: `dashboard.window_layout` section
- Command-line parameters override configuration file settings
- Heights must sum to 100% (including metrics 40% and status 2% defaults)
- If validation fails, error is logged and defaults are used
- Configuration is applied during `Initialize()` and used in `ApplyWindowHeights()` to set FTXUI flexbox proportions

**Layout Persistence Details** (Decision 7):
- Saved state includes: selected tabs, scroll positions, command history, filter preferences
- Stored in user config directory: `~/.gdashboard/layout.json`
- Automatically restored on next dashboard launch (or defaults if first run)
- User can reset to defaults via `reset_layout` command
- Format: JSON for human-readable debugging

#### Dashboard Configuration Example

```json
{
  "dashboard": {
    "title": "Graph Execution Engine Dashboard",
    "window_layout": {
      "metrics_height_percent": 40,
      "logging_height_percent": 35,
      "command_height_percent": 18,
      "status_height_percent": 2,
      "notes": "Heights should sum to 100%. If sum exceeds 100%, defaults are used."
    },
    "sections": {
      "metrics": {
        "height_percent": 40,
        "columns": 3,
        "auto_scroll": false,
        "fields": [
          {
            "name": "node_throughput",
            "type": "double",
            "unit": "hz",
            "precision": 2,
            "description": "Node processing frequency",
            "alert_rule": {
              "ok": [50.0, 200.0],
              "warning": [25.0, 250.0],
              "critical": "outside"
            }
          }
          // ... more metrics from node schemas
        ]
      },
      "logging": {
        "height_percent": 35,
        "max_lines": 1000,
        "auto_scroll": true,
        "level_filter": "debug"
      },
{
  "dashboard": {
    "title": "Graph Execution Engine Dashboard",
    "window_layout": {
      "metrics_height_percent": 40,
      "logging_height_percent": 35,
      "command_height_percent": 18,
      "status_height_percent": 2,
      "notes": "Heights should sum to 100%. If sum exceeds 100%, defaults are used."
    },
    "sections": {
      "metrics": {
        "height_percent": 40,
        "columns": 3,
        "auto_scroll": false,
        "fields": [
          {
            "name": "node_throughput",
            "type": "double",
            "unit": "hz",
            "precision": 2,
            "description": "Node processing frequency",
            "alert_rule": {
              "ok": [50.0, 200.0],
              "warning": [25.0, 250.0],
              "critical": "outside"
            }
          }
          // ... more metrics from node schemas
        ]
      },
      "logging": {
        "height_percent": 35,
        "max_lines": 1000,
        "auto_scroll": true,
        "level_filter": "debug"
      },
      "command": {
        "height_percent": 18,
        "max_history": 100,
        "prompt": "> "
      },
      "status": {
        "height_percent": 2
      }
    }
  }
}
```

### Integration with NodeMetricsSchema

The metrics tile panel directly consumes `NodeMetricsSchema` from system nodes:

```cpp
class MetricsDashboardBuilder {
public:
    /// Build metrics tile panel from graph node metrics schemas
    /// Aggregates metrics from multiple nodes and creates dynamic tiles
    static std::shared_ptr<MetricsTilePanel> BuildFromNodeSchemas(
        const std::vector<app::metrics::NodeMetricsSchema>& node_schemas,
        std::function<std::map<std::string, double>()> metrics_provider) {
        
        std::vector<nlohmann::json> field_specs;
        
        // Extract field definitions from all node schemas
        for (const auto& schema : node_schemas) {
            if (schema.metrics_schema.contains("fields")) {
                for (const auto& field_json : schema.metrics_schema["fields"]) {
                    nlohmann::json field_with_source = field_json;
                    field_with_source["source_node"] = schema.node_name;
                    field_specs.push_back(field_with_source);
                }
            }
        }
        
        auto panel = std::make_shared<MetricsTilePanel>(
            field_specs, 
            metrics_provider);
        
        // Apply metadata configuration from first schema (or aggregate)
        if (!node_schemas.empty() && 
            node_schemas[0].metrics_schema.contains("metadata")) {
            const auto& metadata = node_schemas[0].metrics_schema["metadata"];
            if (metadata.contains("refresh_rate_hz")) {
                int refresh_hz = metadata["refresh_rate_hz"];
                panel->SetUpdateInterval(1000 / refresh_hz);
            }
        }
        
        return panel;
    }
};
```

---

## Design Patterns

### 1. Observer Pattern (Data Updates)
Data providers notify windows of changes without coupling.

### 2. Composite Pattern (Window Hierarchy)
Complex layouts built from simple window components.

### 3. Strategy Pattern (Display Styling)
Different rendering strategies for different display modes.

### 4. Template Method Pattern (Window Lifecycle)
Common initialization/update/render flow for all windows.

### 5. Factory Pattern (Window Creation)
Centralized window instantiation for consistency.

---

## Dashboard-Executor Integration Specification

This section defines the **concrete interfaces and data flows** required for the dashboard to integrate with MockGraphExecutor.

### 1. Component Roles and Responsibilities

**MockGraphExecutor** (Test Harness - Temporary): Generates synthetic metric data
- Runs in background thread
- Publishes MetricsEvent objects via callback (same format as real GraphExecutor will use)
- Each event contains: source node name, metric name, value, timestamp
- Rate: ~199 Hz (5ms intervals)
- **Replaced later**: When real GraphExecutor available, becomes drop-in replacement (no dashboard code changes)

**MetricsCapability** (via CapabilityBus - Production - Permanent): Buffers and exposes metrics
- Receives MetricsEvent callbacks from executor in background thread
- Maintains thread-safe circular buffer of recent events
- Provides `GetDiscoveredMetrics()` for initial metrics discovery with default values
- Provides `GetLatestMetricValue(node, metric)` for polling current values
- Exposes `RegisterMetricsCallback()` for live streaming updates
- Works with both MockGraphExecutor and real GraphExecutor (identical interface)

**DashboardApplication**: Orchestrates UI
- Queries MetricsCapability to discover available metrics (before Init)
- Creates UI tiles for each discovered metric with default values
- Registers callback to receive live metric updates
- Renders tiles with current values each frame (30 FPS)
- Handles user input (commands, navigation)

**MetricsTileWindow**: Displays single metric
- Initialized with default value from MetricDescriptor
- Receives value updates via OnMetricsEvent() callbacks from DashboardApplication
- Maintains local min/max history
- Renders gauge with color coding
- Updates immediately when metrics are published (callback-driven)

### 2. Data Structures

**MetricsEvent** (from executor to harness):
```cpp
struct MetricsEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string source;              // Node name: "DataValidator"
    std::string event_type;          // "metric_update"
    std::map<std::string, std::string> data;
    // data["metric_name"] = "validation_errors"
    // data["value"] = "45.2"
};
```

**MetricDescriptor** (harness discovery):
```cpp
struct MetricDescriptor {
    std::string node_name;           // "DataValidator"
    std::string metric_name;         // "validation_errors"
    double current_value;            // Latest value from buffer
    double min_value;                // Minimum seen
    double max_value;                // Maximum seen
};
```

**MetricsSnapshot** (for tile updates):
```cpp
struct MetricsSnapshot {
    double current_value;
    double min_value;
    double max_value;
    std::chrono::system_clock::time_point timestamp;
};
```

### 2b. MetricsCapability Integration Pattern

**DashboardApplication implements IMetricsSubscriber and receives metric callbacks**:

```cpp
#include "app/metrics/IMetricsSubscriber.hpp"

class DashboardApplication : public IMetricsSubscriber {
private:
    std::shared_ptr<GraphExecutor> executor_;
    std::shared_ptr<MetricsCapability> metrics_cap_;
    std::shared_ptr<MetricsPanel> metrics_panel_;
    
    void Initialize() {
        // Get MetricsCapability BEFORE Init() - we need metrics metadata
        metrics_cap_ = executor_->GetCapability<MetricsCapability>();
        
        // Discover all metrics from node schemas
        auto node_schemas = metrics_cap_->GetNodeMetricsSchemas();
        for (const auto& node_schema : node_schemas) {
            if (node_schema.metrics_schema.contains("fields")) {
                for (const auto& field : node_schema.metrics_schema["fields"]) {
                    auto tile = std::make_shared<MetricsTileWindow>(
                        node_schema.node_name,
                        field["name"]);
                    metric_tiles_[node_schema.node_name + ":" + field["name"].get<std::string>()] = tile;
                    metrics_panel_->AddTile(tile);
                }
            }
        }
        
        // Subscribe to metrics updates via callback
        metrics_cap_->RegisterMetricsCallback(
            [this](const MetricsEvent& event) {
                this->OnMetricsEvent(event);
            }
        );
    }
    
    // IMetricsSubscriber implementation
    void OnMetricsEvent(const MetricsEvent& event) override {
        // Called by MetricsPublisher thread when metric is published
        std::string key = event.source + ":" + event.data.at("metric_name");
        auto it = metric_tiles_.find(key);
        if (it != metric_tiles_.end()) {
            double value = std::stod(event.data.at("value"));
            it->second->UpdateValue(value);
        }
    }
    
    void Run() {
        // Main FTXUI loop - 30 FPS
        // Metrics are updated via OnMetricsEvent callbacks, not polling
        while (!should_exit_) {
            // FTXUI renders with current tile states
            screen_.Post(Event::Custom);
            std::this_thread::sleep_for(33ms);  // ~30 FPS
        }
    }
};
```

**Key insight**: DashboardApplication implements IMetricsSubscriber and receives metric updates via OnMetricsEvent() callbacks. MetricsCapability invokes callbacks asynchronously when metrics are published by the executor.

### 3. Initialization Sequence (Strict Ordering)

```
Step 1: Parse Command-Line Arguments
        - Read --graph-config <config.json>
        - Read [--log-config <log4cxx_config>]
        - Read [--timeout <executor_timeout_ms>]
        - Read [--logging-height <percent>]
        - Read [--command-height <percent>]
        - Validate all parameters
        - Load window height configuration from:
          * Command-line parameters (highest priority)
          * Configuration file (dashboard.window_layout section)
          * Defaults: logging 35%, command 18%
        - Apply validation: heights must sum to 100%
        - If invalid: log ERROR and use defaults

Step 2: Build GraphExecutor Using GraphExecutorBuilder (Builder Pattern)
        - Create GraphExecutorBuilder with graph config
        - Set executor timeout from CLI parameter
        - Call build() to create GraphExecutor instance
        - At this point: Executor created but NOT initialized
        - Capabilities are now AVAILABLE via CapabilityBus
        - MetricsCapability accessible for panel creation

Step 3: Create DashboardApplication with Executor Reference
        - DashboardApplication receives executor instance
        - Window height configuration passed to constructor
        - Call app.Initialize()
          * Get MetricsCapability from executor via CapabilityBus
          * Call ValidateWindowHeights() and SetWindowHeights()
          * Calls metrics_cap->GetNodeMetricsSchemas() to get all metrics schemas
          * MetricsPanel created with MetricsTileWindow for each metric
          * LoggingWindow, CommandWindow, StatusBar created
          * All panels visible on screen immediately with default content
          * Call ApplyWindowHeights() to set FTXUI flexbox proportions

Step 4: Call executor.Init()
        - Initializes all graph nodes
        - Prepares executor for execution (but does not start metrics flow)

Step 5: Call executor.Start()
        - Background thread begins
        - Metrics start publishing via MetricsCapability callback
        - MetricsPublisher thread invokes IMetricsSubscriber::OnMetricsEvent() callbacks
        - Dashboard (IMetricsSubscriber) receives callbacks directly
        - No buffering in capability - callbacks delivered immediately to subscribers

Step 6: [OPTIONAL] Brief Delay Before app.Run()
        - Optional 100ms delay allows metrics to begin flowing
        - Buffering responsibility: MetricsPanel maintains history buffer for trends
        - MetricsPanel accumulates metric values from OnMetricsEvent() callbacks
        - Sparkline display types use this historical data for trends

Step 7: Call app.Run()
        - Starts FTXUI render loop (30 FPS)
        - Each frame: reads latest metric values from MetricsPanel internal buffer
        - MetricsPanel updates history during OnMetricsEvent() callbacks (from executor thread)
        - Renders with current values and historical trends (sparklines)

Step 8: User quits (types 'q' or Ctrl+C)
        - If graph executing: show confirmation dialog
        - On confirm: proceed to shutdown

Step 9: Call executor.Stop() and executor.Join()
        - Stop executor (graceful or forced)
        - Join all executor threads
        - All callbacks flushed and cleaned up
```

**Critical design points**:
- Step 1: Command-line processing must happen FIRST before any executor creation
- Step 2: GraphExecutorBuilder (Builder Pattern) creates executor and makes capabilities available
- Step 3: Dashboard Initialize() uses capabilities to create panels with metrics discovery
- Steps 3-4 happen BEFORE executor.Start() - all UI ready with default values
- Metrics flow only begins after Step 5 (executor.Start())
- All panels created in step 3 - nothing created during execution
- Window heights validated in step 1 and applied in step 3
- Default values shown until first metrics arrive via callbacks in step 5
- Each frame renders latest tile values (updated by OnMetricsEvent() callbacks)
- Builder Pattern ensures executor is fully constructed before dashboard accesses capabilities

### 4. Discovery Interface

**MetricsCapability exposes these methods to DashboardApplication:**

```cpp
class MetricsCapability : public Capability {
public:
    /// Get node metrics schemas (for initial discovery)
    /// Called once during Initialize to get all metrics with type/units/display info
    std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const;
    
    /// Register callback for metric updates (required for dashboard)
    /// Called by MetricsPublisher thread when metrics are published by executor
    /// No buffering in capability - callbacks delivered immediately to subscribers
    void RegisterMetricsCallback(
        std::function<void(const MetricsEvent&)> callback);
};
```

### 5. Update Callback Pattern

DashboardApplication receives metric updates via IMetricsSubscriber callbacks:

```cpp
// In DashboardApplication::Initialize()
// Get NodeMetricsSchema from all nodes - completely describes metrics
auto node_schemas = metrics_cap_->GetNodeMetricsSchemas();
for (const auto& node_schema : node_schemas) {
    // node_schema.metrics_schema["fields"] contains all metrics for this node
    if (node_schema.metrics_schema.contains("fields")) {
        for (const auto& field : node_schema.metrics_schema["fields"]) {
            auto tile = std::make_shared<MetricsTileWindow>(
                node_schema.node_name,
                field["name"],
                field  // Field descriptor contains display_type, alert_rule, etc.
            );
            
            // Store for later lookup
            std::string key = node_schema.node_name + "::" + field["name"].get<std::string>();
            metric_tiles_[key] = tile;
            
            // Add to layout
            metrics_panel_->AddTile(tile);
        }
    }
}

// Subscribe to metrics via callback
metrics_cap_->RegisterMetricsCallback(
    [this](const MetricsEvent& event) {
        this->OnMetricsEvent(event);
    }
);

// In DashboardApplication::OnMetricsEvent() callback (IMetricsSubscriber implementation)
void DashboardApplication::OnMetricsEvent(const MetricsEvent& event) {
    // Called asynchronously when metrics are published by executor
    std::string key = event.source + "::" + event.data.at("metric_name");
    auto it = metric_tiles_.find(key);
    if (it != metric_tiles_.end()) {
        double value = std::stod(event.data.at("value"));
        it->second->UpdateMetric(value);  // Tile buffers value for history/sparkline
    }
}

// In DashboardApplication::Run() main loop
for (;;) {
    // FTXUI renders current state
    // Tiles are updated via OnMetricsEvent() callbacks, not polling
    // Each tile maintains its own history buffer for sparkline display
    
    // Render
    RenderScreen();
    
    // Check exit condition
    if (should_exit_) break;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(33));  // ~30 FPS
}
```

### 6. Thread Safety Requirements

**MetricsPublisher Thread** (executor's dedicated metrics publishing thread):
- Continuously reads `MetricsEvent` objects from internal `ActiveQueue<MetricsEvent>`
- Invokes all registered `IMetricsSubscriber::OnMetricsEvent()` callbacks in this thread context
- **Dashboard receives callbacks in MetricsPublisher thread, not FTXUI main thread**
- Must process callbacks quickly to avoid blocking other subscribers

**Dashboard OnMetricsEvent() Callback**:
- Called by MetricsPublisher thread when new metrics arrive
- Must be **extremely fast** (<1ms) to avoid blocking metrics publishing
- Uses atomic operations or quick mutex-protected updates to change tile state
- Does NOT perform blocking operations (I/O, locks, renderings)
- Simply updates tile's current/min/max values; FTXUI renders on next frame

**Dashboard Main Thread** (runs FTXUI loop):
- Runs independent 30 FPS render loop
- Reads tile state (set by callbacks) and renders to screen
- Does NOT call MetricsCapability methods each frame (uses cached state from callbacks)
- Calls `GetNodeMetricsSchemas()` only once during `Initialize()`
- Must complete each frame render in <33ms

**Synchronization Requirements**:
- Tile state must be protected when accessed from both callback thread and FTXUI thread
- Use `std::atomic<double>` for numeric tile values (lock-free, fast)
- Use mutex only if sharing complex objects between threads
- Callback invocation from MetricsPublisher is automatic; no manual thread management needed
- Executor cleanup (Stop() + Join()) ensures all callbacks complete before destruction

### 7. Data Flow Diagram

```
MockGraphExecutor (background thread)
    publishes MetricsEvent("DataValidator", "validation_errors", 45.2)
          ↓
MetricsCapability (in executor)
    receives event, adds to ActiveQueue<MetricsEvent>
          ↓
MetricsPublisher Thread (executor's dedicated thread)
    reads event from ActiveQueue
    invokes registered IMetricsSubscriber::OnMetricsEvent() callbacks
          ↓
DashboardApplication::OnMetricsEvent() (called in MetricsPublisher thread context)
    updates tile state atomically (atomic<double> or mutex-protected)
    very fast operation (<1ms)
          ↓
MetricsTileWindow (tile state updated)
    current_value_, min_value_, max_value_ are now current
          ↓
DashboardApplication::Run() loop (FTXUI main thread, each 33ms)
    reads tile state (set by callbacks from MetricsPublisher thread)
    FTXUI renders screen with latest tile values
```

### 8. Error Reporting (Decision 9)

**Error Reporting Strategy**:
- **No modal dialogs or popup windows** for error reporting
- All error messages are logged via log4cxx at ERROR or FATAL level
- Logged errors automatically appear in LoggingWindow with timestamps and context
- Users diagnose errors by reviewing LoggingWindow with log level filter set to ERROR/FATAL
- Critical startup errors (before dashboard window opens) printed to console stderr

**Error Message Routing**:
- **Before dashboard launches**: Errors printed to stderr (console)
- **During execution**: Errors logged via log4cxx → appear in LoggingWindow
- Status bar may display brief error indicator (e.g., "Status: ERROR")
- CommandWindow output can display status messages (e.g., "Graph execution failed: timeout exceeded")

**Example Error Scenarios**:

1. **Configuration parsing error** → stderr, application exits
2. **Executor initialization failure** → stderr, application exits
3. **Metrics discovery empty** → ERROR log (visible in LoggingWindow), continues
4. **Executor timeout** → ERROR log, graph stops, dashboard continues
5. **Callback invocation failure** → ERROR log, callback skipped, others proceed
6. **Out-of-memory in tile rendering** → ERROR log, tile displays placeholder
7. **Invalid window heights** → ERROR log, defaults applied, dashboard continues
   - Error message: "Window height validation failed (sum = 120%, expected 100%). Using defaults: Metrics 40%, Logging 35%, Command 18%, Status 2%"

**No Pop-ups, No Modals**:
- Dashboard remains clean and distraction-free
- Errors don't interrupt user workflow
- Users can review errors at their own pace via LoggingWindow
- All information preserved in logs for diagnostics

### Error Cases and Recovery

**Case 1: Metrics not discovered**
- Symptom: metric_tiles_ is empty after Initialize()
- Cause: Capability buffer was empty (executor hadn't published yet)
- Prevention: Always wait 100ms after executor.Start() before app.Run()
- Recovery: Clear all tiles, wait, call Initialize() again

**Case 2: Metric value stops updating**
- Symptom: Tile shows stale value for >1 second
- Cause: Executor stopped or metric stopped publishing
- Detection: Track timestamp of last update per tile
- Recovery: Display warning in tile, show stale data

**Case 3: Executor crashes**
- Symptom: No new metrics arriving
- Cause: Executor thread threw exception
- Detection: Check if metrics_cap_->GetLatestMetricValue() returns nullopt
- Recovery: Show error message, disable update loop, allow user to quit

### Executor Timeout Behavior (Decision 6)

**When Timeout Occurs**:
- Executor timeout specified via `--timeout <milliseconds>` command-line parameter
- If executor doesn't complete within timeout duration, graceful shutdown is triggered
- Executor calls `Stop()` followed by `Join()` to cleanly terminate execution
- **Dashboard does NOT exit** - it remains open and interactive

**Dashboard Impact**:
- Dashboard continues running, showing final state of metrics/logs
- User can inspect execution results, logs, and final metric values
- Commands remain available (help, status, etc.)
- User can launch another graph execution if desired

**User Exit While Graph Running**:
- If user attempts to exit dashboard while graph is executing:
  - **Confirmation dialog is displayed**: "Graph execution in progress. Exit dashboard? (Yes/No)"
  - User can confirm exit (which triggers executor Stop() + Join())
  - Or cancel and keep dashboard running
- This prevents accidental termination of long-running computations

**Implementation Detail**:
```cpp
// In DashboardApplication::Run()
void DashboardApplication::Run() {
    while (!should_exit_) {
        // Check if user pressed 'q' to exit
        if (user_pressed_quit && executor_->GetState() == ExecutorState::Running) {
            // Show confirmation dialog
            bool confirmed = ShowConfirmationDialog(
                "Graph execution in progress. Exit dashboard?");
            if (confirmed) {
                executor_->Stop();
                executor_->Join();
                should_exit_ = true;
            }
            // else: continue running, don't exit
        }
        
        // Render frame
        RenderFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

// Executor timeout is handled by executor itself
// Dashboard only cares about final state after timeout
```

### Callback Lifecycle (Decisions 8 & 10)

**Callback Installation**:
- Callbacks are registered **once** during `DashboardApplication::Initialize()`
- This happens after `executor.Start()` has begun metrics publishing
- All metric callbacks are installed before `app.Run()` starts the FTXUI loop

**Callback Activity**:
- Callbacks **remain active for the entire lifetime of the graph execution**
- MetricsPublisher thread invokes callbacks continuously as metrics are published
- Callbacks are **not** removed mid-execution

**Callback Cleanup**:
- After `executor.Stop()` and `executor.Join()` complete:
  - All executor threads have terminated
  - No more metrics are being published
  - No new callbacks will be invoked
  - Executor destructor automatically cleans up registered callbacks
- Dashboard cleanup happens when `app` is destroyed (destructor called)
- **No manual callback deregistration required** - it's automatic upon executor cleanup

**Memory Safety**:
- Executor holds references to callbacks until destruction
- Dashboard (DashboardApplication) lifetime must outlive executor OR
- Executor is destroyed after Dashboard (recommended)
- Callbacks use weak pointers to Dashboard to safely handle destruction

**Code Example**:
```cpp
int main() {
    auto executor = std::make_shared<MockGraphExecutor>(config);
    executor->Init();
    
    auto app = std::make_shared<DashboardApplication>(executor);
    app->Initialize();  // Registers callbacks HERE
    
    executor->Start();
    
    std::this_thread::sleep_for(100ms);
    
    app->Run();  // Callbacks invoked by MetricsPublisher thread
    
    // When user quits:
    executor->Stop();
    executor->Join();  // All callbacks flushed, executor threads cleaned up
    
    // Cleanup (order matters - callbacks cleaned up with executor)
    executor.reset();  // Executor destructor cleans up callbacks
    app.reset();       // Dashboard destructor cleans up remaining resources
}
```

### Configuration Requirements

**mock_graph.json must specify:**

```json
{
  "mock_executor": {
    "nodes": [
      {
        "name": "DataValidator",
        "metrics": [
          {
            "name": "validation_errors",
            "type": "float",
            "min_value": 0.0,
            "max_value": 100.0,
            "pattern": "random"
          }
        ]
      }
    ]
  }
}
```

**Dashboard startup command:**
```bash
./dashboard config/mock_graph.json
```

### 10. Success Criteria

**Integration is working when:**
- [ ] Dashboard starts without crashes
- [ ] All 48 metrics are discovered and tiles created
- [ ] Tiles display metric values (not zeros or defaults)
- [ ] Tile values update in real-time as executor generates new data
- [ ] Tiles maintain min/max history correctly
- [ ] Commands like 'status', 'metrics', 'help' work
- [ ] Dashboard exits cleanly on quit
- [ ] All 67 tests still pass

---

## Implementation Roadmap

### Phase 1: Basic Window Structure and Layout Framework

**Objective**: Build the **`gdashboard`** application and foundational dashboard layout with static window structure (metrics, logging, command, status sections) following the Overall Layout Structure specification.

**Starting Point - The `gdashboard` Application**:

Create the first-class executable target with this structure:

```
src/gdashboard/
├── dashboard_main.cpp          (Main entry point, follows 10-step initialization)
├── DashboardApplication.hpp    (Core orchestrator class)
└── DashboardApplication.cpp    (Implementation)

include/ui/
├── DashboardApplication.hpp    (Public interface)
├── LayoutWindow.hpp            (Basic container component)
├── MetricsPanel.hpp            (Metrics section placeholder)
├── LoggingWindow.hpp           (Logging section placeholder)
├── CommandWindow.hpp           (Command section placeholder)
└── StatusBar.hpp               (Status section placeholder)

CMakeLists.txt                  (Add 'gdashboard' executable target)
```

**Implementation Sequence for Phase 1**:

1. **Create `gdashboard` CMake Target**:
   - Add executable target: `add_executable(gdashboard src/gdashboard/dashboard_main.cpp ...)`
   - Link with FTXUI, core libraries
   - Set output directory and installation rules

2. **Implement `DashboardApplication` Class**:
   ```cpp
   class DashboardApplication {
   public:
       explicit DashboardApplication(
           std::shared_ptr<GraphExecutor> executor);
       
       // Initialization: discover metrics and create tiles
       void Initialize();
       
       // Main event loop: 30 FPS render/update loop
       void Run();
       
       // Getters for testing/debugging
       const std::shared_ptr<MetricsPanel>& GetMetricsPanel() const;
       const std::shared_ptr<LoggingWindow>& GetLoggingWindow() const;
       // ... other getters
       
   private:
       std::shared_ptr<GraphExecutor> executor_;
       std::shared_ptr<MetricsCapability> metrics_cap_;
       std::shared_ptr<MetricsPanel> metrics_panel_;
       std::shared_ptr<LoggingWindow> logging_window_;
       std::shared_ptr<CommandWindow> command_window_;
       std::shared_ptr<StatusBar> status_bar_;
       
       bool should_exit_ = false;
       
       ftxui::Element BuildLayout();
   };
   ```

3. **Implement `dashboard_main.cpp` Entry Point**:
   ```cpp
   #include <memory>
   #include "ui/DashboardApplication.hpp"
   #include "graph/GraphExecutor.hpp"
   
   int main(int argc, char* argv[]) {
       if (argc < 2) {
           std::cerr << "Usage: gdashboard <config.json>\n";
           return 1;
       }
       
       try {
           // Step 1: Create executor
           nlohmann::json config = LoadJsonConfig(argv[1]);
           auto executor = std::make_shared<MockGraphExecutor>(config);
           
           // Step 2: Init executor
           executor->Init();
           
           // Step 3: Create dashboard with executor
           auto app = std::make_shared<DashboardApplication>(executor);
           
           // Step 4: Initialize dashboard (discovers metrics)
           app->Initialize();
           
           // Step 5: Start executor
           executor->Start();
           
           // Step 6: Wait for metrics to accumulate
           std::this_thread::sleep_for(std::chrono::milliseconds(100));
           
           // Step 7: Run dashboard
           app->Run();
           
           // Step 8: Cleanup
           executor->Stop();
           executor->Join();
           
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << "\n";
           return 1;
       }
       
       return 0;
   }
   ```

4. **Create Static Window Components** (placeholders, no metrics yet):
   - **MetricsPanel**: FTXUI vbox container for metric tiles (40% height)
   - **LoggingWindow**: FTXUI bordered container with text display (35% height)
   - **CommandWindow**: FTXUI Input + output display (18% height)
   - **StatusBar**: FTXUI text showing executor state (2% height)

5. **Compose Dashboard Layout**:
   ```cpp
   ftxui::Element BuildLayout() {
       return ftxui::vbox({
           metrics_panel_->Render()  | ftxui::yflex(40),
           logging_window_->Render() | ftxui::yflex(35),
           command_window_->Render() | ftxui::yflex(18),
           status_bar_->Render()     | ftxui::yflex(2),
       });
   }
   ```

**Deliverables**:
- [ ] `src/gdashboard/` directory created with dashboard_main.cpp
- [ ] `DashboardApplication` class fully implemented
- [ ] `LayoutWindow` base container class
- [ ] Static window shells: MetricsPanel, LoggingWindow, CommandWindow, StatusBar
- [ ] CMakeLists.txt updated with `gdashboard` executable target
- [ ] `gdashboard` compiles without errors
- [ ] `gdashboard config/mock_graph.json` launches and displays 4 sections

**Success Criteria**:
- ✅ `gdashboard` executable builds successfully
- ✅ Application launches with 4 clearly separated sections (no data yet)
- ✅ Sections maintain correct height ratios (40/35/18/2)
- ✅ FTXUI rendering works without artifacts
- ✅ All 54 Phase 0.4 tests still pass
- ✅ Application exits cleanly on 'q' or Ctrl+C
- ✅ No metrics data displayed (placeholders only - Phase 2 concern)

### Phase 2: MockGraphExecutor Integration and Simulated Data

**Objective**: Integrate the MockGraphExecutor test harness to generate simulated metrics data and validate the metrics panel displays live data correctly via the MetricsCapability. This phase establishes the data pipeline that will work identically with the real GraphExecutor.

**Deliverables**:
- [ ] MockGraphExecutor implementation:

  - [ ] Implement GraphExecutor API (Init, Start, Run, Stop, Join)
  - [ ] Implement MetricsCapability with metric buffering
  - [ ] Create configurable metrics generation (NodeMetricsSchema JSON)
  - [ ] Implement metrics callback registration
  - [ ] Generate synthetic MetricsEvent objects at configurable Hz
- [ ] MetricsCapability implementation:
  - [ ] Receive MetricsEvent callbacks from executor background thread
  - [ ] Buffer metrics with circular queue (thread-safe)
  - [ ] Implement GetNodeMetricsSchemas() API
  - [ ] Implement GetDiscoveredMetrics() API
  - [ ] Implement GetLatestMetricValue(node_name, metric_name) API
  - [ ] Implement RegisterMetricsCallback() API
- [ ] Dashboard metrics integration:
  - [ ] DashboardApplication receives executor in constructor
  - [ ] Discover metrics from MetricsCapability on Initialize()
  - [ ] Create MetricsTileWindow for each discovered metric
  - [ ] Populate metrics panel with tiles (using grid layout)
  - [ ] Update tiles each frame with latest values from metrics_cap
- [ ] Create mock_graph.json configuration with sample metrics
- [ ] Test complete pipeline: executor → MetricsCapability → dashboard → tiles → display

**Success Criteria**:
- Dashboard discovers all 48 metrics from MockGraphExecutor
- Metrics tiles populate panel and display live values
- Values update at ~30 FPS from executor data at ~199 Hz
- All 54 Phase 0.4 tests still pass
- New integration tests validate metrics flow (target: 13+ new tests)
- Dashboard renders smoothly without blocking UI

**Example Metrics Generated** (from mock_graph.json):
```
12 nodes × 4 metrics/node = 48 total metrics
- Node metrics: throughput_hz, processing_time_ms, queue_depth, error_count
- Display types: value (8), gauge (16), sparkline (12), state (12)
- Synthetic data: constant, linear ramp, random walk, sinusoidal patterns
```

### Phase 3: Enhanced Features (Future)
- [ ] CommandWindow: command input and execution
- [ ] LoggingWindow: log4cxx integration
- [ ] Layout strategies: vertical, horizontal, tabbed fallback
- [ ] Scrolling and pagination
- [ ] Metric filtering and search
- [ ] Custom themes and styling

---

## Quick Reference: Essential Implementation Guidance

### The `gdashboard` Application: Starting Point for Implementation

**`gdashboard` is the primary executable target** that brings together all architectural components. It is where Phase 1 implementation begins.

**Purpose**: 
- Command-line application that launches the FTXUI dashboard
- Manages the 10-step initialization sequence
- Orchestrates the DashboardApplication and executor integration

**Structure**:
```
gdashboard (executable)
  ├── dashboard_main.cpp (entry point, ~50-100 lines)
  │   └── DashboardApplication (orchestrator, ~200-300 lines)
  │       ├── MetricsPanel (40% height)
  │       ├── LoggingWindow (35% height)
  │       ├── CommandWindow (18% height)
  │       └── StatusBar (2% height)
  │
  ├── MockGraphExecutor (generates synthetic metrics)
  │   └── MetricsCapability (buffers and exposes metrics)
  │
  └── FTXUI Components (all FTXUI, no custom wrappers)
```

**Build Configuration**:
```cmake
# In CMakeLists.txt
add_executable(gdashboard
    src/gdashboard/dashboard_main.cpp
    src/gdashboard/DashboardApplication.cpp
    # ... other UI sources when created
)
target_link_libraries(gdashboard
    PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen
    graph_executor     # MockGraphExecutor and MetricsCapability
    nlohmann_json::nlohmann_json
)
```

**Run Command**:
```bash
./gdashboard config/mock_graph.json
```

### MockGraphExecutor Invariant: Test Harness, Production Dashboard

**Critical to understand**: MockGraphExecutor is a **temporary test fixture**. The dashboard itself is **permanent production code**.

| Aspect | Status | Lifespan | Replaceability |
|--------|--------|----------|-----------------|
| **MockGraphExecutor** | Test Harness | Phases 1-2 (validation) | ❌ Temporary - replaced by real GraphExecutor |
| **DashboardApplication** | Production | Permanent | ✅ Final and permanent |
| **MetricsCapability** | Production | Permanent | ✅ Final and permanent |
| **Dashboard UI Components** | Production | Permanent | ✅ Final and permanent |
| **gdashboard Application** | Production | Permanent | ✅ Final and permanent |

**What this means for implementation**:
- Treat all dashboard code (UI, application logic) as **production quality** during Phases 1-2
- MockGraphExecutor is purely for testing and validation - it will be discarded
- MetricsCapability is provided by executor (both Mock and Real use same interface)
- When real GraphExecutor available: replace only the executor instantiation (one line change in `dashboard_main.cpp`)
- No refactoring of dashboard code or UI when switching to real executor
- Thread safety patterns established now will work with both MockGraphExecutor and real GraphExecutor

---

---

### The Critical Initialization Sequence

**Dashboard only works if these steps happen in exactly this order:**

```cpp
// src/gdashboard/dashboard_main.cpp - The ONLY correct sequence

// Step 1: Create executor
auto executor = std::make_shared<MockGraphExecutor>(config);

// Step 2: Initialize executor
executor->Init();

// Step 3: Create application with executor
auto app = std::make_shared<DashboardApplication>(executor);

// Step 4: Initialize application (discovers metrics)
app->Initialize();

// Step 5: START EXECUTOR (metrics begin publishing in background thread)
executor->Start();

// Step 6: WAIT 100ms - LET METRICS ACCUMULATE IN CAPABILITY BUFFER
// ⚠️  THIS IS CRITICAL - Buffer is empty immediately after Start()
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// Step 7: RUN DASHBOARD (main loop, 30 FPS, polls capability each frame)
app->Run();

// Steps 8-9: Cleanup (in reverse order)
executor->Stop();    // Stop executor
executor->Join();    // Join threads
// Dashboard exits cleanly
```

**Why the 100ms wait is CRITICAL**:
- MockGraphExecutor publishes MetricsEvent asynchronously in background thread (199 Hz)
- MetricsCapability buffers these events in circular queue
- Dashboard reads from capability via GetDiscoveredMetrics()
- Buffer is EMPTY immediately after Start()
- Must wait for executor to populate buffer before Run() starts polling
- If skip or reduce wait: Run() finds zero metrics → no updates → stale data

### The Three Core Components

#### 1. **MockGraphExecutor** (Background Thread)
- **What**: Generates synthetic MetricsEvent objects (source, metric_name, value, timestamp)
- **Where**: Runs in background thread started by executor.Start()
- **Rate**: ~199 Hz (5ms intervals for 48 metrics)
- **Publishes**: Via callback to MetricsCapability.OnMetricsEvent(event)
- **Lifecycle**: Init() → Start() → Run() (blocks) → Stop() → Join()
- **Files**: `src/graph/MockGraphExecutor.hpp/cpp` (implements GraphExecutor API)

#### 2. **MetricsCapability** (Synchronization Point)
- **What**: Thread-safe buffer and discovery interface for metrics (provided by executor)
- **Thread Model**: 
  - Executor writes MetricsEvent to buffer (background thread)
  - Dashboard reads via GetDiscoveredMetrics() / GetLatestMetricValue() (main thread)
  - Uses mutex or atomic for synchronization
- **Key APIs**:
  ```cpp
  std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const;
  std::vector<MetricDescriptor> GetDiscoveredMetrics() const;
  std::optional<MetricsSnapshot> GetLatestMetricValue(
      const std::string& node_name,
      const std::string& metric_name) const;
  void RegisterMetricsCallback(std::function<void(const MetricsEvent&)> callback);
  ```
- **Buffer**: Circular queue, auto-discards old events when full
- **Files**: Provided by executor (MockGraphExecutor or GraphExecutor)

#### 3. **DashboardApplication** (Main Thread)
- **What**: Orchestrates UI, discovers metrics, runs 30 FPS pull loop
- **Key Methods**:
  ```cpp
  void Initialize() {
      // Get MetricsCapability from executor
      metrics_cap_ = executor_->GetCapability<MetricsCapability>();
      
      // Discover metrics from capability buffer (called before Start())
      auto descriptors = metrics_cap_->GetDiscoveredMetrics();
      
      // Create tile for each metric
      for (const auto& descriptor : descriptors) {
          auto tile = std::make_shared<MetricsTileWindow>(
              descriptor.node_name,
              descriptor.metric_name);
          metric_tiles_[descriptor.node_name + "::" + descriptor.metric_name] = tile;
          metrics_panel_->AddTile(tile);
      }
  }
  
  void Run() {
      // Main FTXUI loop - 30 FPS
      while (!should_exit_) {
          // Each frame: poll capability for latest values
          for (auto& [key, tile] : metric_tiles_) {
              auto [node, metric] = ParseKey(key);
              if (auto snapshot = metrics_cap_->GetLatestMetricValue(node, metric)) {
                  tile->UpdateMetric(snapshot->current_value);  // Update tile state
              }
          }
          
          // FTXUI renders automatically
          RenderScreen();
          
          // ~30 FPS = 33ms per frame
          std::this_thread::sleep_for(std::chrono::milliseconds(33));
      }
  }
  ```
- **Files**: `include/ui/DashboardApplication.hpp/cpp`

### Data Structures: The Three Key Types

#### **MetricsEvent** (Executor → MetricsCapability)
```cpp
struct MetricsEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string source;              // Node name: "DataValidator"
    std::string event_type;          // "metric_update"
    std::map<std::string, std::string> data;
    // data["metric_name"] = "validation_errors"
    // data["value"] = "45.2"
};
```

#### **MetricDescriptor** (MetricsCapability → Dashboard Discovery)
```cpp
struct MetricDescriptor {
    std::string node_name;           // "DataValidator"
    std::string metric_name;         // "validation_errors"
    double current_value;            // Latest value
    double min_value;                // Min since start
    double max_value;                // Max since start
    std::string display_type;        // "value", "gauge", "sparkline", "state"
};
```

#### **MetricsSnapshot** (For Tile Updates)
```cpp
struct MetricsSnapshot {
    double current_value;
    double min_value;
    double max_value;
    std::chrono::system_clock::time_point timestamp;
};
```

### FTXUI Components Used (No Custom Wrappers)

**Layout**:
- `ftxui::vbox()` - Stack vertically (metrics panel / logging window / command window)
- `ftxui::hbox()` - Stack horizontally (3-column metric grid)
- `ftxui::flexbox()` - Flexible sizing with proportions
- `ftxui::window()` - Bordered containers

**Display**:
- `ftxui::gauge()` - Progress bar for gauge display type
- `ftxui::text()` - Text content for value/sparkline/state tiles
- `ftxui::border()` - Decorative borders
- `ftxui::color()` - Color styling based on alert status

**Interactive**:
- `ftxui::Input()` - Command window text input
- `ftxui::Button()` - Clickable buttons (if needed)
- `ftxui::Renderer()` - Custom element for sparkline rendering
- `ftxui::Container()` - Focus/event management

### Real-time Update Pattern

```cpp
// Each frame (33ms = ~30 FPS)
for (auto& tile : tiles) {
    // 1. Poll harness for latest value (non-blocking, <1ms)
    auto snapshot = harness.GetLatestMetricValue(tile.node, tile.metric);
    
    // 2. Update tile state with new value
    if (snapshot) {
        tile.current_value = snapshot.current_value;
        tile.min_value = snapshot.min_value;
        tile.max_value = snapshot.max_value;
    }
    
    // 3. FTXUI Renderer() re-renders tile on next frame
}

// FTXUI Screen renders automatically
screen.Refresh();
```

### Metrics Panel Sizing Algorithm (Grid Layout)

**Layout Structure**:
- 3-column grid (fixed, always 3 columns)
- Each row height = maximum of all tile heights in that row
- Example: if row contains [value 3L, gauge 5L, sparkline 7L], row height = 7L

**Display Types and Heights**:
- `"value"` → 3 lines (title + value)
- `"gauge"` → 5 lines (title + bar + value)
- `"sparkline"` → 7 lines (title + graph + value)
- `"state"` → 3 lines (title + indicator)

**Example: 48 metrics layout**
```
12 nodes × 4 metrics = 48 total
Assume: 12 sparkline (7L), 24 gauge (5L), 12 value (3L)

Layout: 48 metrics ÷ 3 columns = 16 rows

Row heights (mixed):
- Rows with sparkline: 7 lines
- Rows with only gauge: 5 lines
- Rows with only value: 3 lines

Total panel height: ~110-144 lines (depends on metric distribution)
Available screen: ~30 lines (40% of 75-line terminal)
Result: Panel scrolls, shows 4-5 rows at a time
```

**Fallback to Tabbed Mode**:
If metrics cannot fit in grid with current screen size:
1. Automatically switch to **tabbed mode**
2. Group 6-12 metrics per tab (by node name)
3. User navigates tabs with LEFT/RIGHT arrow keys
4. Each tab displays its metrics in grid or vertical layout

**Note on Optimization**: No premature optimization is required for metric panel sizing. Typical deployments will not have excessive metrics. If throttling becomes necessary in the future, it will be implemented at the source (executor metrics generation) rather than in the dashboard. The automatic fallback to tabbed mode is sufficient for handling variable metric counts.

**Auto-Sizing Tiles**: Metric tiles automatically expand to accommodate text content from `NodeMetricsSchema`. Display type determines minimum height, and actual tile size expands as needed based on metric name length, description, and other displayed text.

### Phase Breakdown for Implementation

**Phase 1** (Days 1-2): Static Window Structure
- Create Window base class, LayoutWindow, window shells
- Build dashboard container with 4 sections (metrics 40%, logs 35%, command 18%, status 2%)
- Verify layout renders without crashes
- All 54 Phase 0.4 tests must pass
- **No metrics data yet**

**Phase 2** (Days 3-4): MockGraphExecutor Integration
- Implement MockGraphExecutor with full GraphExecutor API
- Implement MetricsCapability with buffer + APIs
- Integrate metrics panel: discover → tiles → updates → render
- Create mock_graph.json with 48 metrics
- Test complete pipeline: executor → MetricsCapability → discovery → tiles → display
- **Success**: All 48 metrics show live values, 30 FPS rendering, no UI blocking

**Phase 3** (Days 5+): Polish and Features
- CommandWindow: command parsing, execution
- LoggingWindow: log4cxx integration, filtering
- Layout strategies: vertical, horizontal, tabbed fallback
- Scrolling, pagination, search, themes

### Architecture Decisions & Why

| Decision | Why | Alternative Considered |
|----------|-----|------------------------|
| Use FTXUI directly, no custom wrappers | Mature, proven, simpler | Custom Window framework |
| Display types in NodeMetricsSchema | Metric owns how it renders | Dashboard hardcodes rendering |
| 3-column grid layout | Balanced, fits 48 metrics on terminal | 2-column or 4-column |
| 30 FPS pull pattern, not callbacks | Non-blocking UI, simpler state | Event-driven with callbacks |
| 100ms wait after executor start | Enough time for buffer to populate | 50ms (too little), 200ms (too slow) |
| Circular queue in harness | Bounded memory, auto-discard old | Unbounded queue (memory leak) |
| Thread-safe buffer synchronization | Multiple threads (executor + UI) | Assumes single thread (unsafe) |

---

### Phase 0 (Archive): MockGraphExecutor for Dashboard Validation

As an initial step, a **MockGraphExecutor** is implemented to validate and develop the dashboard independently from the real graph execution engine. This mock implementation:

1. **Implements the real GraphExecutor API** from `include/graph/GraphExecutor.hpp`:
   - Constructor that accepts graph configuration
   - Full lifecycle methods: `Init()`, `Start()`, `Run()`, `Stop()`, `Join()`
   - State queries: `GetState()`, `GetStats()`
   - Capability discovery: `GetCapabilities()` returns realistic node metrics schemas
   - Callback registration: `RegisterMetricsCallback()` for metric injection

2. **Simulates graph execution with synthetic data**:
   - Periodic generation of `MetricsEvent` objects with realistic metrics
   - Synthetic node state transitions (RUNNING, PAUSED, STOPPED)
   - Configurable metrics schemas (from JSON configuration)
   - Controllable timing and metric generation rates for testing

3. **Enables dashboard development without executor**:
   - Validates window framework, layout system, and rendering
   - Tests metric discovery and callback installation
   - Tests UI responsiveness under high-frequency metric updates
   - Validates command processing pipeline
   - Tests error handling and logging integration
   - Allows performance profiling and optimization

4. **Integration points remain external**:
   - The GraphExecutor itself is used only through its API boundary
   - Actual graph construction, node implementation, and data flow are external
   - Dashboard makes no assumptions about executor internals
   - When real executor is integrated, only the API implementation changes; dashboard code unchanged

**MockGraphExecutor Implementation Sketch**:
```cpp
namespace graph {
    // Mock implementation for testing and validation
    class MockGraphExecutor : public GraphExecutor {
    public:
        explicit MockGraphExecutor(const nlohmann::json& config);
        
        // Lifecycle management
        void Init() override;
        void Start() override;
        void Run() override;
        void Stop() override;
        int Join() override;
        
        // State and stats
        ExecutorState GetState() const override;
        ExecutionStats GetStats() const override;
        
        // Capability discovery (returns mock schemas)
        GraphCapabilities GetCapabilities() const override;
        
        // Metrics callback registration
        void RegisterMetricsCallback(const std::string& node_name,
                                    std::function<void(const MetricsEvent&)> callback) override;
        
    private:
        // Mock data generation
        void MetricsGenerationLoop();
        void GenerateNodeMetric(const std::string& node_name, 
                               const std::string& metric_name);
        
        nlohmann::json config_;
        ExecutorState current_state_ = ExecutorState::Uninitialized;
        ExecutionStats current_stats_;
        
        // Registered callbacks for each node
        std::map<std::string, std::function<void(const MetricsEvent&)>> callbacks_;
        
        // Mock metrics generation thread
        std::thread metrics_thread_;
        std::atomic<bool> should_stop_ = false;
        
        // Configurable parameters
        int metrics_generation_hz_ = 10;  // Generate metrics at 10 Hz
        int update_interval_ms_ = 100;    // 100ms between metric updates
    };
}
```

**Configuration Example** (mock_graph.json):
```json
{
  "mock_executor": {
    "nodes": [
      {
        "name": "DataSource",
        "metrics": [
          {
            "name": "records_read",
            "type": "int",
            "unit": "count",
            "initial_value": 0,
            "increment_per_update": 5
          },
          {
            "name": "read_rate_hz",
            "type": "double",
            "unit": "hz",
            "initial_value": 50.0,
            "variance": 5.0
          }
        ]
      },
      {
        "name": "ProcessorA",
        "metrics": [
          {
            "name": "processing_time_ms",
            "type": "double",
            "unit": "ms",
            "min_value": 10.0,
            "max_value": 50.0
          },
          {
            "name": "queue_depth",
            "type": "int",
            "unit": "count",
            "min_value": 0,
            "max_value": 20
          },
          {
            "name": "throughput_hz",
            "type": "double",
            "unit": "hz",
            "initial_value": 100.0
          }
        ]
      },
      {
        "name": "OutputSink",
        "metrics": [
          {
            "name": "records_written",
            "type": "int",
            "unit": "count"
          }
        ]
      }
    ],
    "execution_profile": {
      "duration_seconds": 60,
      "state_transitions": [
        { "time_seconds": 0, "state": "Initialized" },
        { "time_seconds": 2, "state": "Running" },
        { "time_seconds": 58, "state": "Stopping" },
        { "time_seconds": 60, "state": "Stopped" }
      ]
    }
  }
}
```

**TODO (Phase 0)**:
- [ ] Create MockGraphExecutor class implementing GraphExecutor API
- [ ] Implement lifecycle methods (Init, Start, Run, Stop, Join)
- [ ] Implement GetCapabilities() returning synthetic NodeMetricsSchema objects
- [ ] Implement RegisterMetricsCallback() with callback storage
- [ ] Create metrics generation thread and event loop
- [ ] Implement configurable metrics schemas from JSON
- [ ] Add configurable metric generation rates (Hz)
- [ ] Implement state transitions (Uninitialized → Initialized → Running → Stopped)
- [ ] Add synthetic data generation with configurable patterns (constant, linear, random)
- [ ] Create mock_graph.json configuration file with sample nodes and metrics
- [ ] Test with dashboard: validate metric discovery, tile creation, updates
- [ ] Test with dashboard: validate command processing (run_graph, stop_execution, etc.)
- [ ] Verify UI remains responsive under high-frequency metric updates
- [ ] Document integration: real executor replaces mock, no dashboard code changes

---

### Phase 1: Core Framework
- [ ] Window base class and interfaces
- [ ] Basic layout system
- [ ] Constraint resolution algorithm
- [ ] Event handling framework

### Phase 2: Window Types
- [ ] StaticWindow
- [ ] DataWindow
- [ ] GaugeWindow
- [ ] TableWindow

### Phase 3: Advanced Features
- [ ] GraphWindow with charting
- [ ] Scrollable content
- [ ] Async data updates
- [ ] Theme/styling system

### Phase 4: Application Tools
- [ ] Dashboard builder utilities
- [ ] Configuration system
- [ ] Data provider libraries
- [ ] Example applications

---

## Implementation and Architectural Issues

This section identifies key technical challenges and considerations that must be addressed during implementation.

### 1. Thread Safety and Concurrency

**Issue**: The dashboard runs in the main FTXUI thread, while the graph executor runs in separate worker threads. Metrics and status updates from the executor must be safely accessed by the dashboard without causing data races or blocking the UI.

**Architecture**: The system uses a centralized **MetricsQueue** (based on `core::ActiveQueue<MetricsEvent>`) with a **MetricsPublisher** thread pattern:

1. **Metrics Data Flow**:
   - Graph executor nodes generate `app::metrics::MetricsEvent` objects during execution
   - Each node routes its metrics to a thread-safe **MetricsQueue** (based on `core::ActiveQueue<app::metrics::MetricsEvent>`)
   - The `ActiveQueue` class provides:
     - Thread-safe enqueue/dequeue operations with optional bounded capacity
     - Non-blocking enqueue/dequeue methods (`DequeueNonBlocking()`)
     - Optional metrics collection on queue operations (depth, throughput, latency)
     - Disable/enable mechanism for graceful shutdown
   - A dedicated **MetricsPublisher** thread continuously dequeues metrics from this queue
   - MetricsPublisher publishes metrics to all registered subscribers via `IMetricsSubscriber` interface
   - The **Dashboard** is one subscriber among potentially many others

2. **Key Classes**:

   ```cpp
   // From include/core/ActiveQueue.hpp
   namespace core {
       template<class Element> class ActiveQueue {
       public:
           // Enqueue with optional bounded capacity and blocking behavior
           bool Enqueue(Element element);
           
           // Non-blocking dequeue
           bool DequeueNonBlocking(Element& element);
           
           // Blocking dequeue with optional timeout
           bool Dequeue(Element& element, 
                       std::chrono::milliseconds timeout = 
                           std::chrono::milliseconds{-1});
           
           // Queue control
           void DisableQueue();   // Stop accepting new items
           void EnableQueue();    // Resume operations
           void Clear();          // Empty queue
           
           // Optional metrics collection
           void EnableMetrics(bool enabled = true);
           const QueueMetrics& GetMetrics() const;
       };
       
       struct QueueMetrics {
           std::atomic<uint64_t> enqueued_count;
           std::atomic<uint64_t> dequeued_count;
           std::atomic<uint64_t> enqueue_rejections;
           std::atomic<uint64_t> max_size_observed;
           uint64_t total_enqueue_time_ns;
           uint64_t total_dequeue_time_ns;
       };
   }
   
   // From include/app/metrics/MetricsEvent.hpp
   namespace app::metrics {
       struct MetricsEvent {
           std::chrono::system_clock::time_point timestamp;
           std::string source;       // Node name
           std::string event_type;   // "phase_transition", "completion_status", etc.
           std::map<std::string, std::string> data;
       };
   }
   
   // From include/app/metrics/IMetricsSubscriber.hpp
   namespace app::metrics {
       class IMetricsSubscriber {
       public:
           virtual ~IMetricsSubscriber() = default;
           
           // Called by MetricsPublisher when event is dequeued
           virtual void OnMetricsEvent(const MetricsEvent& event) = 0;
       };
   }
   ```

3. **Dashboard as Subscriber**:
   ```cpp
   class GraphExecutionDashboard : public app::metrics::IMetricsSubscriber {
   public:
       // Called by MetricsPublisher thread when metrics are published
       void OnMetricsEvent(const app::metrics::MetricsEvent& event) override {
           // Route metrics to appropriate panels/tiles
           RouteMetricsToPanel(event);
       }
       
   private:
       void RouteMetricsToPanel(const app::metrics::MetricsEvent& event) {
           // Extract metric details from event
           std::string source_node = event.source;
           std::string event_type = event.event_type;
           
           // Find the corresponding metric tile window
           // Key format: "source_node:metric_name"
           auto key = source_node + ":" + ExtractMetricName(event);
           
           if (auto tile = metric_tiles_.find(key); tile != metric_tiles_.end()) {
               tile->second->UpdateMetric(event);
           }
       }
       
       std::map<std::string, std::shared_ptr<MetricTileWindow>> metric_tiles_;
   };
   ```

**Design Considerations**:

- **ActiveQueue Features**:
  - Thread-safe operations (mutex + condition variables internally)
  - Optional bounded capacity with configurable overflow behavior (drop/block)
  - Built-in metrics for monitoring queue depth, throughput, and latency
  - Non-blocking mode for producer-consumer patterns
  - Graceful shutdown with disable/enable mechanism
  
- **Multiple Subscribers**: The metrics queue design supports any number of subscribers consuming the same metrics stream without interference

- **Non-blocking Publishing**: MetricsPublisher dequeues from `ActiveQueue` and delivers to subscribers synchronously but quickly, minimizing latency

- **Dashboard Routing**: Dashboard's `OnMetricsEvent()` callback must route metrics efficiently using index lookup (hash map by `(source_node, metric_name)`)

- **Subscriber Lifetime**: Subscribers must be added/removed safely from MetricsPublisher without disrupting publishing

- **Atomic Executor State**: Executor state (RUNNING, PAUSED, etc.) should use atomics for low-latency reads independent of metrics queue

- **Queue Metrics**: Leverage `ActiveQueue::GetMetrics()` to monitor queue health in dashboard status bar

**Challenges**:
- **Callback Speed**: Subscribers' `OnMetricsEvent()` must process events quickly; slow callbacks block the publisher thread and delay other subscribers
- **Metric Routing**: Dashboard must efficiently map `(source_node, event_type)` to appropriate tile window
- **Subscriber Cleanup**: Removing subscribers during publishing requires safe list management
- **Queue Overflow**: If metrics are produced faster than consumed, the bounded queue may drop events (tracked in `QueueMetrics::enqueue_rejections`)
- **Event Loss**: Dropped events due to queue capacity limits should be tracked and monitored

**Solutions**:

```cpp
// Metric routing index for O(1) lookups
class MetricRoutingIndex {
public:
    struct TileKey {
        std::string source_node;
        std::string metric_name;
        
        bool operator==(const TileKey& other) const {
            return source_node == other.source_node && 
                   metric_name == other.metric_name;
        }
    };
    
    struct TileKeyHash {
        size_t operator()(const TileKey& key) const {
            return std::hash<std::string>()(key.source_node + ":" + key.metric_name);
        }
    };
    
    void RegisterTile(const std::string& source_node,
                     const std::string& metric_name,
                     MetricTileWindow* tile) {
        std::shared_lock<std::shared_mutex> lock(routing_lock_);
        routing_map_[{source_node, metric_name}] = tile;
    }
    
    MetricTileWindow* FindTile(const std::string& source_node,
                              const std::string& metric_name) const {
        std::shared_lock<std::shared_mutex> lock(routing_lock_);
        auto it = routing_map_.find({source_node, metric_name});
        return (it != routing_map_.end()) ? it->second : nullptr;
    }
    
private:
    std::unordered_map<TileKey, MetricTileWindow*, TileKeyHash> routing_map_;
    mutable std::shared_mutex routing_lock_;
};

// Thread-safe subscriber list with snapshot pattern
class SubscriberList {
public:
    void AddSubscriber(app::metrics::IMetricsSubscriber* subscriber) {
        std::lock_guard<std::mutex> lock(subscribers_lock_);
        subscribers_.push_back(subscriber);
    }
    
    void RemoveSubscriber(app::metrics::IMetricsSubscriber* subscriber) {
        std::lock_guard<std::mutex> lock(subscribers_lock_);
        auto it = std::find(subscribers_.begin(), subscribers_.end(), subscriber);
        if (it != subscribers_.end()) {
            subscribers_.erase(it);
        }
    }
    
    std::vector<app::metrics::IMetricsSubscriber*> GetSnapshot() const {
        std::lock_guard<std::mutex> lock(subscribers_lock_);
        return subscribers_;  // Copy for publisher to iterate without holding lock
    }
    
private:
    mutable std::mutex subscribers_lock_;
    std::vector<app::metrics::IMetricsSubscriber*> subscribers_;
};

// MetricsPublisher loop pseudocode
void MetricsPublisher::PublishLoop() {
    app::metrics::MetricsEvent event;
    
    while (!should_stop_.load(std::memory_order_acquire)) {
        // Non-blocking dequeue from ActiveQueue
        if (metrics_queue_.DequeueNonBlocking(event)) {
            // Get snapshot of current subscribers
            auto subscribers = subscriber_list_.GetSnapshot();
            
            // Publish to all subscribers without holding lock
            for (auto subscriber : subscribers) {
                try {
                    subscriber->OnMetricsEvent(event);
                } catch (const std::exception& e) {
                    LOG4CXX_ERROR(logger, "Subscriber exception: " << e.what());
                }
            }
        } else {
            // Queue empty, check metrics and sleep briefly
            const auto& metrics = metrics_queue_.GetMetrics();
            if (metrics.enqueue_rejections.load() > 0) {
                LOG4CXX_WARN(logger, "Queue rejections: " 
                    << metrics.enqueue_rejections.load());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

// Monitor queue health in dashboard status bar
class StatusBar {
private:
    void UpdateQueueMetrics(const core::ActiveQueue<MetricsEvent>& queue) {
        const auto& metrics = queue.GetMetrics();
        queue_depth_ = metrics.current_size.load();
        dropped_events_ = metrics.enqueue_rejections.load();
        throughput_hz_ = ComputeThroughput(metrics);
    }
};
```

**Metrics Queue Capacity**:

- **Default**: Unbounded capacity for production use
- **Optional Limiting**: Command-line parameter `--metrics-queue-capacity=<N>` can restrict queue size
  - Enables testing with bounded queues
  - Helps detect overflow conditions
  - Useful for resource-constrained environments
- **Overflow Behavior**: When capacity limit reached, enqueue requests fail (tracked in QueueMetrics::enqueue_rejections)

**TODO**:
- [ ] Initialize `ActiveQueue<MetricsEvent>` with unbounded capacity by default
- [ ] Configure `ActiveQueue` metrics collection: `queue.EnableMetrics(true)`
- [ ] Implement metric routing index for O(1) dashboard lookups
- [ ] Implement thread-safe subscriber list with snapshot pattern
- [ ] Implement efficient event routing in `OnMetricsEvent()` callback
- [ ] Monitor queue health: depth, throughput, dropped events (if capacity limited)
- [ ] Log queue overflow events when capacity limit triggered
- [ ] Profile `OnMetricsEvent()` callback latency to ensure < 1ms
- [ ] Test with high-frequency metrics: baseline with unbounded queue
- [ ] Add command-line argument parser for --metrics-queue-capacity option

---

### 2. Responsive UI and Non-Blocking Operations

**Issue**: The dashboard UI must remain responsive while the executor is running. Multiple independent operations (metrics collection, command processing, logging, rendering) must not block each other or the FTXUI event loop.

**Architecture**: The dashboard employs a multi-threaded architecture with independent data pipelines for metrics, command processing, and logging. Each pipeline operates asynchronously and communicates via thread-safe queues.

#### 2.1: Metrics Pipeline (Independent)

Graph metrics collection runs completely independently from the UI thread and command processing:

```cpp
// Metrics flow: Executor → MetricsQueue → MetricsPublisher → Dashboard (IMetricsSubscriber)
// 
// The dashboard receives OnMetricsEvent() callbacks from MetricsPublisher thread.
// Metric updates do not block command input or command result display.
//
// The dashboard implements IMetricsSubscriber and routes metrics to appropriate tiles:

class GraphExecutionDashboard : public app::metrics::IMetricsSubscriber {
public:
    void OnMetricsEvent(const app::metrics::MetricsEvent& event) override {
        // Non-blocking metric routing via index lookup
        auto tile = metric_routing_index_.FindTile(event.source, 
                                                    ExtractMetricName(event));
        if (tile) {
            tile->UpdateMetric(event);  // Queue update, don't block
        }
    }
    
private:
    MetricRoutingIndex metric_routing_index_;  // O(1) lookup by (source, name)
};
```

**Key Properties**:
- Metrics collected and published independently of command processing
- Dashboard routes metrics to tiles via non-blocking index lookup
- No lock contention with command input/output threads
- Metric updates queued to tile windows but don't block rendering

#### 2.2: Command Processing Pipeline (Three Independent Threads)

Command processing is divided into three independent threads that communicate via thread-safe queues:

```
┌──────────────────────────────────────────────────────────────┐
│               FTXUI Main Thread (Rendering)                  │
│  - Processes keyboard events                                  │
│  - Routes text input to CommandDecoderQueue                  │
│  - Renders UI (metrics, logs, command output)                │
│  - Does NOT wait for command results                         │
└──────────────────────────────────────────────────────────────┘
        │
        │ Keyboard input events
        ▼
┌──────────────────────────────────────────────────────────────┐
│  CommandDecoder Thread                                        │
│  - Receives raw user input from input queue                   │
│  - Tokenizes input according to command grammar              │
│  - Manages command history (up/down arrows, search)           │
│  - Validates syntax before processing                         │
│  - Enqueues parsed command to CommandProcessorQueue          │
└──────────────────────────────────────────────────────────────┘
        │
        │ Parsed, validated commands
        ▼
┌──────────────────────────────────────────────────────────────┐
│  CommandProcessor Thread (or executor thread)                │
│  - Receives tokens commands from processor queue              │
│  - Executes command logic (may call executor methods)         │
│  - Generates CommandResult with status/output                │
│  - Enqueues result to CommandResultQueue                     │
└──────────────────────────────────────────────────────────────┘
        │
        │ Command results
        ▼
┌──────────────────────────────────────────────────────────────┐
│  CommandResult Thread                                         │
│  - Waits on CommandResultQueue (blocking dequeue)            │
│  - Formats result for display                                │
│  - Updates CommandWindow display buffer                      │
│  - Runs independently of command input                       │
└──────────────────────────────────────────────────────────────┘
        │
        │ Result display updates
        ▼
┌──────────────────────────────────────────────────────────────┐
│  FTXUI Rendering                                              │
│  - Displays command results in CommandWindow                 │
└──────────────────────────────────────────────────────────────┘
```

#### 2.3: CommandDecoder Thread

Responsible for parsing and validating user input:

```cpp
class CommandGrammar {
public:
    struct TokenRule {
        std::string name;
        std::regex pattern;         // Regex pattern for token matching
        bool optional = false;       // Whether token is optional
        std::string description;     // Human-readable description
    };
    
    struct CommandDefinition {
        std::string name;
        std::string description;
        std::vector<TokenRule> token_rules;  // Regex-based token rules
    };
    
    // Runtime command registration with regex patterns
    // Not JSON-based; registered directly as CommandDefinition structs
    void DefineCommand(const CommandDefinition& cmd);
    
    // Validate and tokenize input
    struct ParseResult {
        bool success;
        std::string command_name;
        std::map<std::string, std::string> parameters;  // Extracted by regex groups
        std::string error_message;
    };
    
    ParseResult Parse(const std::string& user_input);
    
private:
    std::map<std::string, CommandDefinition> commands_;
};

class CommandDecoder {
public:
    CommandDecoder(const CommandGrammar& grammar,
                   core::ActiveQueue<std::string>& input_queue,
                   core::ActiveQueue<ParsedCommand>& output_queue);
    
    void Start();
    void Stop();
    
private:
    void DecoderLoop();
    
    const CommandGrammar& grammar_;
    core::ActiveQueue<std::string>& input_queue_;      // From UI
    core::ActiveQueue<ParsedCommand>& output_queue_;   // To processor
    
    std::thread decoder_thread_;
    std::atomic<bool> should_stop_ = false;
    
    // Command history management
    std::deque<std::string> history_;
    size_t history_index_ = 0;
    static constexpr size_t MAX_HISTORY = 100;
    
    // Helper methods
    ParsedCommand::ParseResult ValidateAndTokenize(const std::string& input);
    void UpdateHistory(const std::string& command);
    std::string GetHistoryEntry(int offset);  // offset: -1 (older), +1 (newer)
};

struct ParsedCommand {
    struct ParseResult {
        bool success;
        std::string command_name;
        std::map<std::string, std::string> parameters;
        std::string error_message;
    };
    
    ParseResult parse_result;
    std::chrono::system_clock::time_point timestamp;
    std::string original_input;  // For history
};
```

**CommandDecoder Responsibilities**:
- Receive raw keyboard input from CommandWindow
- Tokenize according to runtime-supplied `CommandGrammar` (regex-based patterns)
- Validate syntax (report errors to UI immediately)
- Manage command history (up/down arrow navigation, search)
- Enqueue parsed commands to command processor
- Non-blocking: returns immediately after enqueue (or reports queue full)

**Command Registration**:
- Commands added at runtime (not from JSON configuration)
- Regex-based grammar patterns for flexible token matching
- Example registration:
  ```cpp
  grammar.DefineCommand({
      .name = "run_graph",
      .description = "Start graph execution",
      .token_rules = {
          {.name = "command", .pattern = std::regex("run_graph"), .optional = false},
          {.name = "graph_name", .pattern = std::regex("[a-zA-Z_][a-zA-Z0-9_]*"), .optional = true}
      }
  });
  ```

#### 2.4: CommandProcessor Thread

Executes parsed commands:

```cpp
struct Command {
    std::string command_name;
    std::map<std::string, std::string> parameters;
    std::chrono::system_clock::time_point timestamp;
};

struct CommandResult {
    std::string command_name;
    bool success;
    std::string output;           // User-visible result/error message
    std::string status;           // "OK", "ERROR", "PENDING"
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds execution_time;
};

using CommandHandler = std::function<CommandResult(const Command&)>;

class CommandProcessor {
public:
    CommandProcessor(core::ActiveQueue<ParsedCommand>& input_queue,
                    core::ActiveQueue<CommandResult>& result_queue);
    
    void RegisterCommandHandler(const std::string& command_name,
                               CommandHandler handler);
    
    void Start();
    void Stop();
    
private:
    void ProcessorLoop();
    
    core::ActiveQueue<ParsedCommand>& input_queue_;    // From decoder
    core::ActiveQueue<CommandResult>& result_queue_;   // To result thread
    
    std::map<std::string, CommandHandler> handlers_;
    std::thread processor_thread_;
    std::atomic<bool> should_stop_ = false;
    
    CommandResult ExecuteCommand(const ParsedCommand& cmd);
};
```

**CommandProcessor Responsibilities**:
- Receive tokenized, validated commands from CommandDecoder
- Look up appropriate command handler
- Execute handler (may be synchronous or async via callback)
- Generate CommandResult with status and output
- Enqueue result to CommandResultQueue
- Handle errors and timeouts

#### 2.5: CommandResult Thread

Displays command results independently of input:

```cpp
class CommandResultDisplay {
public:
    CommandResultDisplay(core::ActiveQueue<CommandResult>& result_queue,
                        CommandWindow& display_window);
    
    void Start();
    void Stop();
    
private:
    void ResultLoop();
    
    core::ActiveQueue<CommandResult>& result_queue_;   // From processor
    CommandWindow& display_window_;                     // UI to update
    
    std::thread result_thread_;
    std::atomic<bool> should_stop_ = false;
    
    void DisplayResult(const CommandResult& result);
    std::string FormatResultForDisplay(const CommandResult& result);
};
```

**CommandResultDisplay Responsibilities**:
- Wait on CommandResultQueue (blocking dequeue)
- Retrieve results as they become available
- Format result for display (timestamp, status, output)
- Update CommandWindow display buffer
- Run independently: never blocks command input or processing

#### 2.6: Command Input Handler - Asynchronous Design (Decision: Option A)

**Design Decision**: The FTXUI main thread uses **Option A: Asynchronous (Fire-and-Forget)** for command input handling.

**Implementation**:
```cpp
// Main FTXUI thread handles keyboard input
bool CommandWindow::HandleEvent(ftxui::Event event) {
    if (event.is_character()) {
        std::string input(1, event.character());
        
        // Non-blocking: try to enqueue, report if queue full
        if (!input_queue_.Enqueue(input)) {
            DisplayError("Command input queue full, try again");
        }
        // Return immediately, don't wait
        return true;
    }
    return false;
}
```

**Key Benefits**:
- Maintains 30+ FPS rendering regardless of command processing speed
- CommandResult thread independently updates CommandWindow display
- Syntax errors from CommandDecoder can be displayed in real-time
- User input never blocked
- Trade-off: very brief delay between command entry and result display (acceptable for command-line interaction)

#### 2.7: Frame Rate Limiting and Rendering Deadline

Ensure UI rendering meets target frame rate despite concurrent operations:

```cpp
class FrameRateLimiter {
public:
    static constexpr int TARGET_FPS = 30;  // ~33ms per frame
    static constexpr int FRAME_DEADLINE_MS = 1000 / TARGET_FPS;
    
    void BeginFrame();
    bool EndFrame();  // Returns true if deadline met, false if overrun
    
    double GetFrameTimeMs() const;
    double GetCPUUsagePercent() const;
    
private:
    std::chrono::steady_clock::time_point frame_start_;
    std::deque<double> recent_frame_times_;  // Last 60 frames for smoothing
};

// In main FTXUI loop:
void DashboardApplication::MainLoop() {
    while (!should_exit_) {
        frame_limiter_.BeginFrame();
        
        // Render current state (metrics, logs, command output)
        // All updated independently via metrics/result callbacks
        auto screen = ftxui::Screen::Create(Dimension::Full);
        Render(screen);
        screen.Print();
        
        // Check if rendering exceeded frame deadline
        if (!frame_limiter_.EndFrame()) {
            LOG4CXX_WARN(logger, "Frame render exceeded deadline");
        }
    }
}
```

#### 2.8: Data Flow Summary

```
Executor (GraphExecutor, separate processes)
├── Metrics Events
│   └─→ MetricsQueue (core::ActiveQueue)
│       └─→ MetricsPublisher (thread)
│           └─→ Dashboard::OnMetricsEvent() (metric routing thread)
│               └─→ MetricTileWindow::UpdateMetric()
│
└── Command Execution
    └─→ CommandProcessor (thread)
        └─→ CommandResultQueue
            └─→ CommandResultDisplay (result thread)
                └─→ CommandWindow display buffer


User Input (FTXUI Main Thread)
├── Keyboard event
│   └─→ CommandWindow::HandleEvent()
│       └─→ CommandInputQueue
│           └─→ CommandDecoder (decoder thread)
│               ├─ Tokenize, validate
│               ├─ Manage history
│               └─→ CommandProcessorQueue
│
└── Rendering
    ├─ Read metrics tiles (updated by metrics thread)
    ├─ Read command output (updated by result thread)
    ├─ Read logs (from logging window)
    └─→ FTXUI Screen (frame limiter enforces deadline)
```

**Key Invariants**:
- Metrics updates do NOT block command processing
- Command input does NOT block result display
- Result display does NOT block command input
- UI rendering deadline (33ms @ 30 FPS) enforced independently

**Challenges**:
- **Synchronization**: Metrics and results updated concurrently with rendering
- **Display staleness**: Rendered frame may show slightly stale metrics/results (acceptable for 33ms window)
- **User experience**: Asynchronous option has brief delay between Enter and result display
- **Command queue overflow**: If decoder/processor slower than user input, queue may fill
- **Memory**: Need to bound history, result log, and metric tile caches

**TODO**:
- [ ] Define CommandGrammar format with regex-based patterns
- [ ] Implement CommandDecoder with history management
- [ ] Implement CommandProcessor with handler registration
- [ ] Implement CommandResultDisplay for result routing
- [ ] Implement FrameRateLimiter with deadline enforcement
- [ ] Add frame time telemetry to status bar
- [ ] Test with high-frequency metrics and rapid command input
- [ ] Profile thread context switching overhead
- [ ] Add queue depth monitoring (command queues should not accumulate)

---

### 3. Scalability: Few Nodes with Selective Metrics

**Issue**: Most graphs are small (10-15 nodes) but not all nodes generate metrics. The dashboard must discover which nodes have metrics, install callbacks to capture those metrics, and only create UI panels for discovered metrics.

**Design**: Metric discovery and callback installation happens once at dashboard initialization. All metrics are then routed through the centralized `MetricsQueue` via pre-installed callbacks.

#### 3.1: Upfront Metric Discovery and Callback Installation

At dashboard startup, the system:
1. Queries executor capabilities to identify nodes with metrics
2. For each node with metrics, installs a callback function
3. Each callback enqueues discovered metrics to the shared `MetricsQueue`
4. Creates UI tile panels only for actually-discovered metrics
5. Registers metric routing entries in the `MetricRoutingIndex`

No runtime concern about null metrics—all decisions made upfront.

```cpp
class GraphExecutionDashboard : public app::metrics::IMetricsSubscriber {
public:
    void Initialize() override {
        // Get executor capabilities
        auto capabilities = executor_->GetCapabilities();
        auto node_schemas = capabilities.GetNodeMetricsSchemas();
        
        // Discover metrics and install callbacks
        for (const auto& schema : node_schemas) {
            if (schema.metrics_schema.empty()) {
                continue;  // Skip silent nodes
            }
            
            InstallMetricsCallbackForNode(schema);
        }
        
        // Determine grid layout based on actual discovered metrics
        int metric_count = metric_tiles_.size();
        int columns = std::min(3, (metric_count + 2) / 3);
        metrics_panel_->SetGridLayout(columns);
    }
    
private:
    void InstallMetricsCallbackForNode(
        const app::metrics::NodeMetricsSchema& schema) {
        
        const auto& node_name = schema.node_name;
        
        // Extract metrics from schema and create tiles
        if (schema.metrics_schema.contains("fields")) {
            for (const auto& field : schema.metrics_schema["fields"]) {
                std::string metric_name = field["name"];
                
                // Create metric tile window
                auto tile = std::make_shared<MetricTileWindow>(
                    metric_name, 
                    std::optional<nlohmann::json>(field),
                    [this, node_name, metric_name]() {
                        return GetMetricValue(node_name, metric_name);
                    });
                
                std::string key = node_name + ":" + metric_name;
                metric_tiles_[key] = tile;
                metric_routing_index_.RegisterTile(node_name, metric_name, tile.get());
                metrics_panel_->AddChild(tile);
            }
        }
    }
};
```

#### 3.2: Callback Installation and Lifetime Management

**Callback Mechanism**:

For each discovered node with metrics, a callback is installed that captures metric updates and enqueues them. Callbacks are implemented as dynamic_cast references to Nodes in the graph.

**Callback Lifetime**:

- **Installation**: Callbacks are registered during `Initialize()` phase (single-threaded)
- **Active Period**: Callbacks remain active throughout graph execution
- **Removal**: Callbacks are explicitly removed **after `Join()` completes but before GraphExecutor is destroyed**
- **Ownership**: Nodes are owned by GraphManager and only deleted when GraphManager exits
- **Safety**: No special synchronization needed for callback cleanup since removal happens post-Join() when no execution threads are active

This ensures:
- Callbacks remain valid while executor is running
- No dangling pointers after cleanup
- Safe execution context for cleanup operations

**Implementation Sketch**:

```cpp
// Callback installed at initialization
// Called by executor/node when metric is updated
struct MetricsCallback {
    std::string source_node;
    std::string metric_name;
    core::ActiveQueue<app::metrics::MetricsEvent>* metrics_queue;
    
    void OnMetricUpdated(double value) {
        app::metrics::MetricsEvent event;
        event.timestamp = std::chrono::system_clock::now();
        event.source = source_node;
        event.event_type = "metric_update";
        event.data["metric_name"] = metric_name;
        event.data["value"] = std::to_string(value);
        
        // Enqueue to shared metrics queue (non-blocking)
        metrics_queue->Enqueue(event);
    }
};

// Install callbacks for all discovered metrics
void InstallCallbacksForNode(
    const app::metrics::NodeMetricsSchema& schema,
    core::ActiveQueue<app::metrics::MetricsEvent>& metrics_queue) {
    
    for (const auto& field : schema.metrics_schema["fields"]) {
        std::string metric_name = field["name"];
        
        // Create callback for this metric
        auto callback = std::make_shared<MetricsCallback>();
        callback->source_node = schema.node_name;
        callback->metric_name = metric_name;
        callback->metrics_queue = &metrics_queue;
        
        // Register callback with executor/node
        // executor->RegisterMetricsCallback(callback);
        // (Exact mechanism depends on executor API)
    }
}
```

#### 3.3: Data Flow After Initialization

Once callbacks are installed at startup, metrics flow automatically:

```
┌─────────────────────────────────────────┐
│  Node executes, updates metric value    │
│  (inside executor thread)               │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  Pre-installed Callback fires           │
│  (node → callback)                      │
│  Constructs MetricsEvent                │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  Enqueue to MetricsQueue                │
│  (callback → queue, non-blocking)       │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  MetricsPublisher dequeues              │
│  (publisher thread)                     │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  Dashboard::OnMetricsEvent()            │
│  (subscriber callback)                  │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  MetricRoutingIndex lookup              │
│  (O(1) by source_node:metric_name)      │
└────────────┬────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────┐
│  MetricTileWindow::UpdateMetric()       │
│  (tile updates with new value)          │
└─────────────────────────────────────────┘
```

#### 3.4: Typical Scale Example

**12-Node Graph with Selective Metrics**:

```
Graph: DataProcessingPipeline

Nodes with metrics discovered at startup:
├─ DataValidator (2 metrics discovered)
│   ├─ validation_errors (callback installed)
│   └─ throughput_hz (callback installed)
├─ Transformer (3 metrics discovered)
│   ├─ processing_time_ms
│   ├─ queue_depth
│   └─ output_rate
├─ Filter (2 metrics discovered)
├─ Aggregator (5 metrics discovered)
├─ Encoder (1 metric discovered)
├─ NetworkSender (3 metrics discovered)
├─ StateMachine (4 metrics discovered)
├─ Archiver (2 metrics discovered)
└─ LogProcessor (1 metric discovered)

Nodes with NO metrics (silent):
├─ CSVReader (no metrics schema)
├─ MetricsCollector (internal only)
└─ Shutdown (terminal node)

Result at initialization:
├─ 23 metrics discovered
├─ 23 callbacks installed
├─ 23 tile panels created
├─ 23 routing entries registered
└─ Layout: 3 columns × 8 rows (all visible)
```

**Key Property**: Once initialization completes, the system is fully configured. No runtime discovery, no null metrics concerns, no dynamic tile creation. All data flows through pre-configured callbacks and the shared `MetricsQueue`.

#### 3.5: Configuration at Initialization

```json
{
  "dashboard": {
    "title": "Graph Execution Engine Dashboard",
    "sections": {
      "metrics": {
        "height_percent": 40,
        "columns": "auto",
        "auto_scroll": false,
        "scrollable": true,
        "fields": []
      }
    },
    "initialization": {
      "discover_metrics": true,
      "install_callbacks": true,
      "create_tiles": true,
      "register_routing": true
    }
  }
}
```

**TODO**:
- [ ] Implement metric discovery from executor capabilities
- [ ] For each discovered metric, create and install callback function
- [ ] Verify callbacks enqueue to MetricsQueue successfully
- [ ] Create metric tile panel only for discovered metrics
- [ ] Register routing entries in MetricRoutingIndex at initialization
- [ ] Test with typical 10-15 node graphs
- [ ] Verify no tile panels created for silent nodes
- [ ] Measure startup time for metric discovery + callback installation
- [ ] Profile callback execution time (should be <1ms per metric update)
- [ ] Verify metrics flow through queue after initialization completes

---

### 4. Executor-Dashboard API Boundary

**Issue**: The interface between the dashboard and the executor must be well-defined, versioned, and resilient to changes on either side.

**Location**: The relevant classes for graph execution are located in `include/graph`:
- `GraphExecutor` - Main executor managing graph lifecycle (Init/Start/Run/Stop/Join)
- `GraphManager` - Manages graph instantiation and configuration
- `GraphExecutorBuilder` - Builder pattern for constructing configured executors
- Related classes: Node, Graph, Port, Message queues, and execution scheduling

**Design Approach**: 

The dashboard interacts with the executor primarily through:

1. **GraphExecutorBuilder** - Construction phase
   - Parse command-line parameters
   - Load graph configuration (JSON)
   - Validate topology
   - Build and return configured executor instance

2. **GraphExecutor** - Lifecycle and capabilities queries
   - `Init()` - Initialize executor and all nodes
   - `Start()` - Begin execution
   - `Run()` - Block until completion
   - `Stop()` - Signal graceful shutdown
   - `Join()` - Wait for threads to complete
   - `GetCapabilities()` - Query metrics schemas and node metadata
   - `RegisterMetricsCallback()` - Install metric callbacks at startup

3. **Callback Installation Interface**
   - Dashboard discovers metrics schemas via `GetCapabilities()`
   - Dashboard installs callbacks for each metric-producing node
   - Callbacks enqueue events to shared `MetricsQueue`
   - No polling or repeated queries needed after initialization

**API Stability Considerations**:

```cpp
// From include/graph/GraphExecutor.hpp
namespace graph {
    class GraphExecutor {
    public:
        // Lifecycle management
        void Init();
        void Start();
        void Run();
        void Stop();
        int Join();
        
        // State queries (safe for concurrent reads)
        ExecutorState GetState() const;
        ExecutionStats GetStats() const;
        
        // Capability discovery (called once at dashboard startup)
        GraphCapabilities GetCapabilities() const;
        
        // Metric callback registration (called during initialization)
        void RegisterMetricsCallback(
            const std::string& node_name,
            std::function<void(const MetricsEvent&)> callback);
        
        // API versioning for future evolution
        static constexpr int API_VERSION = 1;
        int GetAPIVersion() const;
    };
    
    class GraphCapabilities {
    public:
        // All data returned is read-only and safe for concurrent access
        std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const;
        std::vector<DataInjectionPort> GetDataInjectionPorts() const;
        GraphTopology GetGraphTopology() const;
        std::map<std::string, NodeMetadata> GetNodeMetadata() const;
    };
}
```

**Key Design Properties**:

- **Single Discovery Phase**: All `GetCapabilities()` queries happen once during dashboard `Initialize()`
- **Pre-installed Callbacks**: Metrics callbacks installed during `Initialize()`, not discovered at runtime
- **Thread-Safe Queries**: State queries (`GetState()`, `GetStats()`) safe for concurrent reads from dashboard thread
- **No Polling**: Dashboard does not repeatedly query executor; metrics flow via callbacks through `MetricsQueue`
- **Stable API**: Class locations and method signatures in `include/graph` are authoritative
- **No Versioning**: Unified application - no version compatibility required
- **Single-Threaded Init**: All initialization in `Init()` is single-threaded; no thread safety concerns during setup
- **Dynamic Callback References**: Callbacks use dynamic_cast to Node pointers; guaranteed valid until post-Join() cleanup

**Decisions Made**:

- **API Versioning**: Not required - this is a unified application
- **Error Propagation**: LOG4CXX handles most error reporting; an error stream will be added to GraphExecutor for structured error notifications
- **Callback Cleanup**: Remove callbacks after `Join()` but before GraphExecutor destruction
- **Thread Safety**: Init phase is single-threaded; no synchronization overhead needed

**TODO**:
- [ ] Review actual GraphExecutor API in include/graph/GraphExecutor.hpp
- [ ] Document callback registration mechanism from executor perspective
- [ ] Add error stream to GraphExecutor for structured error notifications (complements LOG4CXX)
- [ ] Implement callback removal in GraphExecutor post-Join() cleanup
- [ ] Document callback lifetime and Node pointer validity
- [ ] Add integration tests for callback lifecycle management

---

### 5. Error Handling and Recovery

**Issue**: The executor may encounter errors during construction or runtime. Graph construction errors are fatal; runtime errors are handled via notifications and timeouts.

**Design Approach**:

The error handling is intentionally simple and pragmatic:

1. **Graph Construction Errors**: If the graph fails to construct (invalid configuration, missing files, topology errors, etc.), the application terminates immediately. No recovery is possible at construction time.

2. **Runtime Errors**: Once the graph is executing, nodes may encounter errors, timeouts, or other issues. These are communicated via the metrics and logging infrastructure:
   - **Error notifications**: Sent as `MetricsEvent` objects with event_type="error" or "timeout"
   - **Logging**: Errors logged via log4cxx with ERROR or FATAL level
   - **Timeout handling**: Long-running operations may timeout; completion signals track this
   - **Executor state**: Execution state may transition to ERROR if critical failure occurs

3. **Dashboard Response**: The dashboard displays errors and timeouts via:
   - Logging window: Shows all ERROR/FATAL log messages
   - Status bar: Displays error count and executor state (RUNNING, ERROR, etc.)
   - Metrics tiles: Alert status turns red if error metrics exceed thresholds
   - Command window: User can query detailed error information or stop execution

**Error Propagation Channels**:

1. **LOG4CXX Logging** (Primary): All error/fatal level messages from nodes and executor
   - Captured by FTXUILog4cxxAppender
   - Displayed in Logging Window
   - Searchable and filterable by level and content

2. **Error Stream** (New): Structured error events from GraphExecutor
   - MetricsEvent with event_type="error" or "timeout"
   - Contains: source node, error code, message, timestamp
   - Routed through MetricsQueue for consistent timing
   - Allows dashboard to react to specific errors (e.g., turn tile red)

**Key Invariants**:
- Construction errors = Application failure (process exit)
- Runtime errors = Notifications via LOG4CXX + error stream (no automatic recovery)
- User action required to stop execution in error state
- Error details available in logs for post-mortem analysis
- No error recovery or automatic retry logic

**Implementation**:

```cpp
// Error notifications come through metrics system
struct ErrorNotification : public app::metrics::MetricsEvent {
    std::string source;              // Node name
    std::string error_message;       // Human-readable error description
    std::string error_code;          // Optional: Structured error code
    std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{0};  // If timeout
};

// Dashboard implements error handling via metrics subscription
class GraphExecutionDashboard : public app::metrics::IMetricsSubscriber {
private:
    void OnMetricsEvent(const app::metrics::MetricsEvent& event) override {
        // Handle error notifications via metrics events
        if (event.event_type == "error") {
            // Route to logging window (automatic)
            // Update status bar error count
            // Turn tile to red if applicable
        }
        if (event.event_type == "timeout") {
            // Similar handling for timeouts
        }
    }
    
    void UpdateExecutorState(ExecutorState new_state) {
        if (new_state == ExecutorState::Error) {
            // Status bar shows ERROR in red
            // User can view logs or stop execution
        }
    }
};

// Command window provides access to error details
class CommandWindow {
    // Commands available to user:
    // - "show_errors": Display all error events from current session
    // - "last_error": Show the most recent error details
    // - "error_count": Display total error count
    // - "stop_execution": Stop the graph (available in ERROR state)
};
```

**Logging Integration**:
```cpp
// Errors are logged via log4cxx at ERROR or FATAL level
// Log4cxx appender (FTXUILog4cxxAppender) forwards logs to LoggingWindow
// Dashboard displays all logged errors in the Logging section

LOG4CXX_ERROR(logger, "Node ProcessorA failed: timeout after 5000ms");
LOG4CXX_FATAL(logger, "Graph construction failed: invalid graph topology");
```

**TODO**:
- [ ] Add error stream to GraphExecutor API (complementary to LOG4CXX)
- [ ] Define error event structure: code, message, node_name, timestamp
- [ ] Ensure executor state transitions include ERROR state
- [ ] Configure log4cxx ERROR/FATAL level routing to LoggingWindow
- [ ] Add error count tracking to status bar
- [ ] Implement command window error query commands (show_errors, last_error, error_count)
- [ ] Add red alert styling to tiles with error metrics
- [ ] Document application exit codes for construction failures

---

### 6. Data Flow and Message Volume

**Issue**: The graph may process high-frequency data (100+ Hz, 1000+ messages/sec). The dashboard cannot display every message but must show representative summaries.

**Challenges**:
- **Message aggregation**: Summarize high-frequency data for display
- **Bottleneck risk**: Dashboard queries could slow executor
- **Memory usage**: Storing all messages is infeasible
- **Visualization**: Represent data flow without cluttering UI
- **Debugging**: Need access to raw data when troubleshooting

**Potential Solutions**:
```cpp
// Message statistics instead of individual messages
struct MessageStats {
    uint64_t total_count;
    uint64_t error_count;
    double avg_size_bytes;
    double messages_per_second;
    std::chrono::steady_clock::time_point last_message_time;
};

// Per-port statistics
class PortStatistics {
public:
    void RecordMessage(size_t size);
    MessageStats GetStats() const;
    
    // Optional: Ring buffer of last N messages for debugging
    std::vector<Message> GetLastMessages(size_t n) const;
    
private:
    uint64_t total_messages_ = 0;
    uint64_t error_count_ = 0;
    std::deque<size_t> recent_sizes_;  // Last 100 message sizes
    std::chrono::steady_clock::time_point last_message_time_;
};

// Data flow visualization (statistical, not real-time)
class DataFlowVisualization {
public:
    void Update(const GraphTopology& topology,
                const std::map<std::string, PortStatistics>& stats);
    
    ftxui::Element Render() const;
    
private:
    std::map<EdgeId, double> edge_throughput_;  // messages/sec
};
```

**TODO**:
- [ ] Design message statistics collection (lightweight)
- [ ] Add per-node and per-port message counters
- [ ] Create data flow visualization showing throughput
- [ ] Implement optional message capture for debugging
- [ ] Add configurable message sampling rate

---

### 7. Configuration and State Synchronization

**Issue**: The dashboard configuration, executor configuration, and UI state must all stay synchronized. Changes in one must be reflected in others.

**Challenges**:
- **Partial failures**: What if executor starts but config update fails?
- **Version mismatch**: User edits config, executor was built from old config
- **State divergence**: Dashboard displays stale state
- **Hot reload**: Can configuration be reloaded without stopping executor?
- **Undo/rollback**: Can configuration changes be undone?

**Potential Solutions**:
```cpp
// Configuration versioning and snapshots
class ConfigurationManager {
public:
    struct ConfigSnapshot {
        uint64_t version;
        std::chrono::system_clock::time_point timestamp;
        nlohmann::json graph_config;
        nlohmann::json dashboard_config;
    };
    
    void SaveSnapshot();
    bool ReloadConfig(const std::string& config_file);
    std::vector<ConfigSnapshot> GetSnapshots() const;
    bool RollbackToVersion(uint64_t version);
    
    uint64_t GetCurrentVersion() const;
    
private:
    std::deque<ConfigSnapshot> snapshots_;
    uint64_t current_version_ = 0;
};

// Configuration change notification
class ConfigChangeListener {
public:
    virtual void OnConfigChanged(const ConfigSnapshot& old_config,
                                 const ConfigSnapshot& new_config) = 0;
};

// Validated configuration updates
class ConfigValidator {
public:
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    ValidationResult ValidateConfiguration(const nlohmann::json& config);
};
```

**TODO**:
- [ ] Implement configuration versioning and snapshots
- [ ] Add configuration change notifications
- [ ] Design configuration validation framework
- [ ] Support hot reload with executor state preservation
- [ ] Create configuration diff/merge utilities

---

### 8. Testing and Simulation

**Issue**: Testing the dashboard-executor interaction is challenging without a real executor. A mock/simulation mode is needed for development and testing.

**Challenges**:
- **Mock executor**: Creating realistic mock behavior is complex
- **Determinism**: Tests must be reproducible
- **Coverage**: How to test all executor states and errors?
- **Performance testing**: How to simulate high-load scenarios?
- **Integration tests**: Dashboard + executor interaction

**Potential Solutions**:
```cpp
// Mock executor for testing
class MockGraphExecutor : public GraphExecutor {
public:
    enum class SimulationMode {
        Normal,           // Simulate normal execution
        HighLoad,         // High message rate
        WithErrors,       // Simulate failures
        Deterministic     // Repeatable behavior for tests
    };
    
    void SetSimulationMode(SimulationMode mode);
    void SetNodeFailurePoint(const std::string& node_name, 
                            uint64_t message_count);
    
private:
    SimulationMode mode_;
    std::map<std::string, uint64_t> failure_points_;
};

// Recorded execution playback
class ExecutionRecording {
public:
    void Record(const GraphExecutor& executor, 
                std::chrono::seconds duration);
    void Playback(GraphExecutor& executor);
    void Export(const std::string& filename);
    
private:
    std::vector<MetricsSnapshot> snapshots_;
    std::vector<ExecutionError> errors_;
    std::vector<ExecutorStateChange> state_changes_;
};

// Test fixtures
struct DashboardTestFixture {
    std::unique_ptr<MockGraphExecutor> executor;
    std::unique_ptr<GraphExecutionDashboard> dashboard;
    
    void SetUp();
    void TearDown();
};
```

**TODO**:
- [ ] Implement mock graph executor with configurable behavior
- [ ] Create test scenarios (normal, high-load, error conditions)
- [ ] Add execution recording and playback
- [ ] Write integration tests for dashboard-executor interaction
- [ ] Add performance benchmarks

---

### 9. Logging and Diagnostics

**Issue**: Log messages from the executor (potentially from multiple threads) must be captured, aggregated, and displayed in the dashboard without losing information or causing performance degradation.

**Challenges**:
- **Thread safety**: Multiple nodes logging concurrently
- **Log volume**: 1000+ messages/sec during high-load execution
- **Storage**: Unbounded log growth
- **Filtering**: Finding relevant logs in large volume
- **Timestamps**: Correlating logs with metrics and events

**Potential Solutions**:
```cpp
// Lock-free log aggregation
class LogAggregator {
public:
    void LogMessage(const LogEntry& entry);
    std::vector<LogEntry> GetLogs(LogLevel min_level = LogLevel::Debug) const;
    std::vector<LogEntry> Search(const std::string& pattern) const;
    
    void SetMaxLogs(size_t max) { max_logs_ = max; }
    
private:
    struct LogSnapshot {
        std::vector<LogEntry> entries;
    };
    
    std::atomic<LogSnapshot*> current_;
    LockFreeRingBuffer<LogEntry> buffer_;
    size_t max_logs_ = 10000;
};

// Structured logging
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string logger_name;
    std::string node_name;  // Which node generated this
    std::string message;
    std::vector<std::pair<std::string, std::string>> context;  // Key-value pairs
};

// Log filtering and search
class LogFilter {
public:
    void SetLevelFilter(LogLevel min_level);
    void SetNodeFilter(const std::vector<std::string>& nodes);
    void SetSearchPattern(const std::string& regex);
    
    std::vector<LogEntry> Apply(const std::vector<LogEntry>& logs) const;
    
private:
    LogLevel level_filter_ = LogLevel::Debug;
    std::vector<std::string> node_filter_;
    std::regex search_pattern_;
};
```

**TODO**:
- [ ] Design lock-free log aggregation
- [ ] Implement structured logging with context
- [ ] Add log filtering and search
- [ ] Export logs to files (for post-mortem analysis)
- [ ] Correlate logs with metrics timestamps

---

### 10. Resource Management and Cleanup

**Issue**: The dashboard and executor allocate resources (threads, memory, file handles, callbacks). Proper cleanup is critical to avoid leaks and ensure graceful shutdown.

**Challenges**:
- **Circular dependencies**: Dashboard callbacks reference executor, executor callbacks reference dashboard
- **Exception safety**: Resource cleanup must happen even on errors
- **Timing**: When is it safe to destroy objects?
- **Thread cleanup**: All threads must be joined before destruction
- **File handles**: CSV files, output files must be closed properly

**Potential Solutions**:
```cpp
// RAII resource management
class DashboardSession {
public:
    DashboardSession(const std::string& config_file);
    ~DashboardSession();  // Guarantees cleanup
    
    // Explicit lifecycle
    void Init();
    void Run();  // Blocks until user quits
    void Shutdown();
    
private:
    std::unique_ptr<GraphExecutor> executor_;
    std::unique_ptr<GraphExecutionDashboard> dashboard_;
    std::vector<std::unique_ptr<IMetricsCallback>> callbacks_;
    
    void CleanupCallbacks();
    void WaitForThreads();
};

// Safe callback registration with weak references
class SafeCallbackHandle {
public:
    template<typename T>
    static SafeCallbackHandle RegisterCallback(
        T* object,
        void (T::*method)(const MetricsEvent&),
        GraphExecutor& executor) {
        
        // Weak reference to object
        std::weak_ptr<T> weak = object->shared_from_this();
        
        auto callback = [weak](const MetricsEvent& event) {
            if (auto strong = weak.lock()) {
                (strong.get()->*method)(event);
            }
            // If object destroyed, callback safely does nothing
        };
        
        return SafeCallbackHandle(executor.RegisterCallback(callback));
    }
};

// Resource allocation summary
class ResourceMonitor {
public:
    struct ResourceStats {
        size_t thread_count;
        size_t memory_allocated_mb;
        size_t file_handle_count;
        size_t message_queue_depth;
    };
    
    ResourceStats GetStats() const;
    
private:
    std::atomic<size_t> thread_count_{0};
    std::atomic<size_t> memory_allocated_{0};
    std::atomic<size_t> file_handles_{0};
};
```

**TODO**:
- [ ] Design RAII wrappers for all resources
- [ ] Use weak_ptr for callbacks to prevent circular deps
- [ ] Add resource cleanup checklist in destructors
- [ ] Write resource leak detection tests
- [ ] Add resource statistics to status bar

---

## Summary of TODOs by Priority

### High Priority (Blocking)
- [ ] Thread-safe metrics sharing (RCU or atomics)
- [ ] Non-blocking command queue
- [ ] Stable API boundary with versioning
- [ ] RAII resource management
- [ ] Error handling framework
- [ ] Mock executor for testing

### Medium Priority (Important)
- [ ] Async metrics polling
- [ ] Metric filtering and search
- [ ] Configuration versioning
- [ ] Lock-free log aggregation
- [ ] Virtual scrolling for large graphs
- [ ] Safe callback lifecycle management

### Low Priority (Enhancement)
- [ ] Hot configuration reload
- [ ] Execution recording/playback
- [ ] Advanced data flow visualization
- [ ] Message sampling and capture
- [ ] Performance profiling tools
- [ ] Resource monitoring dashboard

---

## Conclusion

This architecture provides a flexible foundation for building sophisticated terminal dashboards with FTXUI. The constraint-based layout system and modular window framework enable:

- Rapid dashboard prototyping
- Easy composition of complex UIs
- Clean separation of concerns
- Extensibility for custom widgets
- Maintainable codebase

Follow the patterns and guidelines outlined in this document to build scalable, professional-grade terminal applications.
