# Phase 3: MetricsTilePanel Enhancement & Cleanup Plan

## Overview
Phase 3 builds on Phase 2's successful consolidation to add new features and clean up legacy code.

## Phase 3 Goals

### 1. Legacy Code Cleanup
Remove deprecated Phase 1 methods now that Phase 2 is integrated:
- [ ] Remove `AddTile()` method
- [ ] Remove `GetTile()` method  
- [ ] Remove `GetTilesForNode()` method
- [ ] Remove `ParseMetricId()` helper (no longer needed)
- [ ] Remove legacy `Node` struct and `nodes_` vector storage
- [ ] Remove `node_index_` map

**Impact**: Simplifies codebase, reduces memory, eliminates dual-storage complexity

### 2. New Features

#### 2.1 Per-Node Filtering & Search
- [ ] Add `FilterNodes(predicate)` method
- [ ] Add `SearchMetrics(query)` to find metrics by name
- [ ] Add `GetNodesForStatus(status)` to find nodes with warnings/criticals
- [ ] Add visibility state per node

**Use Cases**:
- Hide nodes that are OK, show only WARNING/CRITICAL
- Search for specific metric "latency" across all nodes
- Find all nodes with errors

#### 2.2 Node Collapse/Expand
- [ ] Add `IsNodeExpanded(node_name)` state
- [ ] Add `ToggleNodeExpanded(node_name)` method
- [ ] Modify `Render()` to respect expanded/collapsed state
- [ ] Add UI indicators (▼ expanded / ▶ collapsed)

**Use Cases**:
- Hide metrics from healthy nodes to focus on problems
- Expand only nodes you're interested in
- Cleaner UI when many nodes exist

#### 2.3 Node Sorting & Organization
- [ ] Add `SortNodes(comparator)` method
- [ ] Add sorting modes: by name, by status (critical first), by field count
- [ ] Add pinning: keep certain nodes at top

**Use Cases**:
- Critical nodes at top
- Alphabetical order
- Recently updated nodes first

#### 2.4 Export & Reporting
- [ ] Add `ExportAsJSON()` - export all metrics as JSON
- [ ] Add `ExportAsCSV()` - time-series data for analysis
- [ ] Add `GetMetricHistory()` - sparkline data
- [ ] Add `GetNodeSummary()` - status summary per node

**Use Cases**:
- Save metrics for analysis
- Generate reports
- Feed data to external systems

#### 2.5 Enhanced Rendering
- [ ] Add compact mode (hide status, just values)
- [ ] Add detailed mode (show thresholds, history)
- [ ] Add threshold visualization
- [ ] Add metric delta (change since last update)

### 3. API Improvements

#### Current Public API (Phase 2):
```cpp
void AddNodeMetrics(...)
void UpdateAllMetrics()
void SetLatestValue(...)
std::shared_ptr<NodeMetricsTile> GetNodeTile(...)
size_t GetNodeCount()
size_t GetTotalFieldCount()
ftxui::Element Render()
```

#### Phase 3 Additions:
```cpp
// Filtering & Search
std::vector<std::string> GetNodeNames() const
std::vector<std::string> SearchMetrics(const std::string& query) const
std::vector<std::string> GetNodesByStatus(AlertStatus status) const
void SetNodeVisible(const std::string& node_name, bool visible)

// Expand/Collapse
void ToggleNodeExpanded(const std::string& node_name)
bool IsNodeExpanded(const std::string& node_name) const

// Sorting
void SortNodes(SortMode mode)
void PinNode(const std::string& node_name)
void UnpinNode(const std::string& node_name)

// Export
nlohmann::json ExportAsJSON() const
std::string ExportAsCSV() const
std::vector<double> GetMetricHistory(const std::string& metric_id) const

// Rendering options
void SetRenderMode(RenderMode mode)
void SetShowThresholds(bool show)
void SetShowDelta(bool show)
```

## Implementation Phases

