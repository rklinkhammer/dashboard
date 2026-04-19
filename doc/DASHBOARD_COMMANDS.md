# Dashboard Command System Analysis

## Overview

This document analyzes the integration of Phase 3b, 3c, and 3d features (filtering, collapse/expand, pinning, sorting, export) into the existing command window processing system.

**Current Architecture:**
- Command-driven CLI interface via CommandWindow
- CommandRegistry for extensible command registration
- BuiltinCommands for default command implementations
- Event loop in Dashboard::Run() that routes events
- MetricsPanel with MetricsTilePanel for metrics display

**Key Finding:** The command system is already well-architected for feature integration. Phase 3 features can be cleanly integrated as new command handlers without disrupting existing functionality.

---

## Current Command Processing Flow

### 1. Event Loop (Dashboard::Run)

```
Terminal Input Event
        ↓
Dashboard::CatchEvent() [Dashboard.cpp:160-220]
        ├─ Special keys: 'q' (quit), F1 (debug toggle)
        ├─ Arrow keys: Left/Right for tab switching
        └─ All other input → CommandWindow::HandleKeyInput()
        ↓
CommandWindow::HandleKeyInput() [CommandWindow.cpp]
        ├─ Character input: Add to input_buffer_
        ├─ Return: Execute command
        ├─ Backspace: Delete from buffer
        └─ Escape: Cancel
        ↓
CommandWindow::ParseAndExecuteCommand()
        ├─ Parse input_buffer_ into command name + args
        ├─ Retrieve CommandInfo from registry
        └─ Call handler function
        ↓
CommandResult returned to CommandWindow
        └─ Display in output_lines_ (max 100 lines)
```

### 2. CommandRegistry (Extensible Pattern)

**Current Status:** 5 built-in commands registered
- `status` - Display dashboard state
- `run` - Execute graph
- `pause` - Pause graph
- `stop` - Stop graph
- `reset_layout` - Reset window layout

**Registration Pattern:**
```cpp
registry->RegisterCommand(
    "command_name",
    "Description",
    "Usage: command_name [args]",
    [app](const std::vector<std::string>& args) {
        // Handler implementation
        return CommandResult(success, message);
    }
);
```

### 3. Access to Application Context

All command handlers receive:
- `Dashboard* app` - Full app context
- Access to: MetricsPanel, MetricsTilePanel, LoggingWindow, CommandWindow, StatusBar

---

## Phase 3 Features: Integration Points

### Phase 3b: Filtering & Search

**Current Implementation in MetricsTilePanel:**
```cpp
void SetGlobalSearchFilter(const std::string& filter);
const std::string& GetGlobalSearchFilter() const;
void SetNodeFilter(const std::string& node_name, const std::string& filter);
void ClearAllFilters();
void ClearNodeFilter(const std::string& node_name);
bool MatchesFilters(const std::string& node_name, const std::string& metric_name) const;
```

**Proposed Commands:**
```
filter.global <pattern>       - Set global search filter
filter.clear                  - Clear all filters
filter.node <node> <pattern>  - Set filter for specific node
filter.help                   - Show filter help
```

**Command Handler Implementation Pattern:**
```cpp
// In BuiltinCommands.cpp
CommandResult cmd::CmdFilterGlobal(Dashboard* app, const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult(false, "Usage: filter.global <pattern>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    
    std::string pattern = args[0];
    tile_panel->SetGlobalSearchFilter(pattern);
    
    return CommandResult(true, "Global filter set to: " + pattern);
}
```

---

### Phase 3c: Collapse/Expand

**Current Implementation in MetricsTilePanel:**
```cpp
void SetNodeCollapsed(const std::string& node_name, bool collapsed);
bool IsNodeCollapsed(const std::string& node_name) const;
void ToggleNodeCollapsed(const std::string& node_name);
void ExpandAll();
void CollapseAll();
std::vector<std::string> GetNodeNames() const;
```

