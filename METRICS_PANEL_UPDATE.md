# Metrics Panel Update Analysis: Single-Tile Display Mode

**Document**: METRICS_PANEL_UPDATE.md  
**Date**: January 26, 2026  
**Status**: Analysis - Not Yet Implemented  
**Objective**: Change MetricsTilePanel to display all fields from a NodeMetricsSchema in a single consolidated tile per node, rather than creating individual tiles for each field.

---

## 1. Current Behavior

### 1.1 Current Architecture

The current `MetricsTilePanel` creates **one tile per metric field** across all nodes:

```
MetricsPanel
├── MetricsTilePanel (container)
│   └── Nodes (organized by node_name)
│       ├── Node: "EstimationPipelineNode"
│       │   ├── Tile: "estimation_quality" (MetricsTileWindow)
│       │   ├── Tile: "fusion_cycles" (MetricsTileWindow)
│       │   └── Tile: "estimation_latency_ms" (MetricsTileWindow)
│       └── Node: "SensorNode"
│           ├── Tile: "sample_rate_hz" (MetricsTileWindow)
│           └── Tile: "buffer_overflow_count" (MetricsTileWindow)
```

### 1.2 Current Tile Structure

**File**: `include/ui/MetricsTileWindow.hpp`

Each tile is independent with its own:
- `MetricDescriptor` (node_name, metric_name, metric_id, min/max, unit)
- `current_value_` (latest metric value)
- `history_` (deque of 60 previous values for sparklines)
- Display type (value, gauge, sparkline, state)
- Alert status computation (OK, WARNING, CRITICAL)
- Individual `Render()` that outputs a bordered box

**Current Tile Render Output** (~4-5 lines height):
```
┌────────────────────┐
│ estimation_quality │
│ Value: 92.5%       │ ← gauge or value display
│ Status: OK (🟢)    │
└────────────────────┘
```

### 1.3 Current Discovery Process

**File**: `src/ui/MetricsPanel.cpp` (lines 80-130)

```cpp
for (const auto& schema : schemas) {
    // schema = NodeMetricsSchema with node_name and metrics_schema (JSON)
    
    for (const auto& field : schema.metrics_schema["fields"]) {
        // Each field becomes a separate MetricDescriptor
        std::string metric_name = field["name"];
        std::string metric_id = schema.node_name + "::" + metric_name;
        
        // Create ONE MetricsTileWindow per field
        auto tile = std::make_shared<MetricsTileWindow>(descriptor, field);
        metrics_tile_panel_->AddTile(tile);  // ← Creates individual tile
    }
}
```

**Result**: If a node has 3 fields, 3 separate tiles are created and added to the panel.

### 1.4 Current Storage and Retrieval

**File**: `include/ui/MetricsTilePanel.hpp`

```cpp
struct Node {
    std::string name;
    // metric_name -> tile (e.g., "validation_errors" -> MetricsTileWindow)
    std::map<std::string, std::shared_ptr<MetricsTileWindow>> metrics;
};

std::vector<Node> nodes_;  // Each node contains map of metric_name -> MetricsTileWindow
```

---

## 2. Critical Rendering Problem & Desired Behavior

### 2.0 CRITICAL ISSUE: Current Rendering is Broken

The current `MetricsTilePanel::Render()` has a fundamental flaw:

```cpp
// Current broken render logic
return vbox(std::move(node_sections)) | border | color(Color::Green);
// ↑ This vbox tries to fit ALL nodes with ALL metrics into the allocated height
// ↑ Uses size(HEIGHT, EQUAL, metrics_height) to constrain the entire panel
// ↑ Result: Everything gets squashed into 15-20 lines of height, unreadable
```

**Current symptoms**:
- All tiles compressed vertically to fit the window height
- Individual tiles are 1-2 lines tall (headers barely visible)
- Field content completely hidden or illegible
- No scrolling mechanism
- No overflow handling
- No pagination or tab switching

