# Phase 8-11 Implementation Priorities and Timeline

**Analysis Date**: February 6, 2026  
**Planning Horizon**: 6 months (Q1-Q2 2026)  
**Team Size**: 1-2 developers  
**Total Estimated Effort**: 18-22 weeks

---

## Executive Priorities (First 90 Days)

### Phase 8: Advanced Metrics Analysis (Weeks 1-4)

**Status**: Ready to start  
**Expected Completion**: Early March 2026  
**Resource Requirement**: 1 developer full-time  

#### 8.1: Multi-Metric Comparison (5-6 days)

**Objective**: Side-by-side visualization of related metrics

**Implementation Steps**:
1. Create MetricsComparisonWindow class (200 lines)
2. Add comparison command to CommandRegistry
3. Implement correlation coefficient calculation
4. Build min/max/avg statistics display
5. Write 15+ unit tests

**Files to Create**:
- `include/ui/MetricsComparisonWindow.hpp`
- `src/ui/MetricsComparisonWindow.cpp`
- `test/ui/test_metrics_comparison.cpp`

**Integration Points**:
- CommandRegistry: Add `compare_metrics` command
- Dashboard: New tab for comparison view
- MetricsPanel: Expose metric history access

**Definition of Done**:
- All comparison tests passing
- Can compare 2-5 metrics simultaneously
- Renders within FTXUI time budget (< 5ms)
- Help text includes examples

---

#### 8.2: CSV Export (4-5 days)

**Objective**: Export metric history for external analysis

**Implementation Steps**:
1. Create CSVExporter class (250 lines)
2. Add export command to CommandRegistry
3. Implement header generation
4. Build record formatting with precision
5. Write 12+ unit tests

**Files to Create**:
- `include/app/export/CSVExporter.hpp`
- `src/app/export/CSVExporter.cpp`
- `test/app/test_csv_exporter.cpp`

**CSV Format**:
```csv
timestamp,node,metric,value,min,max,alert_level
2026-02-06T14:32:01Z,EstimationPipelineNode,altitude_m,1520.5,1510.2,1530.8,OK
```

**Definition of Done**:
- Exports validate with standard CSV tools
- Time range filtering works
- Precision matches metric schema
- All export tests passing

---

#### 8.3: Performance Statistics (4-5 days)

**Objective**: Real-time profiling of dashboard and executor

**Implementation Steps**:
1. Create PerformanceMonitor class (300 lines)
2. Integrate frame timing in Dashboard::Run()
3. Add latency histogram to MetricsPanel
4. Build stats display command
5. Write 18+ unit tests

**Files to Create**:
- `include/app/profiling/PerformanceMonitor.hpp`
- `src/app/profiling/PerformanceMonitor.cpp`
- `test/app/test_performance_monitor.cpp`

**Metrics Tracked**:
- Dashboard FPS (target: 30.0)
- Frame render time (target: < 33ms)
- Executor throughput (events/sec)
- Latency percentiles (p50, p95, p99)
- Memory usage (MB)
- CPU usage (%)

**Definition of Done**:
- Stats command shows live metrics
- Performance overhead < 1% impact
- All profiling tests passing
- Documentation includes interpretation guide

---

### Phase 8 Quality Gates

**Build Status**: ✅ Must pass  
**Test Status**: 40+ new tests all passing  
**Code Coverage**: No decrease from 85%+  
**Performance**: No regression in 30 FPS target  
**Documentation**: Updated with Phase 8 features  

---

## Implementation Roadmap (6 Months)

### Timeline Overview

```
Q1 2026                          Q2 2026
├─ Weeks 1-4: Phase 8 (Analysis)
├─ Weeks 5-9: Phase 9 (Control)
├─ Weeks 10-13: Phase 10 (Alerting)
│                    │
│                    └─ Weeks 10-14: Arch Improvements
└─ Weeks 14-18: Phase 11 (Plugins)

   Development         QA/Review
   ▓▓▓▓▓▓             ░░░░░░
```

---

## Phase 8: Detailed Task Breakdown

### Task 8.1.1: MetricsComparisonWindow Class

