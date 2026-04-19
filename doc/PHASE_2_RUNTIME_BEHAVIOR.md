# Phase 2 Runtime Behavior - What You Should See

## Overview
After completing Phase 2, when you execute `gdashboard`, the metrics panel will display metrics organized by **node**, with each node having a consolidated tile containing all its metrics in a structured, organized layout.

## Visual Layout

### Terminal UI Structure
```
┌────────────────────────────────────────────────────────────────────────────┐
│ DASHBOARD                                                                  │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  [METRICS PANEL]  (50% of screen)                                         │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │ Producer ◄──────── Node Consolidation (Phase 2) ────────────────►│    │
│  │ ┌──────────────────────────────────────────────────────────────┐ │    │
│  │ │ throughput_hz:  523.4 hz  [████████████░░░░░] OK (Green)     │ │    │
│  │ │ latency_ms:      48.2 ms  [██████████░░░░░░░] OK (Green)     │ │    │
│  │ │ error_count:      0.0 ct  [░░░░░░░░░░░░░░░░░] OK (Green)     │ │    │
│  │ └──────────────────────────────────────────────────────────────┘ │    │
│  │                                                                    │    │
│  │ Transformer (node 2)                                               │    │
│  │ ┌──────────────────────────────────────────────────────────────┐ │    │
│  │ │ throughput_hz:  412.1 hz  [██████░░░░░░░░░░░] OK (Green)     │ │    │
│  │ │ latency_ms:      62.5 ms  [████████████░░░░░] WARNING (Yel)  │ │    │
│  │ │ error_count:      1.0 ct  [██░░░░░░░░░░░░░░░] OK (Green)     │ │    │
│  │ └──────────────────────────────────────────────────────────────┘ │    │
│  │                                                                    │    │
│  │ Consumer (node 3)                                                  │    │
│  │ ┌──────────────────────────────────────────────────────────────┐ │    │
│  │ │ throughput_hz:  301.7 hz  [████░░░░░░░░░░░░░░] OK (Green)     │ │    │
│  │ │ latency_ms:      89.3 ms  [██████████████░░░░] CRITICAL (Red) │ │    │
│  │ │ error_count:      5.0 ct  [██████░░░░░░░░░░░░] WARNING (Yel)  │ │    │
│  │ └──────────────────────────────────────────────────────────────┘ │    │
│  │                                                                    │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                            │
├────────────────────────────────────────────────────────────────────────────┤
│  [LOGGING WINDOW] (35% of screen)                                          │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │ [INFO] Dashboard initialized                                      │    │
│  │ [TRACE] MetricsPanel discovered 3 nodes with metrics             │    │
│  │ [DEBUG] Metrics updated: Producer metrics refreshed             │    │
│  │ [DEBUG] Metrics updated: Transformer metrics refreshed          │    │
│  │ [DEBUG] Metrics updated: Consumer metrics refreshed             │    │
│  │ [TRACE] UpdateAllMetrics: Routed 9 values to 3 nodes           │    │
│  │                                                                   │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                            │
├────────────────────────────────────────────────────────────────────────────┤
│  [COMMAND WINDOW] (15% of screen)                                          │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │ >> help                                                           │    │
│  │ Available commands: help, exit, metrics, logs                    │    │
│  │                                                                   │    │
│  └──────────────────────────────────────────────────────────────────┘    │
├────────────────────────────────────────────────────────────────────────────┤
│ [STATUS BAR] Ready | Nodes: 3 | Metrics: 9 | Updates: 45/sec    F1: Info │
└────────────────────────────────────────────────────────────────────────────┘
```

## Key Visual Features (Phase 2)

### 1. Node-Based Organization ✨ NEW
- **Each node is clearly labeled** at the top of its section
- Example: `Producer`, `Transformer`, `Consumer`
- Nodes are visually separated with borders

### 2. Consolidated Metric Display ✨ NEW
- **All metrics for a node grouped together** in one tile
- No longer scattered individual tiles
- Clean, hierarchical view

### 3. Per-Metric Display
Each metric shows:
- **Name**: Metric identifier (e.g., `throughput_hz`, `latency_ms`)
- **Value**: Current numeric value (e.g., `523.4`)
- **Unit**: Measurement unit (e.g., `hz`, `ms`, `ct`)
- **Gauge/Sparkline**: Visual representation of value in range
- **Status Color**: 
  - 🟢 **Green** - OK status (value in normal range)
  - 🟡 **Yellow** - WARNING status (value in warning range)
  - 🔴 **Red** - CRITICAL status (value exceeds warning threshold)

### 4. Dynamic Updates
- Metrics update **every frame** (typically 30-60 FPS in terminal)
- Values change as the application runs
- Gauges and sparklines animate smoothly
- Status colors update based on threshold changes

## Expected Behavior Timeline

### On Startup
```
1. Dashboard initializes
2. MetricsPanel discovers metrics from MetricsCapability
3. Phase 2 creates NodeMetricsTile for each node:
   - Producer gets 1 NodeMetricsTile (3 fields)
   - Transformer gets 1 NodeMetricsTile (3 fields)
   - Consumer gets 1 NodeMetricsTile (3 fields)
4. Logging window shows:
   [TRACE] MetricsPanel discovered 3 nodes with metrics
   [TRACE] Created NodeMetricsTile: Producer with 3 fields
   [TRACE] Created NodeMetricsTile: Transformer with 3 fields
   [TRACE] Created NodeMetricsTile: Consumer with 3 fields
```

