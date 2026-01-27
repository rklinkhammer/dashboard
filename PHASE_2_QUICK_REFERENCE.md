# Phase 2 Runtime Quick Reference

## When You Run `gdashboard`

### What You'll See in the Metrics Panel

**CONSOLIDATED NODE VIEW** (Phase 2 - NEW)
```
Each node displays as one unified section:

┌─ Producer ─────────────────────────────────┐
│ throughput_hz:  523.4 hz   [████████░░]   │ → Green = OK
│ latency_ms:      48.2 ms   [███░░░░░░░]   │ → Green = OK  
│ error_count:      0.0 ct   [░░░░░░░░░░]   │ → Green = OK
└────────────────────────────────────────────┘

┌─ Transformer ──────────────────────────────┐
│ throughput_hz:  412.1 hz   [█████░░░░░]   │ → Green = OK
│ latency_ms:      62.5 ms   [████████░░]   │ → Yellow = WARNING
│ error_count:      1.0 ct   [█░░░░░░░░░]   │ → Green = OK
└────────────────────────────────────────────┘

┌─ Consumer ─────────────────────────────────┐
│ throughput_hz:  301.7 hz   [███░░░░░░░]   │ → Green = OK
│ latency_ms:      89.3 ms   [██████████]   │ → Red = CRITICAL
│ error_count:      5.0 ct   [██░░░░░░░░]   │ → Yellow = WARNING
└────────────────────────────────────────────┘
```

### How It Updates

1. **Metrics arrive** → SetLatestValue() stores them (thread-safe)
2. **Each frame** → UpdateAllMetrics() routes values to nodes
3. **Render** → NodeMetricsTile displays updated values
4. **Colors update** → Based on value thresholds

### Status Colors

| Color | Status | Meaning |
|-------|--------|---------|
| 🟢 Green | OK | Value is in normal range |
| 🟡 Yellow | WARNING | Value approaching limit |
| 🔴 Red | CRITICAL | Value exceeds warning threshold |

### What's Different from Phase 1?

| Aspect | Phase 1 | Phase 2 |
|--------|---------|---------|
| **Organization** | Scattered 3-column grid | Grouped by node |
| **Storage** | Multiple MetricsTileWindow per node | 1 NodeMetricsTile per node |
| **Update Method** | Individual tile updates | Centralized UpdateAllMetrics() |
| **Thread Safety** | Implicit | Explicit mutex + SetLatestValue() |
| **Visual Hierarchy** | Flat | Node-based |

## Data Flow Behind the Scenes

```
Application generates metrics
           ↓
MetricsCapability event fired
           ↓
SetLatestValue("NodeName::metric", value)  ← Thread-safe storage
           ↓
Main loop calls UpdateAllMetrics()
           ↓
Parse "NodeName::metric" format
           ↓
Route to NodeMetricsTile[NodeName]
           ↓
Render() displays consolidated tile
           ↓
Terminal shows organized node layout
```

## Key Phase 2 Methods in Action

### SetLatestValue() - Thread-Safe Storage
```cpp
// Called from metrics callback threads
panel.SetLatestValue("Producer::throughput_hz", 523.4);
panel.SetLatestValue("Producer::latency_ms", 48.2);
// Values stored in thread-safe map, protected by mutex
```

### UpdateAllMetrics() - Value Routing
```cpp
// Called each frame from main loop
panel.UpdateAllMetrics();
// Internally:
// 1. Lock latest_values_ map
// 2. For each "NodeName::metric" → "NodeName", "metric"
// 3. Find NodeMetricsTile["NodeName"]
// 4. Call UpdateMetricValue("metric", value)
// 5. Unlock
```

### Render() - Smart Display
```cpp
element = panel.Render();
// Checks in order:
// 1. If node_tiles_ exists → Render consolidated NodeMetricsTiles ✨ (Phase 2)
// 2. Else if legacy tiles exist → Render 3-column grid (Phase 1 compat)
// 3. Else → Show "No metrics" message
```

## Logging Indicators

**Watch for these messages to confirm Phase 2 is working:**

```
✅ Good - Consolidated rendering:
   [TRACE] MetricsTilePanel: Added NodeMetricsTile: Producer with 3 fields
   [TRACE] MetricsTilePanel: Added NodeMetricsTile: Transformer with 3 fields
   [TRACE] MetricsTilePanel: UpdateAllMetrics: Routed 9 values to 3 nodes

⚠️ Fallback - Legacy rendering (still works, but Phase 2 prefers consolidated):
   [TRACE] MetricsTilePanel: Added tile (legacy): Producer::throughput_hz
   [TRACE] Rendered using legacy 3-column grid
```

## Performance

- **Memory**: 1 NodeMetricsTile per node (consolidation saves memory)
- **Updates**: O(n) metrics, O(1) per value route (map lookup)
- **Rendering**: O(n) nodes, each renders efficiently
- **Thread Safety**: Minimal lock hold time (snapshot pattern)
- **FPS**: Typically 30-60 FPS in terminal UI (smooth updates)

## Next Phase (Phase 3)

Phase 2 sets the foundation for Phase 3, which will:
- Remove legacy AddTile() method
- Add per-node filtering/search
- Add node collapse/expand
- Add export capabilities

Until then, Phase 2 maintains **100% backward compatibility** with Phase 1 code.

---

**TL;DR**: When you run gdashboard after Phase 2, you'll see metrics organized by node instead of scattered tiles. Each node has a consolidated view of all its metrics, with live updates, color-coded status, and smooth animations. Everything is thread-safe and efficient under the hood.