**Estimated Time**: 2 days  
**Complexity**: MEDIUM

```cpp
// Header: include/ui/MetricsComparisonWindow.hpp
class MetricsComparisonWindow {
public:
    MetricsComparisonWindow(
        MetricsPanel* metrics_panel,
        const std::vector<std::string>& metric_names);
    
    // Get comparison data
    struct ComparisonData {
        std::string metric_name;
        double current_value;
        double min_value;
        double max_value;
        double avg_value;
        double stddev;
        std::vector<double> history;
    };
    
    std::vector<ComparisonData> GetComparisonData() const;
    
    // Compute correlation between two metrics
    double GetCorrelation(const std::string& metric1,
                         const std::string& metric2) const;
    
    // FTXUI rendering
    ftxui::Element Render() const;
};
```

**Success Criteria**:
- Compares 2-5 metrics
- Correlation accurate to 0.01
- Renders in < 5ms

---

### Task 8.1.2: compare_metrics Command

**Estimated Time**: 1 day  
**Complexity**: LOW

```cpp
// In CommandRegistry::RegisterBuiltinCommands()
registry->RegisterCommand(
    "compare_metrics",
    "Compare metrics side-by-side",
    "compare_metrics <metric1> [metric2] [metric3] ...",
    [app](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            return CommandResult(false, "At least 2 metrics required");
        }
        
        std::vector<std::string> metrics(
            args.begin() + 1, args.end());
        
        app->ShowMetricsComparison(metrics);
        return CommandResult(true, 
            "Comparing " + std::to_string(metrics.size()) + 
            " metrics");
    }
);
```

---

### Task 8.2.1: CSVExporter Class

**Estimated Time**: 2 days  
**Complexity**: MEDIUM

```cpp
// Header: include/app/export/CSVExporter.hpp
class CSVExporter {
public:
    struct ExportOptions {
        std::vector<std::string> metric_patterns;  // Which metrics
        std::optional<uint64_t> since_ms;          // Time filter
        std::string output_filename;
        bool include_statistics = true;
        int decimal_precision = 2;
    };
    
    bool ExportMetrics(
        MetricsPanel* panel,
        const ExportOptions& options,
        std::string& error_message);
    
private:
    std::string GenerateCSVHeader() const;
    std::string FormatRecord(
        const std::string& metric_name,
        const Metric& metric) const;
};
```

**CSV Header**:
```
timestamp,node_name,metric_name,value,min,max,avg,stddev,alert_level
```

---

### Task 8.3.1: PerformanceMonitor Class

**Estimated Time**: 2 days  
**Complexity**: MEDIUM

```cpp
// Header: include/app/profiling/PerformanceMonitor.hpp
class PerformanceMonitor {
public:
    struct Statistics {
        double current_fps;
        double avg_fps;
        uint64_t frame_time_ms;
        double executor_throughput;  // events/sec
        LatencyHistogram latency;    // p50, p95, p99
        double memory_usage_mb;
        double cpu_usage_percent;
    };
    
    void RecordFrameStart();
    void RecordFrameEnd();
    void RecordExecutorEvent(uint64_t latency_us);
    
    Statistics GetStatistics() const;
    std::string GenerateReport() const;
};
```

**Integration in Dashboard::Run()**:
```cpp
void Dashboard::Run() {
    while (running_) {
        perf_monitor_.RecordFrameStart();
        
        // Handle events, update metrics
        screen_.Print(ftxui::Screen(...));
        
        perf_monitor_.RecordFrameEnd();  // Measures frame time
    }
}
```

---

## Phase 9: Advanced Control (4-5 weeks)

### 9.1: Data Injection Interface

**Objective**: Send custom data to graph nodes during execution

**Estimated Time**: 5-6 days  
**Complexity**: HIGH (integration with executor)

**Key Requirements**:
- Query available injection ports
- Build messages against schema
- Send with validation
- Transaction support (all-or-nothing)
- Feedback on delivery

**Files to Create**:
- `include/ui/DataInjectionWindow.hpp`
- `src/ui/DataInjectionWindow.cpp`
- `test/ui/test_data_injection.cpp`