**Example of what you see**:
```
┌────────────────────────────────────────────────────────────┐
│ [EstimationPipelineNode] (3 metrics) [SensorNode] (2 metrics) │
│ estimation_quality: 92.5% fusion_cycles: 1,234  sample_rate...│
│ [AnotherNode] (4 metrics) [YetAnother] (5 metrics) latency: 45│
│ ← All squeezed, overlapping, unreadable                       │
└────────────────────────────────────────────────────────────┘
```

**Root cause**: FTXUI's `size(HEIGHT, EQUAL, N)` modifier forces all content into N lines regardless of how much content there is. The `vbox` tries to fit everything, but with constrained height, tiles get truncated.

### 2.1 Target Architecture with Proper Rendering

The consolidation to one-tile-per-node ALSO needs to fix the rendering issue. There are three viable approaches:

#### Option A: Scrollable Single View (RECOMMENDED)
```
┌─────────────────────────────────────┐
│ Metrics (6 nodes visible, 2 off-screen)│
├─────────────────────────────────────┤
│┌─ EstimationPipelineNode (3 metrics)┐│ ← Fully visible, readable
│├────────────────────────────────────┤│
││ estimation_quality:      92.5%      ││
││ fusion_cycles:           1,234      ││
││ estimation_latency_ms:   45.2 ms    ││
││ Status: OK (🟢)                     ││
│└────────────────────────────────────┘│
│┌─ SensorNode (2 metrics)             ┐│ ← Next node fully visible
│├────────────────────────────────────┤│
││ sample_rate_hz:          100.0      ││
││ buffer_overflow_count:   0          ││
││ Status: OK (🟢)                     ││
│└────────────────────────────────────┘│
│┌─ [Scroll down for 2 more nodes] ↓  ┐│ ← Visual indication of more
│└────────────────────────────────────┘│
└─────────────────────────────────────┘
```

**Implementation**: Use FTXUI's `Container::Vertical()` with scroll support

#### Option B: Tab-Based Navigation
```
[Node: EstimationPipelineNode ◄► Next]  ← Tab headers for quick switching
┌─────────────────────────────────────┐
│┌─ EstimationPipelineNode (3 metrics)┐│
│├────────────────────────────────────┤│
││ estimation_quality:      92.5%      ││
││ fusion_cycles:           1,234      ││
││ estimation_latency_ms:   45.2 ms    ││
││ Status: OK (🟢)                     ││
│└────────────────────────────────────┘│
│                                      │
│ [Click/arrow key to switch nodes]   │
└─────────────────────────────────────┘
```

**Implementation**: Use TabContainer with one node per tab

#### Option C: Grid with Overflow Hidden (NOT RECOMMENDED)
- Same as current broken design but with proper truncation
- Content is still hidden, just more gracefully

**Recommendation**: Use **Option A (Scrollable)** for MVP:
- Shows multiple nodes at once (gives context)
- Scrolling is familiar interaction
- Easier to see relationships between nodes
- Doesn't require tab management

### 2.2 Desired Behavior with Scrollable Rendering

Change to **one consolidated tile per node** with proper scrolling:

```
MetricsPanel (with scrolling)
├── MetricsTilePanel (scrollable container)
│   └── Nodes (displayed with scroll mechanism)
│       ├── Node: "EstimationPipelineNode" (single consolidated tile, ~7 lines)
│       │   ├── Header: EstimationPipelineNode (3 metrics)
│       │   ├── estimation_quality: 92.5% [gauge]
│       │   ├── fusion_cycles: 1,234
│       │   ├── estimation_latency_ms: 45.2 ms [sparkline]
│       │   └── Status: OK (🟢)
│       │
│       └── Node: "SensorNode" (single consolidated tile, ~5 lines)
│           ├── Header: SensorNode (2 metrics)
│           ├── sample_rate_hz: 100.0
│           ├── buffer_overflow_count: 0
│           └── Status: OK (🟢)
│
└── [Scroll indicator: 2 more nodes below]
```