### Phase 3a: Cleanup (1-2 days)
1. Remove all legacy methods
2. Remove dual-storage structures
3. Update tests (remove legacy tests)
4. Verify integration still works

### Phase 3b: Core Features (2-3 days)
1. Implement filtering & search
2. Implement collapse/expand state
3. Add comprehensive tests
4. Update Render() to support new features

### Phase 3c: Advanced Features (2-3 days)
1. Implement export functionality
2. Implement sorting & pinning
3. Add rendering modes
4. Performance optimization

### Phase 3d: Documentation & Testing (1-2 days)
1. Comprehensive test coverage for all new features
2. Update documentation
3. Create usage examples
4. Performance benchmarks

## Technical Decisions

### Storage Structure (Phase 3)
Remove dual-storage, keep only Phase 2:
```cpp
// ONLY Phase 2 storage:
std::map<std::string, std::shared_ptr<NodeMetricsTile>> node_tiles_;

// NEW state storage:
std::set<std::string> visible_nodes_;
std::set<std::string> expanded_nodes_;
std::set<std::string> pinned_nodes_;
std::vector<std::pair<std::string, double>> metric_history_;  // For sparklines
```

### Thread Safety
Maintain Phase 2's thread-safe patterns:
- `SetLatestValue()` - protected by `values_lock_`
- `UpdateAllMetrics()` - snapshot pattern
- State changes (expand/collapse) - new `state_lock_` for UI state

### Backward Compatibility
- Remove legacy methods entirely (clean break)
- Existing callers already using Phase 2 `AddNodeMetrics()`
- No compatibility concerns

## Acceptance Criteria

### Cleanup Complete
- [ ] All legacy methods removed
- [ ] Dual-storage eliminated
- [ ] No "AddTile" or "GetTile" references remain
- [ ] Tests still pass
- [ ] Code is simpler and faster

### New Features Working
- [ ] Filtering shows/hides nodes correctly
- [ ] Search finds metrics across nodes
- [ ] Collapse/expand works smoothly
- [ ] Export produces valid JSON/CSV
- [ ] Sorting works with all modes

### Quality Metrics
- [ ] No performance regression
- [ ] All tests passing (30+ tests)
- [ ] Thread-safe under concurrent access
- [ ] Memory usage improved (removed dual storage)
- [ ] Documentation complete

## Risk Assessment

### Low Risk
- Removing unused legacy code
- Adding new filtering methods
- UI state management

### Medium Risk
- Changing internal storage structure
- Export functionality (new code paths)
- Rendering mode changes (visual impact)

### Mitigation
- Comprehensive testing at each step
- Incremental rollout (cleanup → features)
- Regression testing with Phase 2 integration
- Manual testing with gdashboard

## Timeline

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| 3a (Cleanup) | 1-2 days | Simplified codebase, all legacy removed |
| 3b (Core Features) | 2-3 days | Filtering, search, collapse/expand |
| 3c (Advanced) | 2-3 days | Export, sorting, rendering modes |
| 3d (Testing & Docs) | 1-2 days | Complete test coverage, documentation |
| **Total** | **6-10 days** | **Production-ready Phase 3** |

## File Modifications

### Core Files
- `include/ui/MetricsTilePanel.hpp` - Add new methods and state
- `src/ui/MetricsTilePanel.cpp` - Implement Phase 3 functionality
- `test/ui/test_metrics_tile_panel.cpp` - Comprehensive testing

### Potentially Affected
- `src/ui/MetricsPanel.cpp` - May need updates for new features
- `src/ui/Dashboard.cpp` - May need updates for new API

## Success Metrics

1. **Code Quality**: Simpler, cleaner implementation
2. **Performance**: Faster rendering without dual-storage
3. **Features**: All planned features implemented
4. **Testing**: 90%+ code coverage
5. **User Experience**: Intuitive filtering, searching, organizing

---

**Ready to begin Phase 3!** 🚀

Next step: Start with Phase 3a (Cleanup) - Remove legacy methods.
