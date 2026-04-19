# Phase 2 Architecture Summary

## Metrics Panel Component Hierarchy

```
┌─────────────────────────────────────────────────────┐
│          MetricsTilePanel                           │
│  (Consolidated metrics container - Phase 2)         │
├─────────────────────────────────────────────────────┤
│                                                     │
│  NEW Storage: map<node_name, NodeMetricsTile>      │
│  ├─ "Producer" → NodeMetricsTile                  │
│  ├─ "Transformer" → NodeMetricsTile               │
│  └─ "Consumer" → NodeMetricsTile                  │
│                                                     │
│  LEGACY Storage: vector<Node> (backward compat)    │
│  ├─ Node("Producer")                              │
│  │  └─ metrics: map<metric_name, TileWindow>     │
│  ├─ Node("Transformer")                           │
│  │  └─ metrics: map<metric_name, TileWindow>     │
│  └─ Node("Consumer")                              │
│     └─ metrics: map<metric_name, TileWindow>      │
└─────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────┐
│      NodeMetricsTile (Per-Node Consolidation)       │
│      Phase 1 Component - Used by Phase 2            │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Storage: vector<MetricDescriptor>                 │
│  Values: map<metric_name, double>                  │
│  Schema: JSON field configurations                 │
│                                                     │
│  Methods:                                           │
│  - UpdateMetricValue(metric_name, value)          │
│  - Render() → ftxui::Element                       │
│  - GetStatus() → AlertStatus                       │
└─────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────┐
│   MetricsTileWindow (Individual Metrics)            │
│   Phase 0 Component - Used by Legacy Path           │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Represents single metric with:                     │
│  - Value display                                    │
│  - Status color (OK/Warning/Critical)              │
│  - Sparkline history                                │
│  - Alert rule thresholds                            │
│                                                     │
│  Methods:                                           │
│  - UpdateValue(value)                              │
│  - Render() → ftxui::Element                       │
│  - GetStatus() → AlertStatus                       │
└─────────────────────────────────────────────────────┘
```

## Data Flow: Phase 2 Update Pipeline

```
Metrics Event (from MetricsCapability)
│
├─→ MetricsTilePanel::SetLatestValue(metric_id, value)
│   └─→ Stores in: latest_values_["Node::metric"]
│       (Thread-safe with mutex)
│
├─→ Dashboard/Main Loop
│   │
│   └─→ MetricsTilePanel::UpdateAllMetrics()
│       │
│       ├─→ For each value in latest_values_:
│       │   ├─ Parse: "NodeName::metric_name"
│       │   ├─ Find: NodeMetricsTile[NodeName]
│       │   └─ Call: NodeMetricsTile::UpdateMetricValue(metric_name, value)
│       │
│       └─→ (Legacy) Also updates individual tiles for backward compat
│
├─→ MetricsTilePanel::Render()
│   │
│   ├─ If NodeMetricsTiles exist:
│   │  └─ For each (node_name, node_tile) in node_tiles_:
│   │     └─ Append: node_tile->Render()
│   │
│   └─ Else (Legacy Mode):
│   │  └─ For each node:
│   │     └─ Render 3-column grid of MetricsTileWindow objects
│
└─→ FTXUI Rendering Engine
    └─→ Display on terminal UI
```

## Phase 2 Method Reference

### New Methods (Phase 2)

#### AddNodeMetrics()
```cpp
void AddNodeMetrics(
    const std::string& node_name,
    const std::vector<MetricDescriptor>& descriptors,
    const nlohmann::json& metrics_schema);
```
- Creates a consolidated NodeMetricsTile for all metrics from a node
- Takes vector of MetricDescriptor objects and schema JSON
- Stores in node_tiles_[node_name]
- Accumulates total_field_count_

#### UpdateAllMetrics()
```cpp
void UpdateAllMetrics();
```
- Called each frame from main rendering loop
- Routes all values in latest_values_ to appropriate NodeMetricsTiles
- Uses ParseMetricId() to split "NodeName::metric_name" format
- Also updates legacy tiles for backward compatibility

#### SetLatestValue()
```cpp
void SetLatestValue(
    const std::string& metric_id,
    double value);
```
- Thread-safe storage of latest metric values
- Called from MetricsCapability callbacks
- Protected by values_lock_ mutex
- Format: metric_id = "NodeName::metric_name"

#### GetNodeTile()
```cpp
std::shared_ptr<NodeMetricsTile> GetNodeTile(
    const std::string& node_name) const;
```
- Retrieve consolidated tile for a node
- Returns nullptr if node not found
- Used by Phase 2 code and potential UI interaction

### Updated Methods

#### Render()
```cpp
ftxui::Element MetricsTilePanel::Render() const;
```
- **Priority 1:** If node_tiles_ is not empty, render consolidated NodeMetricsTiles
  - Shows tiles grouped by node with clear separators
  - Each tile rendered with node->Render()
  - Best for organized layout
  
- **Priority 2:** Else if nodes_ has content, render legacy 3-column grid
  - Maintains backward compatibility
  - Uses existing MetricsTileWindow rendering
  
- **Priority 3:** Show "No metrics" message if both are empty

### Legacy Methods (Backward Compatibility)