**Benefits over current broken design**:
- **Readable**: Each tile gets full height, content is visible
- **Scrollable**: Users can see all nodes by scrolling
- **No clipping**: Fields render to full height
- **Proper overflow**: Graceful handling of many nodes
- **Space efficient**: Consolidated tiles use less space than 3-column grid

### 2.3 Tile Rendering Details

Each consolidated node tile now renders properly:

```
┌──────────────────────────────────┐
│ EstimationPipelineNode (3 metrics)│  ← Header (1 line)
├──────────────────────────────────┤  ← Separator (1 line)
│ estimation_quality:      92.5%    │  ← Field 1 (1 line)
│ fusion_cycles:           1,234    │  ← Field 2 (1 line)
│ estimation_latency_ms:   45.2 ms  │  ← Field 3 (1 line)
├──────────────────────────────────┤  ← Separator (1 line)
│ Status: OK (🟢)                   │  ← Status line (1 line)
└──────────────────────────────────┘
Total: 7 lines, fully readable
```

### 2.4 Scrolling Implementation

```cpp
// Pseudocode for scrolling container
auto nodes_container = Container::Vertical({
    node_tile_1->Render(),
    node_tile_2->Render(),
    node_tile_3->Render(),
    // ... more nodes
});

// Wrap in scrollable container
auto scrollable_view = nodes_container | yframe | border;

// Apply height constraint WITHOUT forcing content to fit
// Height is MINIMUM available, not MAXIMUM squeeze
return scrollable_view | size(HEIGHT, AT_LEAST, 15);
```

---

## 3. Implementation Changes Required

### 3.1 New Class: NodeMetricsTile

**Location**: `include/ui/NodeMetricsTile.hpp` (NEW FILE)

```cpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <deque>
#include <nlohmann/json.hpp>
#include <ftxui/dom/elements.hpp>
#include "ui/MetricsTileWindow.hpp"  // Reuse MetricDescriptor

class NodeMetricsTile {
public:
    // Constructor: takes node_name and all field specs from schema
    NodeMetricsTile(
        const std::string& node_name,
        const std::vector<MetricDescriptor>& descriptors,
        const nlohmann::json& metrics_schema);

    // Update a single metric within this node tile
    void UpdateMetricValue(const std::string& metric_name, double value);

    // Update all metrics at once
    void UpdateAllValues(const std::map<std::string, double>& values);

    // Render consolidated tile as FTXUI element
    ftxui::Element Render() const;

    // Get tile sizing info
    int GetMinHeightLines() const;
    std::string GetNodeName() const { return node_name_; }
    size_t GetFieldCount() const { return field_descriptors_.size(); }

    // Get overall status (worst status of all fields)
    AlertStatus GetStatus() const;

private:
    std::string node_name_;
    std::vector<MetricDescriptor> field_descriptors_;  // All fields for this node
    std::map<std::string, double> latest_values_;      // metric_name -> value
    nlohmann::json metrics_schema_;

    // Per-field data for rendering
    struct FieldRender {
        std::string metric_name;
        double current_value;
        AlertStatus status;
        std::string display_type;
        std::deque<double> history;  // For sparkline rendering
    };

    std::vector<FieldRender> field_renders_;
    mutable std::mutex values_lock_;

    // Private rendering helpers
    ftxui::Element RenderFieldRow(const FieldRender& field) const;
    AlertStatus ComputeFieldStatus(const MetricDescriptor& desc, double value) const;
    AlertStatus ComputeOverallStatus() const;
    ftxui::Color GetStatusColor(AlertStatus status) const;
};
```

### 3.2 CRITICAL FIX: Rendering Architecture with Scrolling

**Problem**: Current render uses `size(HEIGHT, EQUAL, N)` which FORCES all content into N lines.

**Solution**: Use FTXUI scrollable container instead of forcing fit.

#### 3.2a MetricsTilePanel Render Method (FIXED)

**File**: `src/ui/MetricsTilePanel.cpp`