**Example Usage**:
```bash
list_injection_ports
# Output: gps_position, imu_data, magnetometer

inject gps_position --latitude 40.7128 --longitude -74.0060 --altitude 25.5
# Output: "Successfully injected GPS position"

inject_batch /path/to/data.json
# Output: "Injected 100 records in 1.2 seconds"
```

**Integration Points**:
- Query executor for CSVDataInjectionCapability
- Use existing DataInjectionPort schema
- Thread-safe message sending
- Validation against port specifications

---

### 9.2: Node-Level Control

**Objective**: Pause/resume individual nodes without stopping executor

**Estimated Time**: 4-5 days  
**Complexity**: HIGH (requires executor API)

**Requirements**:
- Per-node pause/resume
- Per-node metric reset
- Per-node configuration
- Visual state indicators
- Command confirmation

**Commands**:
```bash
node_list                          # Show all nodes
node_pause EstimationPipelineNode  # Pause specific node
node_resume EstimationPipelineNode # Resume it
node_reset_metrics *               # Reset all metrics
node_config EstimationPipeline --threshold=50
```

**Integration**:
- MetricsPanel: Color-code by node state
- StatusBar: Show paused node count
- Visual indicators (pause symbol, colors)
- Requires GraphExecutor API for per-node control

---

### 9.3: Graph Topology Visualization

**Objective**: Display graph structure (nodes and edges)

**Estimated Time**: 4-5 days  
**Complexity**: MEDIUM

**Display**:
```
GRAPH TOPOLOGY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  [DataValidator] (3 metrics)
         ↓
  [Estimation Pipeline] (4 metrics)
         ├─→ [Altitude Fusion] (2 metrics)
         │       ↓
         └─→ [State Capture] (2 metrics)
                 ↓
         [Feedback Sink]

Commands: arrow keys to navigate, 'h' for highlight path
```

**Integration**:
- Query executor for GraphTopology
- Render as FTXUI component
- Interactive path highlighting
- New Dashboard tab

---

## Phase 10: Advanced Alerting (2-3 weeks)

### 10.1: Alert History and Tracking

**Objective**: Track alert state transitions over time

**Estimated Time**: 3-4 days  
**Complexity**: LOW-MEDIUM

**Features**:
- Alert transition tracking (OK → WARNING → CRITICAL)
- Time-in-state calculation
- Alert summary in StatusBar
- History window with filtering

**Display**:
```
ACTIVE ALERTS (3)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EstimationPipeline::latency  CRITICAL  2m 34s ▲
AltitudeFusionNode::conf     WARNING   45s
DataValidator::errors        WARNING   2m 12s ▲

ALERT HISTORY
14:35:12  DataValidator::errors        WARNING → OK (182s)
14:32:45  EstimationPipeline::latency  OK → CRITICAL (ongoing)
14:31:00  AltitudeFusion::confidence   OK → WARNING (ongoing)
```

---

### 10.2: Smart Threshold Learning

**Objective**: Auto-tune alert thresholds based on history

**Estimated Time**: 3-4 days  
**Complexity**: LOW-MEDIUM

**Algorithm**:
1. Collect baseline (first 100 values)
2. Compute mean and standard deviation
3. Set WARNING = mean + 1σ
4. Set CRITICAL = mean + 2σ
5. Auto-update as data arrives

**Commands**:
```bash
auto_threshold --metric altitude_* --baseline 100
auto_threshold --disable all
threshold_summary  # Show current thresholds
```

---

## Phase 11: Plugin System (3-4 weeks)

### 11.1: Command Plugin Interface

**Objective**: Allow third-party command extensions

**Estimated Time**: 4-5 days  
**Complexity**: MEDIUM

**Plugin Interface**:
```cpp
// include/plugins/DashboardPluginInterface.hpp
extern "C" {
    void RegisterDashboardPlugin(CommandRegistry* registry) {
        registry->RegisterCommand(
            "plugin_command",
            "Custom command from plugin",
            "plugin_command [args]",
            handler_function);
    }
}
```