**Proposed Commands:**
```
collapse.toggle <node>        - Toggle collapse state of node
collapse.set <node> <on|off>  - Set collapse state
collapse.all                  - Collapse all nodes
collapse.expand_all           - Expand all nodes
collapse.status               - Show collapse state of all nodes
```

**Command Handler Implementation Pattern:**
```cpp
CommandResult cmd::CmdCollapseToggle(Dashboard* app, const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult(false, "Usage: collapse.toggle <node_name>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    
    std::string node_name = args[0];
    tile_panel->ToggleNodeCollapsed(node_name);
    
    bool is_collapsed = tile_panel->IsNodeCollapsed(node_name);
    std::string state = is_collapsed ? "collapsed" : "expanded";
    
    return CommandResult(true, "Node '" + node_name + "' is now " + state);
}
```

---

### Phase 3d: Pinning, Sorting, and Export

**Current Implementation in MetricsTilePanel:**

*Pinning:*
```cpp
void TogglePinnedMetric(const std::string& node_name, const std::string& metric_name);
bool IsPinnedMetric(const std::string& node_name, const std::string& metric_name) const;
void ClearAllPinned();
```

*Sorting:*
```cpp
void SetSortingMode(const std::string& mode);  // "name", "value", "status", "pinned"
const std::string& GetSortingMode() const;
```

*Export:*
```cpp
std::string ExportToJSON() const;
std::string ExportToCSV() const;
```

**Proposed Commands:**
```
pin.toggle <node> <metric>    - Toggle pin on metric
pin.clear                     - Clear all pins
pin.list                      - List pinned metrics

sort.by <mode>                - Set sort mode (name|value|status|pinned)
sort.status                   - Show current sort mode

export.json [file]            - Export to JSON file (or stdout if no file)
export.csv [file]             - Export to CSV file (or stdout if no file)
```

**Command Handler Implementation Pattern:**
```cpp
// Pinning example
CommandResult cmd::CmdPinToggle(Dashboard* app, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CommandResult(false, "Usage: pin.toggle <node> <metric>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    
    std::string node_name = args[0];
    std::string metric_name = args[1];
    
    tile_panel->TogglePinnedMetric(node_name, metric_name);
    
    bool is_pinned = tile_panel->IsPinnedMetric(node_name, metric_name);
    std::string state = is_pinned ? "pinned" : "unpinned";
    
    return CommandResult(true, node_name + "::" + metric_name + " is now " + state);
}

// Export example
CommandResult cmd::CmdExportJSON(Dashboard* app, const std::vector<std::string>& args) {
    auto metrics_panel = app->GetMetricsPanel();
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    
    std::string json_data = tile_panel->ExportToJSON();
    
    if (args.empty()) {
        // Print to command window
        return CommandResult(true, json_data);
    } else {
        // Write to file
        std::string filename = args[0];
        std::ofstream file(filename);
        if (!file.is_open()) {
            return CommandResult(false, "Failed to open file: " + filename);
        }
        file << json_data;
        file.close();
        return CommandResult(true, "Exported to " + filename);
    }
}
```

---

## Proposed Command Set

### Complete Command Namespace Structure

```
FILTERING COMMANDS:
├─ filter.global <pattern>      Set global search filter
├─ filter.node <node> <pattern> Set per-node filter
├─ filter.clear                 Clear all filters
└─ filter.help                  Show filter help

COLLAPSE COMMANDS:
├─ collapse.toggle <node>       Toggle collapse state
├─ collapse.set <node> <on|off> Set collapse state
├─ collapse.all                 Collapse all nodes
├─ collapse.expand_all          Expand all nodes
└─ collapse.status              Show collapse status

PIN COMMANDS:
├─ pin.toggle <node> <metric>   Toggle pin on metric
├─ pin.clear                    Clear all pins
└─ pin.list                     List pinned metrics

SORT COMMANDS:
├─ sort.by <mode>               Set sort mode
└─ sort.status                  Show current sort mode

EXPORT COMMANDS:
├─ export.json [file]           Export metrics to JSON
├─ export.csv [file]            Export metrics to CSV
└─ export.list                  List available export formats

SYSTEM COMMANDS (existing):
├─ status                       Show dashboard status
├─ run                          Run computation graph
├─ pause                        Pause graph
├─ stop                         Stop graph
└─ reset_layout                 Reset layout to defaults
```