```cpp
ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    if (node_tiles_.empty()) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    // CRITICAL FIX: Create vertical container for nodes, then make scrollable
    // This allows nodes to render at full height, not squeezed
    
    std::vector<Element> node_elements;
    
    for (const auto& [node_name, node_tile] : node_tiles_) {
        // Each node tile renders at full natural height (7-10 lines for 3-5 fields)
        // NOT forced to fit in available window height
        node_elements.push_back(node_tile->Render());
        
        // Separator between nodes
        if (node_name != node_tiles_.rbegin()->first) {
            node_elements.push_back(separator());
        }
    }
    
    // KEY CHANGE: Wrap in scrollable container instead of forcing content
    auto scrollable_container = Container::Vertical(node_elements) | 
                                yframe |                              // Enable vertical scrolling
                                border;
    
    return scrollable_container;
    // DO NOT apply size(HEIGHT, EQUAL, N) - this destroys the scrolling!
    // The parent MetricsPanel will apply size constraint if needed
}
```

#### 3.2b Dashboard Layout with Proper Height Allocation

**File**: `src/ui/Dashboard.cpp` (BuildLayout method)

```cpp
ftxui::Element Dashboard::BuildLayout() const {
    using namespace ftxui;
    
    // DO NOT force MetricsTilePanel to exact height
    // Instead, allocate percentage and let scrolling handle overflow
    
    int metrics_height = std::max(3, (terminal_height * 68) / 100);
    
    std::vector<Element> layout_elements;
    
    if (metrics_panel_) {
        layout_elements.push_back(
            metrics_panel_->Render()
            // OLD (broken): | size(HEIGHT, EQUAL, metrics_height)
            // NEW (fixed): Allow content to flow, scrolling handles overflow
            | size(HEIGHT, AT_LEAST, metrics_height)  // Minimum height, not maximum squeeze
        );
    }
    
    // ... other components ...
    
    return vbox(layout_elements);
}
```

**Explanation**: 
- `size(HEIGHT, EQUAL, N)` = "force exactly N lines" → content gets squashed/hidden
- `size(HEIGHT, AT_LEAST, N)` = "use at least N lines, more if needed" → scrolling works

### 3.3 Modified: MetricsTilePanel

**File**: `include/ui/MetricsTilePanel.hpp`

```cpp
// Option: Change internal storage structure

class MetricsTilePanel {
private:
    // OLD (current):
    // struct Node {
    //     std::string name;
    //     std::map<std::string, std::shared_ptr<MetricsTileWindow>> metrics;
    // };
    // std::vector<Node> nodes_;

    // NEW:
    std::map<std::string, std::shared_ptr<NodeMetricsTile>> node_tiles_;
    // node_name -> single consolidated NodeMetricsTile
};
```

**Changes to methods**:

#### 3.2a AddTile() - DEPRECATED

```cpp
// OLD: Creates individual tile
void AddTile(std::shared_ptr<MetricsTileWindow> tile);

// NEW: Add entire node's metrics at once
void AddNodeMetrics(
    const std::string& node_name,
    const std::vector<MetricDescriptor>& descriptors,
    const nlohmann::json& metrics_schema);
```

#### 3.2b SetLatestValue() - MODIFIED

```cpp
// OLD:
void SetLatestValue(const std::string& metric_id, double value);
// Expects: "NodeName::metric_name"

// NEW: Same behavior, but routes to NodeMetricsTile instead
void SetLatestValue(const std::string& metric_id, double value);
// Parses metric_id and calls node_tile->UpdateMetricValue(metric_name, value)
```

#### 3.2c Render() - SIMPLIFIED

```cpp
ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    if (node_tiles_.empty()) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    std::vector<Element> node_sections;
    
    for (const auto& [node_name, node_tile] : node_tiles_) {
        // Single consolidated tile per node
        node_sections.push_back(node_tile->Render());
        
        // Add separator between nodes
        if (node_name != node_tiles_.rbegin()->first) {
            node_sections.push_back(separator());
        }
    }
    
    return vbox(std::move(node_sections)) | border | color(Color::Green);
}
```