**Plugin Loading**:
```bash
./gdashboard --graph-config config.json --plugins ./plugins/
```

---

### 11.2: Custom Metric Renderers

**Objective**: Define custom metric display types

**Estimated Time**: 3-4 days  
**Complexity**: MEDIUM

**Renderer Interface**:
```cpp
class IMetricRenderer {
    virtual ftxui::Element Render(const Metric& m) = 0;
    virtual bool CanRender(const std::string& display_type) = 0;
};
```

**Usage in Schema**:
```json
{
  "name": "altitude_heatmap",
  "display_type": "custom_heatmap",
  "custom_renderer": "HeatmapPlugin"
}
```

---

## Quality Assurance Strategy

### Per-Phase Testing Requirements

```
Phase 8 (Analysis)
├─ Unit tests: 40+
├─ Integration: 10+
├─ Performance: Stress test with 1000+ metrics
└─ Documentation: Updated with examples

Phase 9 (Control)
├─ Unit tests: 50+
├─ Integration: 15+
├─ End-to-end: Data injection → node response
└─ Security: Input validation, buffer overflow tests

Phase 10 (Alerting)
├─ Unit tests: 25+
├─ Integration: 10+
├─ Statistical accuracy: Threshold learning validation
└─ Regression: All Phase 4-9 tests still passing

Phase 11 (Plugins)
├─ Unit tests: 30+
├─ Integration: 20+
├─ Plugin isolation: Failure doesn't crash dashboard
└─ Compatibility: Multiple plugins coexist
```

---

## Risk Management

### Top Risks and Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Phase 9 blocks on executor API | MEDIUM | HIGH | Define API early, use adapters |
| Performance regression | LOW | HIGH | Continuous benchmarking |
| Plugin system security | MEDIUM | MEDIUM | Sandboxing, input validation |
| Real executor integration delay | MEDIUM | MEDIUM | Abstract interface ready |
| Metrics explosion (1000+) | LOW | HIGH | Phase 8 optimizations, buffering |

---

## Success Metrics

### Phase 8 Success Criteria
- ✅ 40+ new tests passing
- ✅ CSV export validates with standard tools
- ✅ Comparison accuracy > 99%
- ✅ Performance overhead < 1%
- ✅ All documentation updated

### Phase 9 Success Criteria
- ✅ 50+ new tests passing
- ✅ Data injection delivery success rate 100%
- ✅ Node pause/resume < 100ms response
- ✅ Topology renders for graphs with 50+ nodes
- ✅ Integration tests demonstrate all features

### Phase 10 Success Criteria
- ✅ 25+ new tests passing
- ✅ Alert transitions tracked with < 1ms overhead
- ✅ Learned thresholds improve alert accuracy
- ✅ Alert history persists across sessions
- ✅ No regressions in core alerting

### Phase 11 Success Criteria
- ✅ 30+ new tests passing
- ✅ Plugins load/unload cleanly
- ✅ Plugin failures isolated (no crash)
- ✅ 3+ example plugins demonstrate capabilities
- ✅ Plugin API documented with examples

---

## Maintenance and Support Plan

### Ongoing Activities

**Weekly**:
- Run full test suite
- Review and merge PRs
- Triage bug reports

**Monthly**:
- Performance profiling
- Dependency updates
- Documentation review

**Quarterly**:
- Architecture review
- Roadmap planning
- Community feedback integration

---

## Conclusion

**Phase 8-11 represent a clear path to advanced functionality** while maintaining the strong architectural foundation established in Phases 0-7. The estimated 18-22 weeks of work will position gdashboard as a comprehensive monitoring and control system for graph execution engines.

**Immediate Next Steps**:
1. ✅ Approve Phase 8 roadmap
2. ✅ Assign developer(s)
3. ✅ Create sprint planning for Week 1
4. ✅ Update CI/CD for new test categories
5. ✅ Schedule weekly progress reviews

---

**Timeline**: Q1-Q2 2026  
**Status**: ✅ READY FOR EXECUTION  
**Recommendation**: **PROCEED WITH PHASE 8** as planned