---

## Implementation Architecture

### File Organization

```
include/ui/
├─ CommandRegistry.hpp          (exists) - Extensible registry
├─ CommandWindow.hpp            (exists) - CLI input/output
├─ MetricsTilePanel.hpp         (exists) - Phase 3 feature storage
└─ MetricsPanel.hpp             (exists) - Top-level metrics container

src/ui/
├─ CommandRegistry.cpp          (exists)
├─ CommandWindow.cpp            (exists)
├─ MetricsTilePanel.cpp         (exists) - Phase 3 implementations
├─ MetricsPanel.cpp             (exists)
├─ BuiltinCommands.cpp          (UPDATE) - Add Phase 3 command handlers
├─ Dashboard.cpp                (UPDATE) - Add focus state management
└─ [OPTIONAL] MetricsCommands.cpp (NEW) - Separate Phase 3 commands

test/ui/
├─ test_command_registry.cpp    (exists) - 16 tests
├─ test_metrics_tile_panel.cpp  (exists) - 46 tests with Phase 3 features
└─ test_metrics_commands.cpp    (NEW) - 15+ tests for new commands
```

### Implementation Phases

#### Phase 3d-1: Command Handlers (Week 1)

**Files to Update:**
- `src/ui/BuiltinCommands.cpp` - Add 15+ new command handlers
- `include/ui/BuiltinCommands.hpp` - Add handler signatures

**Commands to Add:**
- Filtering: 3 commands (global, node, clear)
- Collapse: 5 commands (toggle, set, all, expand_all, status)
- Pinning: 3 commands (toggle, clear, list)
- Sorting: 2 commands (set, status)
- Export: 3 commands (json, csv, list)

**Test Coverage:** 16 new tests
```cpp
// test_metrics_commands.cpp
TEST(MetricsCommandsTest, FilterGlobalCommand_SetsFilter)
TEST(MetricsCommandsTest, FilterGlobalCommand_InvalidArgs)
TEST(MetricsCommandsTest, CollapseToggleCommand_TogglesNode)
TEST(MetricsCommandsTest, PinToggleCommand_PinsMetric)
TEST(MetricsCommandsTest, ExportJsonCommand_WritesFile)
// ... etc
```

**Estimated Code:** 200-300 lines (handler implementations)

---

#### Phase 3d-2: Focus State Management (Week 1-2)

**Files to Update:**
- `include/ui/MetricsTilePanel.hpp` - Add focus tracking
- `src/ui/MetricsTilePanel.cpp` - Implement focus methods
- `include/ui/Dashboard.hpp` - Add focus state delegates
- `src/ui/Dashboard.cpp` - Track focused node/metric

**Methods to Add:**
```cpp
// Focus tracking in MetricsTilePanel
void SetFocusedNode(const std::string& node_name);
std::string GetFocusedNode() const;
void MoveFocusNextNode();
void MoveFocusPreviousNode();
void MoveFocusToFirstNode();
```

**Test Coverage:** 8 new tests

**Estimated Code:** 100-150 lines

---

#### Phase 3d-3: Keyboard Shortcut Aliases (Week 2)

**Enhancement to CommandWindow:**
- Allow single-character aliases for common commands
- Pattern: Ctrl+X to map to a command

**Example Implementation:**
```cpp
// In Dashboard::Run() event handling
if (event.ctrl && event.character() == "f") {
    // Send "filter.global" command to CommandWindow
    command_window_->HandleDirectCommand("filter.global");
}
```