#### 3.2d UpdateAllMetrics() - SIMPLIFIED

```cpp
void MetricsTilePanel::UpdateAllMetrics() {
    // Get latest values snapshot (thread-safe)
    std::map<std::string, double> values_snapshot;
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        values_snapshot = latest_values_;
    }
    
    // Route each metric to its node tile
    for (const auto& [metric_id, value] : values_snapshot) {
        auto [node_name, metric_name] = ParseMetricId(metric_id);
        
        if (auto node_it = node_tiles_.find(node_name); node_it != node_tiles_.end()) {
            node_it->second->UpdateMetricValue(metric_name, value);
        }
    }
}
```

#### 3.2e GetTileCount() - RETURNS NODE COUNT

```cpp
// OLD: Returns total field count (e.g., 12 tiles for 3 nodes with 4 fields each)
size_t GetTileCount() const { return total_tile_count_; }

// NEW: Returns node count (e.g., 3 for 3 nodes)
// OR: Add both methods
size_t GetNodeCount() const { return node_tiles_.size(); }
size_t GetTotalFieldCount() const { return total_field_count_; }
```

### 3.3 Modified: MetricsPanel Discovery

**File**: `src/ui/MetricsPanel.cpp` (lines 80-130)

```cpp
void MetricsPanel::DiscoverMetricsFromExecutor(
    std::shared_ptr<app::capabilities::MetricsCapability> capability) {
    
    if (!capability) return;
    
    const auto& schemas = capability->GetNodeMetricsSchemas();
    
    for (const auto& schema : schemas) {
        // OLD: Loop through fields, create individual tile for each
        // for (const auto& field : schema.metrics_schema["fields"]) {
        //     auto tile = std::make_shared<MetricsTileWindow>(descriptor, field);
        //     metrics_tile_panel_->AddTile(tile);
        // }
        
        // NEW: Create one consolidated node tile with all fields
        std::vector<MetricDescriptor> descriptors;
        
        if (schema.metrics_schema.contains("fields")) {
            for (const auto& field : schema.metrics_schema["fields"]) {
                MetricDescriptor desc;
                desc.node_name = schema.node_name;
                desc.metric_name = field["name"];
                desc.metric_id = schema.node_name + "::" + desc.metric_name;
                desc.unit = field.contains("unit") ? field["unit"].get<std::string>() : "";
                
                descriptors.push_back(desc);
            }
        }
        
        // Add entire node's metrics at once
        metrics_tile_panel_->AddNodeMetrics(
            schema.node_name,
            descriptors,
            schema.metrics_schema
        );
    }
    
    // Tab mode activation logic changes:
    // OLD: if (tile_count > 36) ActivateTabMode();
    // NEW: if (node_count > 6) ActivateTabMode();  // 6 consolidated nodes per screen
}
```

---

## 4. Display Rendering Details

### 4.1 Field Row Layout

Each field within a consolidated tile rendered as a row:

```
┌─────────────────────────────────────┐
│ EstimationPipelineNode (3 metrics)  │
├─────────────────────────────────────┤
│ estimation_quality:      92.5%      │
│   └─ Gauge: [████████░░░░░░░░░]     │
│                                    │
│ fusion_cycles:           1,234      │
│   └─ Value only                     │
│                                    │
│ estimation_latency_ms:   45.2 ms    │
│   └─ Sparkline: ╱╲╱╲╱╲╱╲╱╲╱╲       │
│                                    │
│ Status: OK (🟢) / WARN (🟡) / CRIT (🔴) │
└─────────────────────────────────────┘
```

### 4.2 Consolidated Status Display

Overall node status is **worst of all fields**:
- If any field is CRITICAL → node shows CRITICAL
- Else if any field is WARNING → node shows WARNING
- Else → node shows OK

### 4.3 Height Calculation