### During Execution
```
1. Application generates metric values (throughput, latency, errors, etc.)
2. Metrics events are received via MetricsCapability callback
3. Phase 2 SetLatestValue() stores values thread-safely:
   - SetLatestValue("Producer::throughput_hz", 523.4)
   - SetLatestValue("Producer::latency_ms", 48.2)
   - SetLatestValue("Producer::error_count", 0.0)
   - ... (same for other nodes)
4. Main render loop calls UpdateAllMetrics():
   - Parses metric IDs ("Producer::throughput_hz" → "Producer", "throughput_hz")
   - Routes values to NodeMetricsTile["Producer"]
   - NodeMetricsTile updates its internal metric values
5. Render() is called:
   - Prefers consolidated NodeMetricsTiles (Phase 2 new behavior)
   - Each NodeMetricsTile renders all its metrics
   - Visual display updates smoothly
6. Logging shows periodic updates:
   [TRACE] UpdateAllMetrics: Routed 9 values to 3 nodes
   [TRACE] Producer metrics updated
   [TRACE] Transformer metrics updated
   [TRACE] Consumer metrics updated
```

### Color Status Examples

**Green (OK)** - All normal
```
throughput_hz: 523.4 hz [████████████░░░░░░] OK
```

**Yellow (WARNING)** - Approaching limit
```
latency_ms: 62.5 ms [████████████░░░░░░] WARNING (70ms threshold)
```

**Red (CRITICAL)** - Exceeding warning
```
latency_ms: 89.3 ms [██████████████░░░░] CRITICAL (>80ms)
```

## Phase 2 vs Phase 1 Comparison

### Phase 1 (Before)
```
┌─ MetricsTilePanel ──────────┐
│                             │
│  [throughput_hz] [latency]  │  Multiple scattered tiles
│  [error_count]   [queue]    │  in a 3-column grid
│  [status_a]      [status_b] │  No clear node grouping
│                             │
└─────────────────────────────┘
```

### Phase 2 (After) ✨
```
┌─ MetricsTilePanel ─────────────────────────────┐
│                                               │
│ Producer Node (Consolidated)                 │
│ ┌─────────────────────────────────────────┐  │
│ │ [throughput_hz] [latency_ms]           │  │
│ │ [error_count] [queue_depth]           │  │
│ │ ... (all Producer metrics together)    │  │
│ └─────────────────────────────────────────┘  │
│                                               │
│ Transformer Node (Consolidated)              │
│ ┌─────────────────────────────────────────┐  │
│ │ [throughput_hz] [latency_ms]           │  │
│ │ [error_count] ...                       │  │
│ └─────────────────────────────────────────┘  │
│                                               │
│ Consumer Node (Consolidated)                 │
│ ┌─────────────────────────────────────────┐  │
│ │ [throughput_hz] [latency_ms]           │  │
│ │ [error_count] ...                       │  │
│ └─────────────────────────────────────────┘  │
│                                               │
└────────────────────────────────────────────────┘
```

## Keyboard Controls
- **Arrow Left/Right**: Switch between metric tabs (if tab mode enabled)
- **F1**: Toggle debug overlay
- **q**: Quit dashboard
- **Type commands**: Enter commands in command window

## Logging Output
Watch the logging window for Phase 2 behavior confirmation:

✅ **Expected messages**:
```
[TRACE] MetricsTilePanel: Created with NodeMetricsTile consolidation
[TRACE] Added NodeMetricsTile: Producer with 3 fields
[TRACE] Added NodeMetricsTile: Transformer with 3 fields
[TRACE] Added NodeMetricsTile: Consumer with 3 fields
[TRACE] UpdateAllMetrics: Routed 9 values to 3 nodes
```

❌ **Legacy messages (Phase 1)** - Should NOT see:
```
[TRACE] Added tile (legacy): Producer::throughput_hz
[TRACE] Rendered 9 individual tiles
```

## Testing the Phase 2 Implementation

To verify Phase 2 is working correctly, check:

1. **Node Organization** - Can you see metrics grouped by node?
2. **Metric Values** - Do values display and update?
3. **Status Colors** - Do colors change (green→yellow→red) with values?
4. **Performance** - Is the UI responsive and smooth?
5. **Thread Safety** - Do rapid value updates work without crashes?
6. **Logging** - Do you see UpdateAllMetrics routing messages?

## Troubleshooting

**Seeing old grid layout instead of consolidated nodes?**
- Make sure MetricsCapability is passing data to SetLatestValue()
- Check that UpdateAllMetrics() is being called each frame
- Verify logging for "Routed N values to M nodes" messages

**Metrics not updating?**
- Ensure MetricsCapability callback is registered
- Check that metric_id format is "NodeName::metric_name"
- Verify thread-safe SetLatestValue() is being called

**Incorrect colors or statuses?**
- Check alert_rule thresholds in metric schema JSON
- Verify MetricDescriptor min/max values are correct
- Look for warning/critical threshold crossings in logs

## Summary

Phase 2 makes the metrics display **node-aware** and **consolidated**. Instead of scattered individual tiles, you get a **hierarchical view where each node is a first-class entity** with all its metrics grouped together. This provides better organization, clearer data relationships, and improved visual hierarchy.