**Keyboard Shortcuts:**
```
Ctrl+F  → filter.global (opens input)
Ctrl+O  → collapse.all
Ctrl+E  → collapse.expand_all
Ctrl+P  → pin.toggle (on focused metric)
Ctrl+S  → export.csv
Ctrl+1  → sort.by name
Ctrl+2  → sort.by value
Space   → collapse.toggle (on focused node)
```

**Test Coverage:** 10 new tests

**Estimated Code:** 50-80 lines in Dashboard.cpp

---

#### Phase 3d-4: Interactive Input Dialogs (Optional, Week 3)

**For more advanced UX, add modal input for:**
- Filter input dialog (shows live match count)
- Export filename dialog (with format selection)
- Keyboard shortcut help dialog

**This would require:**
- Modal dialog system in Dashboard
- Input validation
- Preview of results

**Not essential for MVP** - command-line interface sufficient initially.

---

## Integration with MetricsTilePanel

### Data Flow Through Command System

```
User Input: "filter.global throughput"
        ↓
CommandWindow::ParseAndExecuteCommand()
        ├─ Parse: name="filter.global", args=["throughput"]
        ├─ Lookup: registry->ExecuteCommand("filter.global", ["throughput"])
        └─ Call handler: cmd::CmdFilterGlobal(app, ["throughput"])
        ↓
CmdFilterGlobal Handler:
        ├─ Get MetricsPanel from Dashboard
        ├─ Get MetricsTilePanel from MetricsPanel
        ├─ Call: tile_panel->SetGlobalSearchFilter("throughput")
        └─ Return success message
        ↓
MetricsTilePanel:
        ├─ Store filter in global_search_filter_
        ├─ Mark panel for re-render
        └─ Next Render() call applies filter
        ↓
Dashboard Render Loop:
        ├─ Calls metrics_panel_->Render()
        ├─ MetricsPanel calls metrics_tile_panel_->Render()
        ├─ MetricsTilePanel applies filters in render
        └─ Terminal displays filtered view
        ↓
CommandWindow:
        ├─ Display result message
        └─ Ready for next command
```

### Separation of Concerns

**MetricsTilePanel (Data Layer):**
- Stores filter/collapse/pin/sort state
- Provides accessor methods
- No UI rendering of state

**MetricsPanel (View Layer):**
- Calls MetricsTilePanel to get state
- Passes state to render system
- Displays filtered/collapsed/pinned metrics

**CommandWindow (CLI Layer):**
- Parses commands
- Calls handlers
- Displays results

**Dashboard (Orchestration Layer):**
- Routes events to CommandWindow
- Triggers periodic renders
- Manages focus state

---

## Benefits of Command-Driven Architecture

### 1. Testability
- Commands are pure functions with simple inputs/outputs
- No UI dependencies required to test commands
- Can test all Phase 3 features via command execution

### 2. Extensibility
- New commands can be added without modifying core systems
- Third-party code can register custom commands
- Easy to add macros or command aliases later

### 3. Scriptability
- Commands can be put in a file and executed as a script
- Enable batch operations: "collapse.all; filter.global error; export.csv errors.csv"
- Reproducible workflows

### 4. Discoverability
- All commands listed via CommandRegistry::GetAllCommands()
- Help text auto-generated
- Users can discover features via "help" command

### 5. Auditability
- All user actions are commands (already logged in CommandWindow output)
- Can replay user session by replaying commands
- Compliance and debugging benefits

---

## Design Patterns

### 1. Command Result Pattern
All commands return `CommandResult(success, message)` for consistency:
```cpp
// Success case
return CommandResult(true, "Filter applied: 5 metrics match");

// Error case
return CommandResult(false, "Invalid node name: 'Node99'");

// Complex result with data
std::ostringstream oss;
oss << "Pinned metrics:\n";
for (const auto& item : pinned_items) {
    oss << "  - " << item << "\n";
}
return CommandResult(true, oss.str());
```