```cpp
int NodeMetricsTile::GetMinHeightLines() const {
    // Header (2 lines)
    // + one line per field
    // + status line (1)
    // + padding (1)
    return 2 + field_count + 1 + 1;
    
    // Example: 3 fields = 2 + 3 + 1 + 1 = 7 lines minimum
}
```

---

## 5. Data Flow Updates

### 5.1 Event Processing (OnMetricsEvent)

```
MetricsEvent arrives
    ├── event.metric_id = "EstimationPipelineNode::fusion_cycles"
    └── event.value = 1234.0

MetricsPanel::OnMetricsEvent(event)
    └── metrics_tile_panel_->SetLatestValue(event.metric_id, event.value)

MetricsTilePanel::SetLatestValue()
    ├── Parse metric_id → (node_name="EstimationPipelineNode", metric_name="fusion_cycles")
    └── node_tiles_["EstimationPipelineNode"]->UpdateMetricValue("fusion_cycles", 1234.0)

NodeMetricsTile::UpdateMetricValue()
    ├── Lock values_lock_
    ├── latest_values_["fusion_cycles"] = 1234.0
    ├── Unlock
    └── Return (note: does NOT trigger re-render immediately)
```

### 5.2 Rendering (UpdateAllMetrics + Render)

```
Event loop iteration
    ├── UpdateAllMetrics() called
    │   └── Routes all buffered values to respective NodeMetricsTiles
    │
    └── BuildLayout() called
        └── Render() called
            └── NodeMetricsTile::Render() for each node
                └── Returns FTXUI Element with all fields displayed
```

---

## 6. Backward Compatibility & Migration

### 6.1 GetTileCount() Change

**Current code** expects individual tile count:
```cpp
if (metrics_tile_panel_->GetTileCount() > 36) {
    ActivateTabMode();
}
```

**Migrate to**:
```cpp
if (metrics_tile_panel_->GetNodeCount() > 6) {
    ActivateTabMode();  // ~6 consolidated nodes fit on screen
}
```

### 6.2 GetTile() / GetTilesForNode()

**Current methods** may be removed:
```cpp
// OLD:
std::shared_ptr<MetricsTileWindow> GetTile(const std::string& metric_id);
std::vector<std::shared_ptr<MetricsTileWindow>> GetTilesForNode(const std::string& node_name);

// NEW:
std::shared_ptr<NodeMetricsTile> GetNodeTile(const std::string& node_name);
```

### 6.3 Test Updates Required

**Files to update**:
- `test/ui/test_phase2_metrics.cpp` - assertions about tile count
- `test/graph/test_mock_graph_executor.cpp` - any tile-related assumptions

---

## 7. Benefits & Trade-offs

### 7.1 Benefits

| Aspect | Benefit |
|--------|---------|
| **Visual Clarity** | Grouped metrics reduce eye movement; logical units easier to scan |
| **Space Efficiency** | Fewer boxes means less border/padding overhead |
| **Scalability** | Works better for nodes with many fields (3-10 fields) |
| **Maintenance** | Simpler data structure: map<node_name, tile> vs map<metric_id, tile> |
| **User Mental Model** | "Node X has these metrics" is more intuitive |

### 7.2 Trade-offs

| Trade-off | Impact |
|-----------|--------|
| **Per-field styling loss** | Can't have different display types per field within same tile (e.g., one gauge + one sparkline) |
| **Complexity increase** | New NodeMetricsTile class to maintain |
| **Refactoring effort** | Requires updating MetricsPanel discovery and storage |
| **Tab mode logic** | Threshold changes from 36 tiles → 6 nodes |

### 7.3 Performance Considerations

**Memory**: Slightly reduced (fewer MetricsTileWindow objects)  
**Rendering**: Simpler (fewer FTXUI elements to compose)  
**Update latency**: Unchanged (still lock-free per node)

---

## 8. Implementation Roadmap