#### AddTile()
```cpp
void AddTile(std::shared_ptr<MetricsTileWindow> tile);
```
- Adds individual MetricsTileWindow to legacy storage
- Parses metric_id into node_name and metric_name
- Creates Node if needed
- Still fully functional for legacy code

#### GetTile()
```cpp
std::shared_ptr<MetricsTileWindow> GetTile(
    const std::string& metric_id) const;
```
- Retrieve individual tile from legacy storage
- Returns nullptr if not found

#### GetTilesForNode()
```cpp
std::vector<std::shared_ptr<MetricsTileWindow>> GetTilesForNode(
    const std::string& node_name) const;
```
- Get all tiles from a specific node (legacy)
- Returns empty vector if node not found

### Utility Methods

#### ParseMetricId()
```cpp
static std::pair<std::string, std::string> ParseMetricId(
    const std::string& metric_id);
```
- Splits metric_id: "NodeName::metric_name" → ("NodeName", "metric_name")
- Returns (metric_id, "") if "::" not found
- Used by AddTile and UpdateAllMetrics

#### GetNodeCount()
```cpp
size_t GetNodeCount() const;
```
- Returns number of consolidated NodeMetricsTiles
- Replaces GetTileCount() for Phase 2

#### GetTotalFieldCount()
```cpp
size_t GetTotalFieldCount() const;
```
- Returns total field count across all nodes
- Sum of all MetricDescriptor counts across all NodeMetricsTiles

## Thread Safety

### Synchronization Strategy
```
latest_values_ map
    ↓
    Protected by: values_lock_ (std::mutex)
    
    SetLatestValue() ─→ Lock → Update → Unlock
                       (From metrics callbacks)
    
    UpdateAllMetrics() ─→ Lock → Read snapshot → Unlock
                         (From main loop - infrequent lock hold)
```

- **Lock Duration:** Minimal - snapshot copy of latest_values_
- **Contention:** Low - metric callbacks vs. main loop thread
- **Guarantee:** All metric updates are visible to UpdateAllMetrics()

## Test Coverage Summary

| Test Case | Category | Status |
|-----------|----------|--------|
| AddNodeMetrics_CreatesConsolidatedTile | Phase 2 | ✅ PASS |
| AddNodeMetrics_FieldCountAccuracy | Phase 2 | ✅ PASS |
| SetLatestValue_ThreadSafe | Phase 2 | ✅ PASS |
| UpdateAllMetrics_RoutesValuesToCorrectNodes | Phase 2 | ✅ PASS |
| Render_WithConsolidatedTiles | Phase 2 | ✅ PASS |
| Render_WithoutTiles_ShowsNoMetrics | Phase 2 | ✅ PASS |
| GetNodeTile_ReturnsCorrectTile | Phase 2 | ✅ PASS |
| GetNodeTile_ReturnsNullForMissingNode | Phase 2 | ✅ PASS |
| MultipleNodeMetrics_FullWorkflow | Integration | ✅ PASS |
| UpdateLatestValueMultipleTimes | Integration | ✅ PASS |
| ParseMetricId_CorrectlySplitsNodeAndMetric | Compat | ✅ PASS |
| AddTile_LegacyBackwardCompatibility | Compat | ✅ PASS |
| GetTileCount_ReflectsNodeCount | Compat | ✅ PASS |

**Total: 13/13 Tests Passing (100%)**

## Migration Path: From Legacy to Phase 2

### Current State (After Phase 2)
```cpp
// Phase 2 Recommended Approach
MetricsTilePanel panel;

// Add metrics from schema
auto descriptors = CreateDescriptorsFromSchema(schema);
panel.AddNodeMetrics("ProcessingNode", descriptors, schema);

// Updates happen automatically via SetLatestValue + UpdateAllMetrics
panel.SetLatestValue("ProcessingNode::cpu", 75.5);
panel.UpdateAllMetrics();

// Render uses consolidated tiles
auto element = panel.Render();
```

### Legacy Approach (Still Supported)
```cpp
// Old approach - still works
MetricsTilePanel panel;

auto tile = std::make_shared<MetricsTileWindow>(descriptor, field_spec);
panel.AddTile(tile);

auto retrieved = panel.GetTile("NodeName::metric");
```

### Future State (Phase 3)
```cpp
// Will remove legacy methods in Phase 3
// All code migrated to AddNodeMetrics/UpdateAllMetrics pattern
```

## Performance Characteristics

| Aspect | Phase 2 | Benefit |
|--------|---------|---------|
| **Memory per node** | 1 NodeMetricsTile | Multiple TileWindows → Single consolidated tile |
| **Update complexity** | O(n) fields/node | O(1) per value route (map lookup) |
| **Render complexity** | O(n) nodes | O(1) per node (consolidated render) |
| **Lock contention** | Minimal snapshot | Brief lock during UpdateAllMetrics |
| **Backward compat** | 100% | All legacy methods functional |

## Next Steps (Phase 3)

1. **Remove Legacy Methods** - Once all callers migrated
2. **Simplify Storage** - Remove legacy nodes_ vector
3. **Performance Optimization** - Pre-allocate NodeMetricsTiles
4. **Enhanced Features** - Per-node filtering, export, etc.
