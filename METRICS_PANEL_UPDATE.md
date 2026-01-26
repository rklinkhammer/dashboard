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
│ estimation_quality:      92.5%      │ ← Green text (OK status)
│ fusion_cycles:           1,234      │ ← Green text (OK status)
│ estimation_latency_ms:   45.2 ms    │ ← Yellow text (WARNING status)
└─────────────────────────────────────┘
```

**Efficiency optimization**: Use text color to indicate status instead of gauges/sparklines:
- ✅ Single line per metric (not 2-3 lines for gauge/sparkline)
- ✅ Renders instantly (no calculation overhead)
- ✅ Compact display (more metrics fit per tile)
- ✅ Color intuitive: Red=bad, Yellow=warning, Green=good

**Rendering implementation**:
```cpp
// Efficient: Just color the value text based on status
std::string value_str = FormatValue(field.current_value) + " " + unit;
auto row = hbox({
    text(label) | width(LESS_THAN, 25),
    filler(),
    text(value_str) | color(GetStatusColor(field.status))  // ← Color based on status
});
```

**For complex visualizations** (gauges, sparklines, state machines):
- These get their own separate tiles (see Q4: Hybrid Approach)
- Create dedicated `MetricsTileWindow` with full rendering capability
- Only used when metric truly needs visual representation beyond text+color

### 4.2 Consolidated Status Display

Overall node status is **worst of all fields**:
- If any field is CRITICAL → node shows CRITICAL
- Else if any field is WARNING → node shows WARNING
- Else → node shows OK

### 4.3 Height Calculation

```cpp
int NodeMetricsTile::GetMinHeightLines() const {
    // Header (1 line)
    // + one line per field (single-line compact rows)
    // + separator (1)
    // Total = 2 + field_count
    
    // Example: 3 fields = 2 + 3 = 5 lines
    return 2 + static_cast<int>(field_descriptors_.size());
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

**Objective**: Enable scrolling in MetricsTilePanel so current design is readable  
**Effort**: 1-2 hours  
**Blocker**: Nothing else works until this is fixed

#### Step 0.1: Update Dashboard::BuildLayout()
**File**: `src/ui/Dashboard.cpp`

- [ ] Locate `BuildLayout()` method
- [ ] Find line with `size(HEIGHT, EQUAL, metrics_height)` constraint on MetricsPanel
- [ ] Replace with `size(HEIGHT, AT_LEAST, metrics_height)`
- [ ] Ensure parent vbox allocates remaining space correctly

**Code change**:
```cpp
// OLD:
layout_elements.push_back(
    metrics_panel_->Render()
    | size(HEIGHT, EQUAL, metrics_height)
);

// NEW:
layout_elements.push_back(
    metrics_panel_->Render()
    | size(HEIGHT, AT_LEAST, metrics_height)
);
```

#### Step 0.2: Update MetricsTilePanel::Render() with Scrolling
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Locate current `Render()` method
- [ ] Find `vbox(std::move(node_sections)) | border` line
- [ ] Wrap container in scrollable element using FTXUI's yframe
- [ ] Test that scrolling works with keyboard (arrow keys)

**Code change**:
```cpp
// OLD:
return vbox(std::move(node_sections)) | border | color(Color::Green);

// NEW:
return Container::Vertical(std::move(node_sections)) 
    | yframe 
    | border 
    | color(Color::Green);
```

#### Step 0.3: Add Scroll Indicator
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Add scroll position tracking (if FTXUI doesn't provide built-in indicator)
- [ ] Display indicator showing "N nodes, M visible, scroll for more"
- [ ] Consider visual indicator (arrow, percentage, page indicator)

#### Step 0.4: Verify Phase 0
**Testing**:
- [ ] Build dashboard
- [ ] Run with metrics data from mock executor
- [ ] Verify all node tiles are now readable (not squeezed)
- [ ] Test scrolling with keyboard arrows (↑↓)
- [ ] Verify scroll wraps at boundaries
- [ ] Check no data is clipped or hidden

**Success criteria**:
- ✅ Can read all metric values in any node tile
- ✅ Can scroll through all nodes with many fields
- ✅ No visual clipping or overlapping
- ✅ Current multi-tile design is readable with consolidation coming later

---

### Phase 1: Create NodeMetricsTile Class

**Objective**: Implement new consolidated tile class  
**Effort**: 2 hours  
**Dependencies**: Phase 0 complete

#### Step 1.1: Create Header File
**File**: `include/ui/NodeMetricsTile.hpp` (NEW)

- [ ] Create file with class definition (see Section 3.1 for full spec)
- [ ] Include necessary headers:
  - `#include <memory>`
  - `#include <vector>`
  - `#include <string>`
  - `#include <deque>`
  - `#include <mutex>`
  - `#include <nlohmann/json.hpp>`
  - `#include <ftxui/dom/elements.hpp>`
  - `#include "ui/MetricsTileWindow.hpp"`

- [ ] Define class members:
  - `node_name_`, `field_descriptors_`, `latest_values_`
  - `FieldRender` struct with metric_name, current_value, status, display_type, history
  - `values_lock_` mutex for thread-safe updates

- [ ] Declare public methods:
  - Constructor: `NodeMetricsTile(node_name, descriptors, schema)`
  - `UpdateMetricValue(metric_name, value)`
  - `UpdateAllValues(values_map)`
  - `Render() const`
  - `GetMinHeightLines() const`
  - `GetNodeName() const`
  - `GetFieldCount() const`
  - `GetStatus() const`

- [ ] Declare private helper methods:
  - `RenderFieldRow(field)`
  - `ComputeFieldStatus(descriptor, value)`
  - `ComputeOverallStatus()`
  - `GetStatusColor(status)`

#### Step 1.2: Create Implementation File
**File**: `src/ui/NodeMetricsTile.cpp` (NEW)

- [ ] Implement constructor:
  - Store node_name and field_descriptors
  - Initialize field_renders_ vector with empty values
  - Parse display_type from schema for each field

- [ ] Implement `UpdateMetricValue()`:
  - Lock values_lock_
  - Update latest_values_ map
  - Update field_renders_ entry
  - Recalculate status
  - Unlock

- [ ] Implement `RenderFieldRow()`:
  - Format as: "metric_name:    value    unit"
  - Left-align name (25 chars max)
  - Right-align value (with unit)
  - Color value based on status (Green/Yellow/Red)
  - Use hbox with filler for spacing

- [ ] Implement `Render()`:
  - Create header row: "NodeName (N metrics)" in bold cyan
  - Add separator
  - Add field rows (one per field)
  - Add separator
  - Add status line (overall node status) right-aligned
  - Wrap in vbox | border | green

- [ ] Implement status computation:
  - Worst-case: CRITICAL > WARNING > OK
  - Each field status based on thresholds in MetricDescriptor

**Code skeleton**:
```cpp
NodeMetricsTile::NodeMetricsTile(const std::string& node_name, 
                                const std::vector<MetricDescriptor>& desc,
                                const nlohmann::json& schema)
    : node_name_(node_name), 
      field_descriptors_(desc),
      metrics_schema_(schema) {
    // Initialize field_renders_ from descriptors
    for (const auto& d : field_descriptors_) {
        FieldRender fr;
        fr.metric_name = d.metric_name;
        fr.current_value = 0.0;
        fr.status = AlertStatus::OK;
        fr.display_type = "value";
        fr.history.clear();
        field_renders_.push_back(fr);
    }
}

ftxui::Element NodeMetricsTile::Render() const {
    using namespace ftxui;
    
    std::vector<Element> rows;
    rows.push_back(text(node_name_ + " (" + 
                  std::to_string(field_descriptors_.size()) + " metrics)") 
                  | bold | color(Color::Cyan));
    rows.push_back(separator());
    
    for (const auto& field : field_renders_) {
        rows.push_back(RenderFieldRow(field));
    }
    
    rows.push_back(separator());
    auto status = ComputeOverallStatus();
    rows.push_back(text("Status: " + StatusString(status)) 
                  | color(GetStatusColor(status)) | right);
    
    return vbox(std::move(rows)) | border | color(Color::Green);
}
```

#### Step 1.3: Write Unit Tests
**File**: `test/ui/test_node_metrics_tile.cpp` (NEW)

- [ ] Create test fixture with sample MetricDescriptor and schema
- [ ] Test constructor initialization
- [ ] Test `UpdateMetricValue()` updates internal state
- [ ] Test `GetStatus()` computes correct overall status
- [ ] Test `Render()` produces valid FTXUI element
- [ ] Test height calculation matches expected formula
- [ ] Test thread-safety of concurrent updates

**Test cases**:
```cpp
TEST(NodeMetricsTileTest, ConstructorInitializes) { ... }
TEST(NodeMetricsTileTest, UpdateMetricValue) { ... }
TEST(NodeMetricsTileTest, StatusComputation) { ... }
TEST(NodeMetricsTileTest, RenderProducesElement) { ... }
TEST(NodeMetricsTileTest, HeightCalculation) { ... }
TEST(NodeMetricsTileTest, ConcurrentUpdates) { ... }
```

#### Step 1.4: Verify Phase 1
**Testing**:
- [ ] Compile with `make` or CMake build
- [ ] Run unit tests: `./test_node_metrics_tile`
- [ ] Verify all tests pass
- [ ] Check for any compiler warnings

**Success criteria**:
- ✅ Class compiles without errors
- ✅ All unit tests pass
- ✅ No memory leaks (if using sanitizers)
- ✅ Ready for integration into MetricsTilePanel

---

### Phase 2: Refactor MetricsTilePanel Storage & Methods

**Objective**: Update MetricsTilePanel to use NodeMetricsTile internally  
**Effort**: 2 hours  
**Dependencies**: Phase 0, Phase 1 complete

#### Step 2.1: Update Header File
**File**: `include/ui/MetricsTilePanel.hpp`

- [ ] Find existing Node struct definition
- [ ] Replace or modify internal storage:
  ```cpp
  // OLD:
  // struct Node {
  //     std::string name;
  //     std::map<std::string, std::shared_ptr<MetricsTileWindow>> metrics;
  // };
  // std::vector<Node> nodes_;

  // NEW:
  std::map<std::string, std::shared_ptr<NodeMetricsTile>> node_tiles_;
  ```

- [ ] Add include for NodeMetricsTile: `#include "ui/NodeMetricsTile.hpp"`

- [ ] Update method signatures:
  - Add: `void AddNodeMetrics(const std::string& node_name, const std::vector<MetricDescriptor>&, const nlohmann::json& schema);`
  - Keep but modify: `void SetLatestValue(const std::string& metric_id, double value);`
  - Deprecate: `void AddTile(std::shared_ptr<MetricsTileWindow> tile);`
  - Update: `size_t GetTileCount() const;` → add comment that it now returns node count OR rename to `GetNodeCount()`

#### Step 2.2: Update MetricsTilePanel::Render()
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Find current `Render()` method
- [ ] Replace entire implementation with new version using node_tiles_:

```cpp
ftxui::Element MetricsTilePanel::Render() const {
    using namespace ftxui;
    
    if (node_tiles_.empty()) {
        return text("No metrics") | center | color(Color::Yellow);
    }
    
    std::vector<Element> node_sections;
    
    for (const auto& [node_name, node_tile] : node_tiles_) {
        node_sections.push_back(node_tile->Render());
        
        // Add separator between nodes (but not after last)
        if (node_name != node_tiles_.rbegin()->first) {
            node_sections.push_back(separator());
        }
    }
    
    // Use scrollable container (from Phase 0 work)
    return Container::Vertical(std::move(node_sections)) 
        | yframe 
        | border 
        | color(Color::Green);
}
```

#### Step 2.3: Implement AddNodeMetrics()
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Add new method implementation:
```cpp
void MetricsTilePanel::AddNodeMetrics(
    const std::string& node_name,
    const std::vector<MetricDescriptor>& descriptors,
    const nlohmann::json& metrics_schema) {
    
    auto node_tile = std::make_shared<NodeMetricsTile>(
        node_name, descriptors, metrics_schema);
    
    node_tiles_[node_name] = node_tile;
    total_field_count_ += descriptors.size();
}
```

#### Step 2.4: Update SetLatestValue()
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Modify to parse metric_id and route to correct NodeMetricsTile:
```cpp
void MetricsTilePanel::SetLatestValue(const std::string& metric_id, double value) {
    // metric_id format: "NodeName::metric_name"
    auto pos = metric_id.find("::");
    if (pos == std::string::npos) return;
    
    std::string node_name = metric_id.substr(0, pos);
    std::string metric_name = metric_id.substr(pos + 2);
    
    auto it = node_tiles_.find(node_name);
    if (it != node_tiles_.end()) {
        it->second->UpdateMetricValue(metric_name, value);
    }
}
```

#### Step 2.5: Update GetTileCount() or Add GetNodeCount()
**File**: `include/ui/MetricsTilePanel.hpp` & `src/ui/MetricsTilePanel.cpp`

- [ ] Option A: Rename and update semantics
  ```cpp
  // OLD: returns number of individual field tiles
  // NEW: returns number of node tiles
  size_t GetNodeCount() const { return node_tiles_.size(); }
  ```

- [ ] Option B: Keep both methods
  ```cpp
  size_t GetNodeCount() const { return node_tiles_.size(); }
  size_t GetTotalFieldCount() const { return total_field_count_; }
  size_t GetTileCount() const { return GetNodeCount(); } // Deprecated
  ```

#### Step 2.6: Update UpdateAllMetrics()
**File**: `src/ui/MetricsTilePanel.cpp`

- [ ] Find existing implementation
- [ ] Replace to use new storage:
```cpp
void MetricsTilePanel::UpdateAllMetrics() {
    // Get snapshot of latest values
    std::map<std::string, double> values_snapshot;
    {
        std::lock_guard<std::mutex> lock(values_lock_);
        values_snapshot = latest_values_;
    }
    
    // Route each value to its node tile
    for (const auto& [metric_id, value] : values_snapshot) {
        SetLatestValue(metric_id, value);
    }
}
```

#### Step 2.7: Verify Phase 2
**Testing**:
- [ ] Compile without errors
- [ ] Update existing MetricsTilePanel tests to use new AddNodeMetrics() API
- [ ] Verify old AddTile() is deprecated (if still present)
- [ ] Test SetLatestValue() routing to correct node tile
- [ ] Test Render() produces correct output

**Success criteria**:
- ✅ No compilation errors
- ✅ Unit tests still pass with new storage
- ✅ Render output matches expected format
- ✅ Values are correctly routed to node tiles

---

### Phase 3: Update MetricsPanel Discovery

**Objective**: Modify MetricsPanel to batch fields per node when creating tiles  
**Effort**: 1 hour  
**Dependencies**: Phase 0, 1, 2 complete

#### Step 3.1: Update DiscoverMetricsFromExecutor()
**File**: `src/ui/MetricsPanel.cpp` (lines 80-130)

- [ ] Find current discovery loop that creates individual tiles
- [ ] Replace with logic that:
  1. Groups fields by node_name
  2. Creates one NodeMetricsTile per node
  3. Calls AddNodeMetrics() instead of AddTile()

```cpp
void MetricsPanel::DiscoverMetricsFromExecutor(
    std::shared_ptr<app::capabilities::MetricsCapability> capability) {
    
    if (!capability) return;
    
    const auto& schemas = capability->GetNodeMetricsSchemas();
    
    for (const auto& schema : schemas) {
        // Collect all field descriptors for this node
        std::vector<MetricDescriptor> descriptors;
        
        if (schema.metrics_schema.contains("fields")) {
            for (const auto& field : schema.metrics_schema["fields"]) {
                MetricDescriptor desc;
                desc.node_name = schema.node_name;
                desc.metric_name = field["name"];
                desc.metric_id = schema.node_name + "::" + desc.metric_name;
                desc.unit = field.contains("unit") 
                    ? field["unit"].get<std::string>() 
                    : "";
                // ... other descriptor fields from field JSON
                
                descriptors.push_back(desc);
            }
        }
        
        // Create ONE consolidated tile for all fields in this node
        metrics_tile_panel_->AddNodeMetrics(
            schema.node_name,
            descriptors,
            schema.metrics_schema
        );
    }
    
    // Update tab mode threshold (changed from individual tiles to nodes)
    // OLD: if (metrics_tile_panel_->GetTileCount() > 36) ActivateTabMode();
    // NEW: Approximately 6 consolidated nodes fit on typical terminal
    if (metrics_tile_panel_->GetNodeCount() > 6) {
        ActivateTabMode();
    }
}
```

#### Step 3.2: Update Tab Mode Logic
**File**: `src/ui/MetricsPanel.cpp`

- [ ] Locate any tab mode threshold checks
- [ ] Update from 36 tiles → 6 nodes:
  ```cpp
  // OLD threshold: 36 individual field tiles
  // NEW threshold: 6 consolidated node tiles
  // Rationale: A 120-char wide terminal with 3-column grid would show ~15 tiles
  //           With consolidation, each node takes more height but fewer total tiles
  //           ~6 nodes visible per screen, rest scrollable
  if (metrics_tile_panel_->GetNodeCount() > 6) {
      ActivateTabMode();
  }
  ```

#### Step 3.3: Verify Phase 3
**Testing**:
- [ ] Compile without errors
- [ ] Run with mock executor that provides NodeMetricsSchemas
- [ ] Verify discovery creates one tile per node
- [ ] Verify all fields appear in consolidated tiles
- [ ] Test tab mode threshold activation

**Success criteria**:
- ✅ Discovery correctly batches fields by node
- ✅ Consolidated tiles created instead of individual tiles
- ✅ Tab mode threshold adjusted appropriately
- ✅ No fields missing from discovery

---

### Phase 4: Update Tests & Compatibility

**Objective**: Fix tests broken by structural changes  
**Effort**: 1.5 hours  
**Dependencies**: Phases 0-3 complete

#### Step 4.1: Update Existing Tests
**Files to update**:
- `test/ui/test_phase2_metrics.cpp`
- `test/graph/test_mock_graph_executor.cpp`
- Any other tests that use MetricsTilePanel or make tile count assertions

- [ ] Find assertions like `ASSERT_EQ(metrics_panel->GetTileCount(), expected_count);`
- [ ] Update to use node count instead:
  ```cpp
  // OLD:
  ASSERT_EQ(metrics_tile_panel->GetTileCount(), 9);  // 3 nodes × 3 fields
  
  // NEW:
  ASSERT_EQ(metrics_tile_panel->GetNodeCount(), 3);  // 3 nodes
  ASSERT_EQ(metrics_tile_panel->GetTotalFieldCount(), 9);  // All fields
  ```

- [ ] Find any mock executor setup that creates individual tiles
- [ ] Update to use NodeMetricsSchemas with batched fields

#### Step 4.2: Create Integration Tests
**File**: `test/ui/test_metrics_panel_consolidation.cpp` (NEW)

- [ ] Test full discovery → MetricsTilePanel → render flow
- [ ] Verify consolidated tiles render correctly with real schema
- [ ] Test updating values reaches correct fields in consolidated tiles
- [ ] Test scrolling (if FTXUI supports testing)

#### Step 4.3: Update Mock Executor
**File**: `test/graph/MockGraphExecutor.cpp` (if applicable)

- [ ] Update any tile count assertions
- [ ] Update discovery to match new batching logic
- [ ] Provide proper NodeMetricsSchemas with field arrays

#### Step 4.4: Verify Phase 4
**Testing**:
- [ ] Run full test suite: `make test` or `ctest`
- [ ] Verify all tests pass
- [ ] Check test coverage
- [ ] No test regressions

**Success criteria**:
- ✅ All existing tests pass with no modifications to test expectations
- ✅ New tests cover consolidation behavior
- ✅ No test failures or warnings

---

### Phase 5: Verify & Integrate

**Objective**: Full integration testing and deployment  
**Effort**: 1 hour  
**Dependencies**: Phases 0-4 complete

#### Step 5.1: Full Build & Test
**Testing**:
- [ ] Clean build: `rm -rf build && mkdir build && cd build && cmake .. && make`
- [ ] Run unit test suite: `ctest --output-on-failure`
- [ ] Run integration tests
- [ ] Check for compiler warnings: `make VERBOSE=1 2>&1 | grep -i warning`

#### Step 5.2: Manual GUI Testing
**Testing prerequisites**: 
- Set up mock executor or live system
- Build dashboard binary
- Run with metrics data

**Test scenarios**:
- [ ] Dashboard starts without errors
- [ ] Metrics panel displays all nodes as consolidated tiles
- [ ] Scrolling works (arrow keys move through nodes)
- [ ] All field values are visible (not clipped)
- [ ] Status colors correct (Green/Yellow/Red based on thresholds)
- [ ] Tab navigation works if tab mode active (>6 nodes)
- [ ] No visual artifacts or overlapping text
- [ ] Rendering performance acceptable (no lag with 10+ nodes)

#### Step 5.3: Performance Verification
**Testing**:
- [ ] Dashboard with 5 nodes: smooth rendering
- [ ] Dashboard with 20 nodes: scrolling responsive
- [ ] Dashboard with 100 fields: no memory leaks
- [ ] Update throughput: can handle N metrics/second without visual lag

#### Step 5.4: Backward Compatibility Check
**Testing**:
- [ ] Any public APIs changed? (Document in migration guide)
- [ ] GetTileCount() deprecation handled? (Keep as alias or remove?)
- [ ] Any dependent code needs updates? (Search codebase)

#### Step 5.5: Documentation & Commit
**Final steps**:
- [ ] Update code comments explaining NodeMetricsTile consolidation
- [ ] Update README or design docs with new architecture
- [ ] Create git commit: `git commit -m "Implement metrics consolidation & scrolling (Phases 0-5)"`
- [ ] Optional: Create merge request for code review

**Success criteria**:
- ✅ Full test suite passes (unit + integration + manual)
- ✅ Dashboard displays metrics correctly with scrolling
- ✅ No performance regressions
- ✅ Code is production-ready
- ✅ All changes documented

---

## Implementation Notes

### Dependencies & Build Order
```
Phase 0: Rendering fix (prerequisite for all others)
    ├── Phase 1: NodeMetricsTile class
    │   ├── Phase 2: Refactor MetricsTilePanel
    │   │   ├── Phase 3: Update discovery
    │   │   │   ├── Phase 4: Update tests
    │   │   │   │   └── Phase 5: Final verification
```

### Critical Files to Modify
1. **src/ui/Dashboard.cpp** - Line with `size(HEIGHT, EQUAL, ...)` (Phase 0)
2. **src/ui/MetricsTilePanel.cpp** - Render() method (Phase 0, 2)
3. **include/ui/MetricsTilePanel.hpp** - Storage structure (Phase 2)
4. **src/ui/MetricsPanel.cpp** - Discovery logic (Phase 3)
5. **test/ui/test_phase2_metrics.cpp** - Test updates (Phase 4)

### Files to Create
1. **include/ui/NodeMetricsTile.hpp** (Phase 1)
2. **src/ui/NodeMetricsTile.cpp** (Phase 1)
3. **test/ui/test_node_metrics_tile.cpp** (Phase 1)
4. **test/ui/test_metrics_panel_consolidation.cpp** (Phase 4)

### Rollback Strategy
If issues occur:
- **Phase 0 issues**: Revert Dashboard.cpp to use `EQUAL` height (single commit)
- **Phase 1 issues**: Remove NodeMetricsTile files (new files only)
- **Phase 2 issues**: Revert MetricsTilePanel to old storage, revert discovery (harder)
- **Phase 3+ issues**: Git revert individual commits as needed

### Time Estimates (Revised)
- Phase 0: 1-2 hours (critical path blocker)
- Phase 1: 2 hours (new class implementation)
- Phase 2: 2 hours (refactoring storage)
- Phase 3: 1 hour (update discovery)
- Phase 4: 1.5 hours (test updates)
- Phase 5: 1 hour (verification)

**Total: 8-8.5 hours** (originally estimated 3-5 hours, but detailed breakdown is more realistic)

### Testing Strategy
- **Unit tests**: Each phase independently testable
- **Integration tests**: End-to-end discovery → render flow
- **Manual testing**: Visual verification in dashboard GUI
- **Regression tests**: Ensure existing functionality preserved

### Commit Strategy
- Phase 0: One commit (rendering fix)
- Phase 1: One commit (NodeMetricsTile implementation)
- Phase 2: One commit (MetricsTilePanel refactoring)
- Phase 3: One commit (discovery update)
- Phase 4: One commit (test updates)
- Phase 5: Final verification (document in commit)

**Total: ~6 commits** for full implementation with clear separation of concerns

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

### Q4: Hybrid Approach for Special Display Types

**Problem**: Most metrics are simple text + value, but some need special rendering:
- Gauge display (requires width)
- Sparkline graph (requires height + 60-point history)
- State machine visualization (requires layout)
- Complex alerts (multi-row layout)

**Decision**: **Hybrid Consolidation with Special-case Tiles**

For each node:
1. **Create NodeMetricsTile** with all "basic" metrics (text + value format)
   - Simple numbers with labels
   - Colors for status
   - 1 line per metric
   
2. **If metric needs special display AND can't fit in NodeMetricsTile**, create additional standalone tile
   - Gauge tile (10 lines height)
   - Sparkline tile (5 lines height)
   - State visualization tile (varies)
   - Complex alert tile (varies)

**Example layout**:
```
┌──────────────────────────────┐
│ EstimationPipelineNode       │ ← NodeMetricsTile (consolidated)
├──────────────────────────────┤
│ estimation_quality: 92.5%    │ ← Basic metric in consolidated tile
│ fusion_cycles: 1,234         │ ← Basic metric in consolidated tile
│ estimation_latency_ms: 45.2  │ ← Basic metric in consolidated tile
│ Status: OK (🟢)              │
└──────────────────────────────┘

┌──────────────────────────────┐
│ EstimationPipelineNode      │ ← Additional tile for special display
│ [Quality Gauge]              │
│ ████████████░░░░░░░░░░░░░░░ │ ← Gauge needs more space
│ 92.5% of optimal             │
└──────────────────────────────┘

┌──────────────────────────────┐
│ EstimationPipelineNode      │ ← Another additional tile if needed
│ [Latency Sparkline]          │
│ ╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲ (chart) │
│ Trend: Stable                │
└──────────────────────────────┘
```

**Implementation logic**:
```cpp
// In MetricsPanel::DiscoverMetricsFromExecutor()

std::vector<MetricDescriptor> basic_metrics;
std::vector<std::pair<MetricDescriptor, std::string>> special_metrics;
// special_metrics = (descriptor, display_type)

for (const auto& field : schema.metrics_schema["fields"]) {
    std::string display_type = field.contains("display_type") 
        ? field["display_type"].get<std::string>() 
        : "value";  // Default
    
    MetricDescriptor desc = ...;
    
    if (display_type == "value" || display_type == "text") {
        // Basic metric - goes in consolidated NodeMetricsTile
        basic_metrics.push_back(desc);
    } else {
        // Special display - create separate MetricsTileWindow
        special_metrics.push_back({desc, display_type});
    }
}

// Create consolidated tile for basic metrics
auto node_tile = std::make_shared<NodeMetricsTile>(
    schema.node_name, basic_metrics, ...);
metrics_tile_panel_->AddNodeTile(node_tile);

// Create additional tiles for special metrics
for (const auto& [desc, display_type] : special_metrics) {
    auto special_tile = std::make_shared<MetricsTileWindow>(desc, ...);
    special_tile->SetDisplayType(display_type);
    metrics_tile_panel_->AddSpecialTile(special_tile);
    // These render AFTER consolidated tiles in the scrollable view
}
```

**Benefits**:
- ✅ Consolidated view for 80% of metrics (clean, compact)
- ✅ Special tiles for 20% of metrics (gauge, sparkline, etc.)
- ✅ Flexible: Add special display types without restructuring
- ✅ Scannable: Basic metrics easy to read, special metrics get focus

**Trade-offs**:
- ⚠️ Slightly more complex code (two tile types)
- ⚠️ Ordering matters: special tiles come after consolidated tile
- ⚠️ Metadata needed: JSON must specify display_type for fields

**Recommendation**: Implement in Phase 1B (after consolidation complete):
- Phase 1A: Basic consolidation (all fields as text+value)
- Phase 1B: Add special tile support for gauges, sparklines, etc.

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