### 2. Argument Validation Pattern
```cpp
// Check argument count
if (args.size() < required_count) {
    return CommandResult(false, "Usage: " + usage_string);
}

// Validate argument content
std::string mode = args[0];
if (mode != "name" && mode != "value" && mode != "status" && mode != "pinned") {
    return CommandResult(false, "Invalid sort mode: " + mode);
}
```

### 3. State Query Pattern
```cpp
// Before making changes, show current state
std::string current = tile_panel->GetGlobalSearchFilter();
if (current == pattern) {
    return CommandResult(false, "Filter already set to: " + pattern);
}

// After changes, confirm new state
tile_panel->SetGlobalSearchFilter(pattern);
std::string updated = tile_panel->GetGlobalSearchFilter();
return CommandResult(true, "Filter changed: '" + current + "' → '" + updated + "'");
```

---

## Risks and Mitigations

### Risk 1: Command Window Input Blocker
**Issue:** If user types in command window, it captures all input
**Mitigation:** Already handled by Dashboard event routing - only CommandWindow events are keyboard

### Risk 2: Conflicting Focus States
**Issue:** Multiple components tracking focus (CommandWindow, MetricsTilePanel)
**Mitigation:** Clear focus ownership - CommandWindow has input focus, MetricsTilePanel has selection focus

### Risk 3: Export File Permissions
**Issue:** Writing to arbitrary file paths
**Mitigation:** Validate filenames, use safe directory, provide default location

### Risk 4: Large Export Output
**Issue:** Exporting millions of metrics could freeze UI
**Mitigation:** StreamingExport pattern - write incrementally, show progress

---

## Success Criteria

### Functional Requirements
✅ All 15+ Phase 3 commands registered and callable
✅ Commands properly update MetricsTilePanel state
✅ Updates reflected in next render cycle
✅ Error cases handled with descriptive messages

### Code Quality
✅ New commands follow existing patterns
✅ No duplicate code (DRY principle)
✅ All functions documented with purpose/usage

### Testing
✅ 16+ new tests for command handlers
✅ Tests cover success and error cases
✅ All new tests pass
✅ Existing tests still pass (no regressions)

### Integration
✅ No breaking changes to existing APIs
✅ Clean separation of concerns
✅ Easy to add more commands in future

---

## Implementation Checklist

### Phase 3d-1: Command Handlers
- [ ] Create cmd namespace in BuiltinCommands
- [ ] Implement 15+ command handler functions
- [ ] Register commands in RegisterBuiltinCommands()
- [ ] Add unit tests (16+ tests)
- [ ] Verify all tests pass
- [ ] Build clean with zero warnings

### Phase 3d-2: Focus State
- [ ] Add focus tracking to MetricsTilePanel
- [ ] Implement focus navigation methods
- [ ] Wire focus to Dashboard context
- [ ] Add focus tests (8 tests)
- [ ] Verify focus-aware commands work correctly

### Phase 3d-3: Keyboard Shortcuts
- [ ] Map Ctrl/Alt key combinations in Dashboard
- [ ] Add alias support to CommandWindow
- [ ] Route keyboard shortcuts to commands
- [ ] Add keyboard shortcut tests (10 tests)
- [ ] Document keyboard shortcuts

### Phase 3d-4: Polish & Documentation
- [ ] Update help text with Phase 3 commands
- [ ] Add keyboard shortcut help
- [ ] Create command reference documentation
- [ ] Add examples for each command category

---

## Example Command Workflows

### Workflow 1: Debug High-Error Node
```
> filter.global error              # Show only error-related metrics
> collapse.all                     # Collapse all nodes for overview
> collapse.toggle Node5            # Expand the problem node
> pin.toggle Node5 error_count     # Pin the key metric
> sort.by pinned                   # Move pinned to top
> export.csv debug_report.csv      # Export for analysis
```