### Phase 0: CRITICAL - Fix Rendering Architecture (DO THIS FIRST)
- [ ] Update `MetricsTilePanel::Render()` to use scrollable container instead of forcing fit
- [ ] Change `Dashboard::BuildLayout()` to use `size(HEIGHT, AT_LEAST, N)` instead of `EQUAL`
- [ ] Replace FTXUI container with scrollable support
- [ ] Add scroll indicators to show when more nodes exist below
- [ ] Test with current multi-tile design first (before consolidation)
- **BLOCKING**: Current design is unreadable until this is fixed
- **Estimated effort**: 1-2 hours

### Phase 1: Create NodeMetricsTile
- [ ] Create `include/ui/NodeMetricsTile.hpp` with class definition
- [ ] Create `src/ui/NodeMetricsTile.cpp` with implementation
- [ ] Implement `Render()` with scrollable-friendly field row layout
- [ ] Implement `UpdateMetricValue()` and `UpdateAllValues()`
- [ ] Write unit tests in `test/ui/test_node_metrics_tile.cpp`

### Phase 2: Refactor MetricsTilePanel
- [ ] Change internal storage from `vector<Node>` to `map<node_name, NodeMetricsTile>`
- [ ] Update `AddTile()` → `AddNodeMetrics()` (signature change)
- [ ] Update `SetLatestValue()` to route to NodeMetricsTile
- [ ] Simplify `Render()` (remove 3-column grid, use consolidation)
- [ ] Update `GetTileCount()` semantics or split into `GetNodeCount()`
- [ ] Update `UpdateAllMetrics()` to use new storage

### Phase 3: Update MetricsPanel Discovery
- [ ] Modify `DiscoverMetricsFromExecutor()` to batch fields per node
- [ ] Change tab mode threshold from 36 → 6
- [ ] Test with mock executor

### Phase 4: Update Tests
- [ ] Fix `test_phase2_metrics.cpp` assertions
- [ ] Fix any mock executor tile count assumptions
- [ ] Add new tests for NodeMetricsTile behavior

### Phase 5: Verify & Integrate
- [ ] Build and run full test suite
- [ ] Manual testing with dashboard GUI
- [ ] Verify keyboard tab navigation works with consolidated display

---

## 9. Code Examples

### 9.1 Creating NodeMetricsTile

```cpp
// In MetricsPanel::DiscoverMetricsFromExecutor()

std::vector<MetricDescriptor> descriptors;

// Collect all fields for this node
for (const auto& field : schema.metrics_schema["fields"]) {
    MetricDescriptor desc;
    desc.node_name = schema.node_name;
    desc.metric_name = field["name"];
    desc.metric_id = schema.node_name + "::" + desc.metric_name;
    descriptors.push_back(desc);
}

// Create single consolidated tile
auto node_tile = std::make_shared<NodeMetricsTile>(
    schema.node_name,
    descriptors,
    schema.metrics_schema
);

metrics_tile_panel_->AddNodeTile(node_tile);
```

### 9.2 Rendering a Field Row

```cpp
// In NodeMetricsTile::RenderFieldRow()

ftxui::Element NodeMetricsTile::RenderFieldRow(const FieldRender& field) const {
    using namespace ftxui;
    
    // Format: "metric_name:    value    unit"
    std::string label = field.metric_name + ":";
    std::string value_str = FormatValue(field.current_value) + " " + 
                            GetUnitForMetric(field.metric_name);
    
    auto row = hbox({
        text(label) | width(LESS_THAN, 25),
        filler(),
        text(value_str) | color(GetStatusColor(field.status))
    });
    
    return row;
}
```

### 9.3 Overall Render

```cpp
ftxui::Element NodeMetricsTile::Render() const {
    using namespace ftxui;
    
    std::string header = node_name_ + " (" + 
                        std::to_string(field_descriptors_.size()) + " metrics)";
    
    std::vector<Element> rows;
    rows.push_back(text(header) | bold | color(Color::Cyan));
    rows.push_back(separator());
    
    // Add each field
    for (const auto& field : field_renders_) {
        rows.push_back(RenderFieldRow(field));
    }
    
    // Add status line
    auto status = ComputeOverallStatus();
    rows.push_back(separator());
    rows.push_back(text("Status: " + StatusString(status)) | 
                   color(GetStatusColor(status)) | right);
    
    return vbox(std::move(rows)) | border | color(Color::Green);
}
```

---

## 10. Questions & Decisions

### Q1: Per-field Display Type

**Current**: Each MetricsTileWindow can have different display type (gauge, sparkline, value, state)

**Decision**: In consolidated view, either:
- **Option A** (Recommended): All fields use "value" display with optional sparkline
- **Option B**: Support mixed display types within same tile (more complex layout)
- **Option C**: Let field JSON spec define display type per field

**Recommendation**: Option A for v1 (simpler), upgrade to Option C later

### Q2: Field Ordering

**Decision**: Display fields in order they appear in `metrics_schema["fields"]` array

### Q3: Collapsed/Expanded State

**Future consideration**: Support collapsing a node tile to show only status line

---

## 11. References

- Current implementation: [MetricsTilePanel.hpp](include/ui/MetricsTilePanel.hpp)
- Current tile: [MetricsTileWindow.hpp](include/ui/MetricsTileWindow.hpp)
- Discovery code: [MetricsPanel.cpp](src/ui/MetricsPanel.cpp) lines 80-130
- Schema structure: [NodeMetricsSchema.hpp](include/app/metrics/NodeMetricsSchema.hpp)

---

## 12. CRITICAL SUMMARY: The Rendering Problem is the ROOT CAUSE

### Current Situation
The dashboard metrics panel is **currently unreadable** because:
1. All tiles are forced into a small fixed height
2. No scrolling mechanism exists
3. Content gets clipped/overlapped and invisible
4. 3-column grid layout makes it worse (more tiles crammed in)

### Why Consolidation Alone Won't Fix It
Simply changing from many small tiles to fewer large tiles **will not fix the underlying rendering problem**. You need BOTH:
1. **Consolidation**: Reduce number of tiles (1 per node instead of 1 per field)
2. **Scrolling**: Enable vertical scrolling in the metrics panel

### The Fix (in order of priority)
1. **Phase 0 (CRITICAL)**: Implement scrollable metrics container
   - Makes current design readable (even before consolidation)
   - Takes 1-2 hours
   - Unblocks dashboard testing
   
2. **Phase 1-5**: Implement consolidation + cleanup
   - Makes design more elegant
   - Reduces visual clutter further
   - Takes additional 2-3 hours

### Without Phase 0
- Consolidation alone won't make content readable
- Scrolling is NECESSARY for multi-node metrics display
- Current metrics panel remains broken until scrolling is added

### Recommended Approach
1. Start with Phase 0: Add scrolling to current multi-tile design
2. Verify metrics are now readable
3. Then proceed with consolidation in Phases 1-5
4. This gives you working metrics quickly, then improves layout gradually

---

## Summary

The proposed change consolidates metrics display from **many small tiles** (one per field) to **fewer larger tiles** (one per node), BUT it ALSO fixes the critical rendering problem by implementing scrolling. This makes the dashboard:
- ✅ Readable (Phase 0)
- ✅ Well-organized (Phase 1-5)
- ✅ Scalable (scrolling handles many nodes)

**Key implementation point**: Create `NodeMetricsTile` class AND fix rendering with scrollable container.

**Critical path**: Phase 0 (scrolling fix) → Phase 1 (consolidation) → Phases 2-5 (cleanup)

**Estimated total effort**: 3-5 hours (Phase 0: 1-2h + Phases 1-5: 2-3h)

## Summary

The proposed change consolidates metrics display from **many small tiles** (one per field) to **fewer larger tiles** (one per node). This improves readability, reduces visual clutter, and aligns better with the mental model of nodes as first-class entities in the computation graph.

**Key implementation point**: Create `NodeMetricsTile` class to hold and render all fields from a NodeMetricsSchema, then modify `MetricsTilePanel` to use consolidated tiles instead of individual tiles.

**Estimated effort**: ~2-3 hours implementation + testing