### Workflow 2: Performance Analysis
```
> filter.global latency            # Focus on latency metrics
> sort.by value                    # Show highest latencies first
> export.json perf_snapshot.json   # Save baseline
# (make changes)
> export.json perf_after.json      # Compare with tool
```

### Workflow 3: Production Monitoring
```
> collapse.all                     # Get overview
> filter.global WARNING            # Alert monitoring
> pin.toggle Node1 cpu_usage       # Track critical node
> pin.toggle Node3 memory_usage    # Track memory pressure
> sort.by status                   # Group by status
```

---

## Appendix A: Command Quick Reference

| Category | Command | Purpose |
|----------|---------|---------|
| **Filter** | `filter.global <pattern>` | Search all metrics by name |
| | `filter.node <node> <pattern>` | Filter specific node |
| | `filter.clear` | Remove all filters |
| **Collapse** | `collapse.toggle <node>` | Toggle node visibility |
| | `collapse.all` | Hide all node details |
| | `collapse.expand_all` | Show all node details |
| | `collapse.status` | List collapse state |
| **Pin** | `pin.toggle <node> <metric>` | Mark important metric |
| | `pin.clear` | Remove all pins |
| | `pin.list` | Show pinned metrics |
| **Sort** | `sort.by <mode>` | Reorder metrics |
| | `sort.status` | Show sort mode |
| **Export** | `export.json [file]` | JSON snapshot |
| | `export.csv [file]` | CSV for spreadsheet |
| | `export.list` | Show export formats |

---

## Appendix B: Implementation Example (Filter Command)

```cpp
// In BuiltinCommands.hpp
namespace cmd {
    CommandResult CmdFilterGlobal(Dashboard* app, const std::vector<std::string>& args);
    CommandResult CmdFilterNode(Dashboard* app, const std::vector<std::string>& args);
    CommandResult CmdFilterClear(Dashboard* app, const std::vector<std::string>& args);
}

// In BuiltinCommands.cpp
CommandResult cmd::CmdFilterGlobal(Dashboard* app, const std::vector<std::string>& args) {
    if (!app || args.empty()) {
        return CommandResult(false, "Usage: filter.global <pattern>");
    }
    
    auto metrics_panel = app->GetMetricsPanel();
    if (!metrics_panel) {
        return CommandResult(false, "Metrics panel not available");
    }
    
    auto tile_panel = metrics_panel->GetMetricsTilePanel();
    if (!tile_panel) {
        return CommandResult(false, "Metrics tile panel not available");
    }
    
    std::string pattern = args[0];
    std::string old_filter = tile_panel->GetGlobalSearchFilter();
    
    tile_panel->SetGlobalSearchFilter(pattern);
    
    std::ostringstream oss;
    oss << "Global filter applied: '" << pattern << "'";
    if (!old_filter.empty()) {
        oss << " (was: '" << old_filter << "')";
    }
    
    return CommandResult(true, oss.str());
}

// In RegisterBuiltinCommands():
registry->RegisterCommand(
    "filter.global",
    "Set global search filter for all metrics",
    "filter.global <pattern>",
    [app](const std::vector<std::string>& args) {
        return cmd::CmdFilterGlobal(app, args);
    }
);
```

---

## Conclusion

The existing command architecture provides an excellent foundation for Phase 3b, 3c, and 3d feature integration. By treating each feature as a set of CLI commands registered with the CommandRegistry, we can:

1. **Cleanly integrate** new features without modifying core systems
2. **Thoroughly test** features as independent command units
3. **Provide flexible UX** with both CLI and keyboard shortcuts
4. **Enable scripting** for advanced users and automation
5. **Maintain code quality** with clear separation of concerns

The implementation can proceed incrementally, with Phase 3d-1 (command handlers) ready in 1-2 days, followed by optional keyboard shortcuts and modal dialogs.
