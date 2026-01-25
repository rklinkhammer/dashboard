# Detailed Implementation Plan for gdashboard

**Date**: January 25, 2026  
**Status**: Phase 2 In Progress - Foundation Classes Complete  
**Duration Estimate**: 2-3 weeks remaining for Phase 2 metrics integration

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Technology Stack](#technology-stack)
3. [Phase Breakdown](#phase-breakdown)
4. [Detailed Phase 1: Basic Window Structure](#detailed-phase-1-basic-window-structure)
5. [Detailed Phase 2: Metrics Integration](#detailed-phase-2-metrics-integration)
6. [Detailed Phase 3: Enhanced Features](#detailed-phase-3-enhanced-features)
7. [Testing Strategy](#testing-strategy)
8. [Risk Management](#risk-management)
9. [Success Criteria](#success-criteria)

---

## Project Overview

### Objective
Build **gdashboard**, a real-time metrics visualization dashboard for graph execution engines using FTXUI 6.1.9. The dashboard displays live metrics from graph node execution in a responsive, multi-window layout.

### Key Features
- **Real-time Metrics Display**: 3-column grid layout with 4 display types (value, gauge, sparkline, state)
- **Multi-window Dashboard**: Metrics panel (40%), Logging window (35%), Command window (18%), Status bar (2%)
- **Thread-safe Updates**: MetricsPublisher thread → callbacks → tile updates → 30 FPS rendering
- **Configurable Layout**: Adjustable window heights via CLI or config file
- **Command Interface**: Extensible command registration system
- **Logging Integration**: log4cxx appender for centralized logging

### Architecture Highlights
- **No custom UI framework**: Uses FTXUI 6.1.9 components directly
- **Metrics-driven configuration**: NodeMetricsSchema defines all display characteristics
- **Callback-based updates**: MetricsPublisher thread → IMetricsSubscriber callbacks
- **No buffering in capability**: Dashboard/MetricsPanel responsible for history
- **Single initialization sequence**: 9-step ordered sequence for reliable startup

---

## Technology Stack

### Core Dependencies
| Component | Version | Purpose | Status |
|-----------|---------|---------|--------|
| **FTXUI** | 6.1.9 | Terminal UI framework (flexbox layout) | ✅ Available |
| **nlohmann/json** | v3.11+ | JSON configuration parsing | ✅ Available |
| **log4cxx** | 0.13+ | Structured logging | ✅ Available |
| **C++** | 17+ | Language standard | ✅ Required |
| **CMake** | 3.16+ | Build system | ✅ Available |

### Module Dependencies
```
gdashboard (executable)
├── graph::executor (GraphExecutor interface + MockGraphExecutor impl)
├── app::metrics (MetricsEvent, NodeMetricsSchema, IMetricsSubscriber)
├── ui::components (MetricsPanel, LoggingWindow, CommandWindow, StatusBar)
├── FTXUI 6.1.9 (ftxui::component, ftxui::dom, ftxui::screen)
└── Core libraries (nlohmann_json, log4cxx)
```

### Build Configuration
```cmake
# Key CMakeLists.txt additions
add_executable(gdashboard
    src/gdashboard/dashboard_main.cpp
    src/gdashboard/DashboardApplication.cpp
    src/ui/MetricsPanel.cpp
    src/ui/LoggingWindow.cpp
    src/ui/CommandWindow.cpp
    src/ui/StatusBar.cpp
)

target_link_libraries(gdashboard
    PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen
    graph::executor
    app::metrics
    nlohmann_json::nlohmann_json
    log4cxx::log4cxx
)
```

---

## Phase Breakdown

### Timeline Overview

| Phase | Duration | Focus | Status |
|-------|----------|-------|--------|
| **Phase 0** | 3-4 weeks | MockGraphExecutor + MetricsCapability | ✅ **COMPLETE** (32/32 tests passing) |
| **Phase 1** | 1-2 weeks | Window structure + FTXUI rendering | ✅ **COMPLETE** (22/22 tests passing) |
| **Phase 2** | 2-3 weeks | Metrics integration + live updates | 🚀 **IN PROGRESS** (foundation classes complete) |
| **Phase 3** | 2-3 weeks | Enhanced features + polish | 📋 Planned |
| **Testing** | Continuous | Unit, integration, system tests | 📋 Planned |

### Dependency Graph
```
Phase 0 (MockGraphExecutor) ✅ COMPLETE
    ↓
Phase 1 (Window Structure) ✅ COMPLETE
    ↓
Phase 2 (Metrics Integration) 🚀 IN PROGRESS
    ↓
Phase 3 (Enhanced Features)
    ↓
Final Integration & Testing
```

---

## Detailed Phase 0: Foundation (MockGraphExecutor & MetricsCapability)

### Objective
Build core executor and metrics infrastructure for dashboard integration. Create MockGraphExecutor that simulates realistic graph execution with metrics publication. Implement MetricsCapability as the bridge between executor and dashboard.

### Duration
**3-4 weeks** (foundation, done in parallel with team)

### Key Success Factors
- Metrics publish at realistic rates (0-199 Hz configurable)
- Thread-safe callback invocation to dashboard
- Complete NodeMetricsSchema descriptors with alert rules
- MockGraphExecutor launches immediately with no external dependencies
- All 48 mock metrics discoverable and updateable

### Deliverables

#### D0.0: Project Structure & CMakeLists Organization
**File**: `CMakeLists.txt` (root) and subdirectories

**Directory Structure**:
```
src/
├── CMakeLists.txt (root)
├── graph/
│   ├── CMakeLists.txt
│   ├── executor/
│   │   ├── GraphExecutor.hpp          (Abstract interface)
│   │   ├── GraphExecutorImpl.cpp       (For real graphs, Phase 2+)
│   │   └── GraphExecutorBuilder.hpp   (Factory with fluent API)
│   │
│   └── mock/
│       ├── MockGraphExecutor.hpp      (~150 lines)
│       └── MockGraphExecutor.cpp      (~400 lines)
│
├── app/
│   ├── CMakeLists.txt
│   ├── metrics/
│   │   ├── MetricsEvent.hpp           (Data structure)
│   │   ├── NodeMetricsSchema.hpp      (Descriptor format)
│   │   ├── IMetricsSubscriber.hpp     (Callback interface)
│   │   ├── MetricsCapability.hpp      (Discovery + registration)
│   │   └── MetricsCapability.cpp      (~200 lines)
│   │
│   └── executor/
│       └── GraphExecutor.hpp          (Capability lookup interface)
│
├── gdashboard/
│   ├── CMakeLists.txt (Phase 1+)
│   └── ...
│
└── ui/
    ├── CMakeLists.txt (Phase 1+)
    └── ...

include/
├── graph/
│   ├── executor/GraphExecutor.hpp
│   ├── executor/GraphExecutorBuilder.hpp
│   └── mock/MockGraphExecutor.hpp
│
└── app/
    ├── metrics/MetricsEvent.hpp
    ├── metrics/NodeMetricsSchema.hpp
    ├── metrics/IMetricsSubscriber.hpp
    └── metrics/MetricsCapability.hpp

test/
├── graph/
│   ├── test_mock_graph_executor.cpp   (12 unit tests)
│   ├── test_metrics_publication.cpp   (8 unit tests)
│   └── test_graph_builder.cpp         (6 unit tests)
│
└── integration/
    ├── test_metrics_discovery.cpp     (4 integration tests)
    └── test_graph_executor_init.cpp   (5 integration tests)
```

**Total Phase 0 code**: ~1,000 lines (headers + implementation + tests)

#### D0.1: MetricsEvent & NodeMetricsSchema Data Structures
**File**: `include/app/metrics/MetricsEvent.hpp` + `include/app/metrics/NodeMetricsSchema.hpp`

**MetricsEvent** (~40 lines):
```cpp
#pragma once

#include <string>
#include <map>
#include <chrono>

struct MetricsEvent {
    // Metadata
    std::string source;                    // Node name: "DataValidator"
    std::string timestamp;                 // ISO 8601: "2025-01-24T12:00:00.123Z"
    
    // Metrics key-value pairs
    std::map<std::string, std::string> data;  // {"metric_name": "123.45", "unit": "ms"}
    
    // For rate limiting
    std::chrono::system_clock::time_point publish_time;
};
```

**NodeMetricsSchema** (~100 lines):
```cpp
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct NodeMetricsSchema {
    // Node identification
    std::string node_name;                 // "DataValidator", "Transform", etc.
    std::string node_type;                 // "filter", "map", "reduce", etc.
    
    // Complete metric descriptors
    nlohmann::json metrics_schema;         // Full field descriptor array
    
    // Example structure (JSON):
    // {
    //   "fields": [
    //     {
    //       "name": "validation_errors",
    //       "description": "Number of validation errors encountered",
    //       "display_type": "value",
    //       "unit": "count",
    //       "alert_rule": {
    //         "ok": [0, 10],
    //         "warning": [10, 50],
    //         "critical_range_type": "outside"
    //       }
    //     },
    //     {
    //       "name": "validation_rate",
    //       "description": "Validation throughput",
    //       "display_type": "gauge",
    //       "unit": "percent",
    //       "alert_rule": { ... }
    //     }
    //   ]
    // }
};
```

#### D0.2: MetricsCapability Interface
**File**: `include/app/metrics/MetricsCapability.hpp`

**Responsibilities**:
- Publish metrics on regular interval (configurable 1-199 Hz)
- Register callbacks for metric updates
- Provide complete NodeMetricsSchema descriptors
- Maintain metrics history (MetricsPanel responsibility)

**Public Interface** (~80 lines):
```cpp
#pragma once

#include "MetricsEvent.hpp"
#include "NodeMetricsSchema.hpp"
#include "IMetricsSubscriber.hpp"
#include <vector>
#include <memory>
#include <functional>

class MetricsCapability {
public:
    virtual ~MetricsCapability() = default;
    
    // Discovery: Get all node metrics schemas
    // Called during dashboard Initialize() to create tiles
    virtual std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const = 0;
    
    // Registration: Register callback for metrics updates
    // Called during dashboard Initialize()
    // Callback invoked by MetricsPublisher thread
    virtual void RegisterMetricsCallback(
        std::function<void(const MetricsEvent&)> callback) = 0;
    
    // Status
    virtual bool IsPublishing() const = 0;
    virtual size_t GetCallbackCount() const = 0;
};
```

**Design Notes**:
- `GetNodeMetricsSchemas()`: Called ONCE during Initialize(), returns complete schema
- `RegisterMetricsCallback()`: Called ONCE during Initialize(), callback invoked by publisher thread
- No GetDiscoveredMetrics() or GetCurrentValues() - dashboard buffers metrics
- No capability buffering - all buffering responsibility of MetricsPanel

#### D0.3: IMetricsSubscriber Callback Interface
**File**: `include/app/metrics/IMetricsSubscriber.hpp`

**Purpose**: Define callback contract for metrics updates

**Interface** (~30 lines):
```cpp
#pragma once

#include "MetricsEvent.hpp"

class IMetricsSubscriber {
public:
    virtual ~IMetricsSubscriber() = default;
    
    // Called by MetricsPublisher thread with each metrics event
    // MUST execute in < 1ms
    // MUST be thread-safe
    // MUST NOT block
    virtual void OnMetricsEvent(const MetricsEvent& event) = 0;
};
```

#### D0.4: GraphExecutor Interface & Builder
**File**: `include/graph/executor/GraphExecutor.hpp` + `include/graph/executor/GraphExecutorBuilder.hpp`

**GraphExecutor Interface** (~60 lines):
```cpp
#pragma once

#include <memory>
#include <string>

class MetricsCapability;

class GraphExecutor {
public:
    virtual ~GraphExecutor() = default;
    
    // Lifecycle
    virtual void Init() = 0;                    // Initialize graph
    virtual void Start() = 0;                   // Begin execution
    virtual void Stop() = 0;                    // Stop gracefully
    virtual void Join() = 0;                    // Wait for completion
    
    // Status
    virtual bool IsRunning() const = 0;
    
    // Capability lookup
    template<typename T>
    std::shared_ptr<T> GetCapability() {
        return std::dynamic_pointer_cast<T>(GetCapabilityImpl(typeid(T).name()));
    }
    
private:
    virtual std::shared_ptr<void> GetCapabilityImpl(const char* type_name) = 0;
};
```

**GraphExecutorBuilder** (~70 lines):
```cpp
#pragma once

#include "GraphExecutor.hpp"
#include <memory>
#include <string>

class GraphExecutorBuilder {
public:
    GraphExecutorBuilder() = default;
    
    // Fluent API for configuration
    GraphExecutorBuilder& LoadGraphConfig(const std::string& config_path) {
        config_path_ = config_path;
        return *this;
    }
    
    GraphExecutorBuilder& SetTimeout(int timeout_ms) {
        timeout_ms_ = timeout_ms;
        return *this;
    }
    
    GraphExecutorBuilder& UseMock(bool use_mock = true) {
        use_mock_ = use_mock;
        return *this;
    }
    
    GraphExecutorBuilder& SetMetricsPublishRate(int hz) {
        metrics_publish_rate_hz_ = hz;
        return *this;
    }
    
    // Build the executor
    std::shared_ptr<GraphExecutor> Build();
    
private:
    std::string config_path_ = "config/default_graph.json";
    int timeout_ms_ = 0;
    bool use_mock_ = true;
    int metrics_publish_rate_hz_ = 30;  // Default 30 Hz
};
```

#### D0.5: MockGraphExecutor Implementation
**File**: `include/graph/mock/MockGraphExecutor.hpp` + `src/graph/mock/MockGraphExecutor.cpp`

**Header** (~150 lines):
```cpp
#pragma once

#include "graph/executor/GraphExecutor.hpp"
#include "app/metrics/MetricsCapability.hpp"
#include <thread>
#include <atomic>
#include <map>
#include <functional>
#include <memory>
#include <random>

class MockMetricsCapability : public MetricsCapability {
public:
    std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const override;
    void RegisterMetricsCallback(
        std::function<void(const MetricsEvent&)> callback) override;
    bool IsPublishing() const override;
    size_t GetCallbackCount() const override;
    
private:
    mutable std::mutex callbacks_lock_;
    std::vector<std::function<void(const MetricsEvent&)>> callbacks_;
};

class MockGraphExecutor : public GraphExecutor {
public:
    explicit MockGraphExecutor(int metrics_publish_rate_hz = 30);
    ~MockGraphExecutor() override;
    
    // Lifecycle
    void Init() override;
    void Start() override;
    void Stop() override;
    void Join() override;
    
    bool IsRunning() const override;
    
    // For testing
    int GetMetricsPublishCount() const;
    const MetricsEvent& GetLastMetricsEvent() const;
    
private:
    // Configuration
    int metrics_publish_rate_hz_;           // 1-199 Hz
    
    // Lifecycle state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    
    // Metrics publication
    std::unique_ptr<MockMetricsCapability> metrics_capability_;
    std::thread metrics_publisher_thread_;
    
    // Metrics state (48 mock metrics)
    struct MockMetric {
        std::string node_name;
        std::string metric_name;
        double current_value = 0.0;
        double min_value = 0.0;
        double max_value = 100.0;
        int update_frequency_hz = 30;  // How often to update
    };
    
    std::vector<MockMetric> metrics_state_;
    mutable std::mutex metrics_lock_;
    
    MetricsEvent last_event_;
    std::atomic<int> publish_count_{0};
    
    // RNG for realistic metric values
    std::mt19937 rng_;
    
    // Capability lookup
    std::shared_ptr<void> GetCapabilityImpl(const char* type_name) override;
    
    // Internal methods
    void MetricsPublisherLoop();
    void InitializeMockMetrics();
    MetricsEvent GenerateMetricsEvent();
};
```

**Implementation** (~400 lines):

Key sections:

1. **Initialization** (~60 lines):
```cpp
void MockGraphExecutor::Init() {
    // Initialize 48 mock metrics
    InitializeMockMetrics();
    
    // Create metrics capability
    metrics_capability_ = std::make_unique<MockMetricsCapability>();
    
    initialized_ = true;
    std::cerr << "[MockGraphExecutor] Initialized with 48 metrics\n";
}

void MockGraphExecutor::InitializeMockMetrics() {
    // 6 nodes × 8 metrics = 48 total
    // Node names: DataValidator, Transform, Aggregator, Filter, etc.
    
    // DataValidator: 8 metrics
    metrics_state_.push_back({"DataValidator", "validation_errors", 5.0, 0, 100, 10});
    metrics_state_.push_back({"DataValidator", "validation_rate", 523.4, 0, 1000, 30});
    metrics_state_.push_back({"DataValidator", "queue_depth", 12.0, 0, 100, 5});
    // ... 45 more metrics
    
    std::cerr << "[MockGraphExecutor] Initialized " 
              << metrics_state_.size() << " mock metrics\n";
}
```

2. **Start/Stop** (~40 lines):
```cpp
void MockGraphExecutor::Start() {
    if (!initialized_) {
        throw std::runtime_error("MockGraphExecutor not initialized");
    }
    
    running_ = true;
    
    // Launch MetricsPublisher thread
    metrics_publisher_thread_ = std::thread(
        &MockGraphExecutor::MetricsPublisherLoop, this);
    
    std::cerr << "[MockGraphExecutor] Started metrics publication at "
              << metrics_publish_rate_hz_ << " Hz\n";
}

void MockGraphExecutor::Stop() {
    running_ = false;
    std::cerr << "[MockGraphExecutor] Stop requested\n";
}

void MockGraphExecutor::Join() {
    if (metrics_publisher_thread_.joinable()) {
        metrics_publisher_thread_.join();
    }
    std::cerr << "[MockGraphExecutor] Joined after "
              << publish_count_ << " metric events\n";
}
```

3. **Metrics Publisher Loop** (~80 lines):
```cpp
void MockGraphExecutor::MetricsPublisherLoop() {
    const auto frame_time = std::chrono::milliseconds(1000 / metrics_publish_rate_hz_);
    auto last_publish = std::chrono::high_resolution_clock::now();
    
    std::cerr << "[MetricsPublisher] Starting loop at "
              << metrics_publish_rate_hz_ << " Hz\n";
    
    while (running_) {
        try {
            // Generate and publish metrics event
            auto event = GenerateMetricsEvent();
            
            // Invoke all registered callbacks
            {
                std::lock_guard<std::mutex> lock(metrics_capability_->callbacks_lock_);
                for (const auto& callback : metrics_capability_->callbacks_) {
                    callback(event);
                }
            }
            
            publish_count_++;
            last_event_ = event;
            
            // Maintain publishing rate
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_publish);
            
            if (elapsed < frame_time) {
                std::this_thread::sleep_for(frame_time - elapsed);
            }
            
            last_publish = std::chrono::high_resolution_clock::now();
            
        } catch (const std::exception& e) {
            std::cerr << "[MetricsPublisher] Error: " << e.what() << "\n";
        }
    }
    
    std::cerr << "[MetricsPublisher] Loop exited\n";
}
```

4. **Metrics Generation** (~80 lines):
```cpp
MetricsEvent MockGraphExecutor::GenerateMetricsEvent() {
    std::lock_guard<std::mutex> lock(metrics_lock_);
    
    MetricsEvent event;
    event.timestamp = GetCurrentTimestamp();  // ISO 8601
    event.publish_time = std::chrono::system_clock::now();
    
    // Generate random metric updates
    // Update ~20% of metrics per event
    std::uniform_int_distribution<> update_dist(0, 100);
    
    for (auto& metric : metrics_state_) {
        if (update_dist(rng_) < 20) {  // 20% chance to update
            // Simulate realistic metric behavior
            // (random walk, periodic patterns, etc.)
            
            double delta = (update_dist(rng_) / 100.0 - 0.5) * 10;  // -5 to +5
            metric.current_value += delta;
            
            // Clamp to bounds
            metric.current_value = std::max(metric.min_value,
                std::min(metric.max_value, metric.current_value));
        }
        
        // Add to event data
        event.source = metric.node_name;
        event.data[metric.metric_name] = std::to_string(metric.current_value);
    }
    
    return event;
}
```

#### D0.6: MetricsCapability Implementation
**File**: `src/app/metrics/MetricsCapability.cpp` (~200 lines)

**Responsibilities**:
- Store registered callbacks safely (mutex-protected)
- Provide complete NodeMetricsSchemas
- Count callbacks for debugging

**Key Implementation**:
```cpp
void MockMetricsCapability::RegisterMetricsCallback(
    std::function<void(const MetricsEvent&)> callback) {
    
    std::lock_guard<std::mutex> lock(callbacks_lock_);
    callbacks_.push_back(callback);
    
    std::cerr << "[MetricsCapability] Registered callback "
              << (callbacks_.size()) << "\n";
}

std::vector<NodeMetricsSchema> MockMetricsCapability::GetNodeMetricsSchemas() const {
    // Return hardcoded schema for 6 nodes × 8 metrics = 48 total
    
    std::vector<NodeMetricsSchema> schemas;
    
    // Node 1: DataValidator
    NodeMetricsSchema validator_schema;
    validator_schema.node_name = "DataValidator";
    validator_schema.node_type = "filter";
    validator_schema.metrics_schema = nlohmann::json::parse(R"({
        "fields": [
            {
                "name": "validation_errors",
                "description": "Number of validation errors encountered",
                "display_type": "value",
                "unit": "count",
                "alert_rule": {
                    "ok": [0, 10],
                    "warning": [10, 50],
                    "critical_range_type": "outside"
                }
            },
            {
                "name": "validation_rate",
                "description": "Validation throughput",
                "display_type": "gauge",
                "unit": "records/sec",
                "alert_rule": {
                    "ok": [100, 1000],
                    "warning": [50, 100],
                    "critical_range_type": "outside"
                }
            }
            // ... 6 more metrics for DataValidator
        ]
    })");
    
    schemas.push_back(validator_schema);
    
    // Node 2-6: Similar schemas
    // ...
    
    std::cerr << "[MetricsCapability] GetNodeMetricsSchemas() returning "
              << schemas.size() << " node schemas\n";
    
    return schemas;
}
```

#### D0.7: Mock Metrics Specification (48 metrics across 6 nodes)
**Reference Document** (inline in MockGraphExecutor.cpp, ~150 lines)

**6 Node Types × 8 Metrics Each**:

1. **DataValidator** (input validation):
   - validation_errors (value: count)
   - validation_rate (gauge: records/sec)
   - queue_depth (value: count)
   - error_rate (gauge: percent)
   - processing_time (sparkline: ms)
   - memory_usage (gauge: mb)
   - cache_hit_rate (gauge: percent)
   - uptime (value: seconds)

2. **Transform** (data transformation):
   - rows_processed (value: count)
   - transform_rate (gauge: records/sec)
   - error_count (value: count)
   - avg_latency (sparkline: ms)
   - cpu_usage (gauge: percent)
   - memory_usage (gauge: mb)
   - queue_depth (value: count)
   - success_rate (gauge: percent)

3. **Aggregator** (aggregation):
   - aggregations_completed (value: count)
   - aggregation_rate (gauge: records/sec)
   - group_count (value: count)
   - result_size (sparkline: bytes)
   - cpu_usage (gauge: percent)
   - memory_usage (gauge: mb)
   - pending_groups (value: count)
   - computation_time (sparkline: ms)

4. **Filter** (filtering):
   - records_evaluated (value: count)
   - filter_rate (gauge: records/sec)
   - records_passed (value: count)
   - filter_latency (sparkline: us)
   - cpu_usage (gauge: percent)
   - memory_usage (gauge: mb)
   - pass_rate (gauge: percent)
   - error_count (value: count)

5. **Sink** (output/storage):
   - records_written (value: count)
   - write_rate (gauge: records/sec)
   - write_errors (value: count)
   - write_latency (sparkline: ms)
   - storage_usage (gauge: mb)
   - buffer_fullness (gauge: percent)
   - flush_count (value: count)
   - success_rate (gauge: percent)

6. **Monitor** (health monitoring):
   - health_checks_passed (value: count)
   - check_rate (gauge: checks/sec)
   - alert_count (value: count)
   - response_time (sparkline: ms)
   - cpu_usage (gauge: percent)
   - memory_usage (gauge: mb)
   - active_alerts (value: count)
   - uptime (value: seconds)

### Testing Strategy (Phase 0)

#### Unit Tests (26 tests total)

**MockGraphExecutor Tests** (12):
```cpp
TEST(MockGraphExecutorTest, ConstructsSuccessfully) { }
TEST(MockGraphExecutorTest, InitializesMetrics) { }
TEST(MockGraphExecutorTest, StartsMetricsPublisher) { }
TEST(MockGraphExecutorTest, StopsCleanly) { }
TEST(MockGraphExecutorTest, PublishesAtCorrectRate) { }
TEST(MockGraphExecutorTest, GetCapabilityReturnsMetrics) { }
TEST(MockGraphExecutorTest, CanStartStopMultipleTimes) { }
TEST(MockGraphExecutorTest, PublishCountIncreases) { }
TEST(MockGraphExecutorTest, GeneratesValidMetricsEvents) { }
TEST(MockGraphExecutorTest, ThreadSafeMetricsUpdates) { }
TEST(MockGraphExecutorTest, NoMemoryLeaksAfter1000Events) { }
TEST(MockGraphExecutorTest, HandlesHighPublishRates) { }
```

**MetricsCapability Tests** (8):
```cpp
TEST(MetricsCapabilityTest, RegistersCallbackSuccessfully) { }
TEST(MetricsCapabilityTest, ReturnsValidNodeSchemas) { }
TEST(MetricsCapabilityTest, Returns48Metrics) { }
TEST(MetricsCapabilityTest, CallbackInvokedOnMetricsEvent) { }
TEST(MetricsCapabilityTest, MultipleCallbacksWork) { }
TEST(MetricsCapabilityTest, CallbacksThreadSafe) { }
TEST(MetricsCapabilityTest, SchemaFieldsCompletelyDefined) { }
TEST(MetricsCapabilityTest, AlertRulesValidInAllSchemas) { }
```

**GraphExecutorBuilder Tests** (6):
```cpp
TEST(GraphExecutorBuilderTest, BuildsWithDefaults) { }
TEST(GraphExecutorBuilderTest, LoadsGraphConfig) { }
TEST(GraphExecutorBuilderTest, SetsTimeout) { }
TEST(GraphExecutorBuilderTest, UsesMockWhenRequested) { }
TEST(GraphExecutorBuilderTest, SetsMetricsRate) { }
TEST(GraphExecutorBuilderTest, FluentAPIWorks) { }
```

#### Integration Tests (9 tests)

**Metrics Discovery** (4):
```cpp
TEST(MetricsDiscoveryTest, DiscoverAllNodeSchemas) { }
TEST(MetricsDiscoveryTest, All48MetricsDiscoverable) { }
TEST(MetricsDiscoveryTest, SchemasHaveCompleteAlertRules) { }
TEST(MetricsDiscoveryTest, NoSchemaValidationErrors) { }
```

**Executor Initialization** (5):
```cpp
TEST(ExecutorInitializationTest, InitSequenceCorrect) { }
TEST(ExecutorInitializationTest, MetricsPublisherStartsAutomatically) { }
TEST(ExecutorInitializationTest, CallbacksInvokedAfterStart) { }
TEST(ExecutorInitializationTest, CleanShutdownNoHangups) { }
TEST(ExecutorInitializationTest, RapidRestartWorks) { }
```

#### Performance Tests
```cpp
TEST(PerformanceTest, PublishesAt199Hz) { }
TEST(PerformanceTest, CallbackLatencyUnder1ms) { }
TEST(PerformanceTest, NoMemoryLeaksAt199Hz) { }
```

### Acceptance Criteria (Phase 0)
- ✅ MockGraphExecutor launches immediately with no config
- ✅ All 48 metrics discoverable via GetNodeMetricsSchemas()
- ✅ Metrics publish at configurable rate (1-199 Hz)
- ✅ Callbacks invoked from separate MetricsPublisher thread
- ✅ NodeMetricsSchemas have complete field descriptors with alert rules
- ✅ All 4 display types supported (value, gauge, sparkline, state)
- ✅ Alert rules defined for all 48 metrics
- ✅ Thread-safe callback registration and invocation
- ✅ **All 20 unit tests pass** (MockGraphExecutor + MetricsCapability)
- ✅ **All 12 integration tests pass** (Discovery + Initialization + Performance)
- ✅ Performance tests confirm < 1ms callback latency
- ✅ Library builds successfully: `libgdashboard_lib.a`

**Phase 0 Summary**:
- **Test Results**: 32/32 passing ✅
  - Unit Tests: 20/20 (12 MockGraphExecutor + 8 MetricsCapability)
  - Integration Tests: 12/12 (4 Discovery + 5 Initialization + 3 Performance)
- **Metrics Implemented**: 48 across 6 nodes (DataValidator, Transform, Aggregator, Filter, Sink, Monitor)
- **Key Features**: Thread-safe publishing, configurable 1-199 Hz, complete schema discovery
- **Deliverables**: MockGraphExecutor, MetricsCapability, MetricsEvent, NodeMetricsSchema

---

## Detailed Phase 1: Basic Window Structure

### Objective
Create static dashboard layout with 4 sections (metrics, logging, command, status) using FTXUI flexbox. All sections display placeholders; no data integration yet.

### Duration
**1-2 weeks** (assuming Phase 0 complete)

### Key Success Factors
- Proper FTXUI initialization and component lifecycle
- Correct height percentage calculation and flexbox composition
- Responsive event handling (quit gracefully)
- No memory leaks or resource leaks
- Consistent rendering across different terminal sizes (80x24 minimum)

### Deliverables

#### D1.0: CMakeLists.txt Updates
**File**: `CMakeLists.txt` (root)

**Add to existing build**:
```cmake
# Add gdashboard executable target
add_executable(gdashboard
    src/gdashboard/dashboard_main.cpp
    src/gdashboard/DashboardApplication.cpp
)

# Link with FTXUI and other dependencies
target_link_libraries(gdashboard
    PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen
    nlohmann_json::nlohmann_json
)

# Set C++ standard
set_target_properties(gdashboard PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Add optimization flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(gdashboard PRIVATE -O3)
endif()
```

**Include paths**:
```cmake
target_include_directories(gdashboard
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

#### D1.1: Project Structure & File Organization
```
src/gdashboard/
├── dashboard_main.cpp              (Entry point, CLI parsing)
│                                   ~150 lines
│
└── DashboardApplication.cpp        (Orchestrator, layout, event loop)
                                    ~350 lines

include/ui/
├── DashboardApplication.hpp        (Public interface, 60 lines)
├── Window.hpp                      (Base class for all windows, 40 lines)
├── MetricsPanel.hpp                (Metrics section header, 50 lines)
├── LoggingWindow.hpp               (Logs section header, 50 lines)
├── CommandWindow.hpp               (Command section header, 50 lines)
├── StatusBar.hpp                   (Status footer header, 40 lines)
└── WindowHeightConfig.hpp          (Height configuration struct, 20 lines)

test/ui/
├── test_dashboard_application.cpp  (8 unit tests)
├── test_window_base.cpp            (5 unit tests)
├── test_metrics_panel.cpp          (4 unit tests)
├── test_logging_window.cpp         (4 unit tests)
├── test_command_window.cpp         (3 unit tests)
└── test_status_bar.cpp             (3 unit tests)

test/integration/
├── test_phase1_layout.cpp          (4 integration tests)
└── test_phase1_cli.cpp             (5 CLI argument tests)
```

**Total Phase 1 code**: ~1,200 lines (headers + implementation + tests)

#### D1.2: Command-Line Argument Parser & Validation
**File**: `src/gdashboard/dashboard_main.cpp`

**Responsibilities**:
- Parse CLI arguments: `--graph-config`, `--log-config`, `--timeout`, `--logging-height`, `--command-height`
- Load window height configuration from CLI or defaults
- Validate heights sum to 100% (with metrics 40% + status 2%)
- Handle errors gracefully (print to stderr, exit with code 1)
- Provide clear usage/help messages

**Implementation Structure** (~150 lines):
```cpp
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <stdexcept>

// Simple command-line parser
class CommandLineParser {
public:
    CommandLineParser(int argc, char* argv[]);
    
    std::string GetRequired(const std::string& key) const;
    std::string GetOptional(const std::string& key, const std::string& default_value) const;
    int GetOptionalInt(const std::string& key, int default_value) const;
    bool Has(const std::string& key) const;
    
    void PrintUsage() const;
    
private:
    std::map<std::string, std::string> args_;
    void Parse(int argc, char* argv[]);
};

// Window height configuration struct
struct WindowHeightConfig {
    int metrics_height_percent = 40;       // Fixed
    int logging_height_percent = 35;       // Adjustable
    int command_height_percent = 18;       // Adjustable
    int status_height_percent = 2;         // Fixed
    
    bool Validate() const {
        return (metrics_height_percent + logging_height_percent + 
                command_height_percent + status_height_percent) == 100;
    }
    
    std::string DebugString() const {
        return "Metrics:" + std::to_string(metrics_height_percent) + "% " +
               "Logging:" + std::to_string(logging_height_percent) + "% " +
               "Command:" + std::to_string(command_height_percent) + "% " +
               "Status:" + std::to_string(status_height_percent) + "%";
    }
};

int main(int argc, char* argv[]) {
    try {
        // 1. Parse command-line arguments
        CommandLineParser parser(argc, argv);
        
        // Get required graph-config
        std::string graph_config;
        try {
            graph_config = parser.GetRequired("--graph-config");
        } catch (const std::exception& e) {
            std::cerr << "Error: --graph-config is required\n";
            parser.PrintUsage();
            return 1;
        }
        
        // Get optional parameters
        std::string log_config = parser.GetOptional("--log-config", "");
        int timeout_ms = parser.GetOptionalInt("--timeout", 0);
        
        // Parse window heights
        WindowHeightConfig heights;
        heights.logging_height_percent = parser.GetOptionalInt("--logging-height", 35);
        heights.command_height_percent = parser.GetOptionalInt("--command-height", 18);
        
        // Validate heights
        if (!heights.Validate()) {
            std::cerr << "Error: Window heights must sum to 100%\n";
            std::cerr << "Current: " << heights.DebugString() << "\n";
            std::cerr << "Using defaults instead\n";
            heights = WindowHeightConfig();  // Reset to defaults
        }
        
        std::cerr << "Configuration: " << heights.DebugString() << "\n";
        
        // 2. Create GraphExecutor (from Phase 0)
        // TODO: GraphExecutorBuilder not yet implemented
        // auto executor = GraphExecutorBuilder()
        //     .LoadGraphConfig(graph_config)
        //     .SetTimeout(timeout_ms)
        //     .Build();
        
        // For Phase 1, use a mock/stub executor
        // auto executor = std::make_shared<StubGraphExecutor>();
        
        // 3. Create DashboardApplication
        // auto app = std::make_shared<DashboardApplication>(executor, heights);
        
        // 4-9: Initialize → Run
        // app->Initialize();
        // app->Run();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}

void CommandLineParser::PrintUsage() const {
    std::cout << "Usage: gdashboard --graph-config <config.json> [options]\n"
              << "\n"
              << "Required arguments:\n"
              << "  --graph-config <file>      Path to graph configuration JSON\n"
              << "\n"
              << "Optional arguments:\n"
              << "  --log-config <file>        Path to log4cxx configuration file\n"
              << "  --timeout <milliseconds>   Executor timeout in milliseconds\n"
              << "  --logging-height <percent> Logging window height (1-98, default 35)\n"
              << "  --command-height <percent> Command window height (1-98, default 18)\n"
              << "\n"
              << "Notes:\n"
              << "  - Logging and command heights must sum to <=100%\n"
              << "  - Metrics (40%) and Status (2%) heights are fixed\n"
              << "  - Invalid heights will default to: Metrics 40%, Logging 35%, Command 18%, Status 2%\n"
              << "\n"
              << "Examples:\n"
              << "  gdashboard --graph-config config/mock_graph.json\n"
              << "  gdashboard --graph-config config/mock_graph.json --logging-height 40\n"
              << "  gdashboard --graph-config config/mock_graph.json --timeout 5000\n";
}
```

**Error Handling** (~50 lines):
- Missing required argument → print error + usage → exit 1
- Invalid height values (not integers) → print error → use defaults
- Heights don't sum to 100% → log warning → use defaults
- File not found → caught during GraphExecutor construction (Phase 2)

**Testing**:
- ✅ Parse valid arguments
- ✅ Reject missing required arguments
- ✅ Handle invalid height values (non-integer)
- ✅ Handle heights sum > 100%
- ✅ Use defaults for optional arguments
- ✅ Exit with code 1 on error
- ✅ Print usage on --help (future: add --help)
- ✅ Handle arguments in any order

#### D1.3: DashboardApplication Core Class
**File**: `include/ui/DashboardApplication.hpp` + `src/gdashboard/DashboardApplication.cpp`

**Design Principles**:
1. Single responsibility: Orchestrate layout and event loop
2. Dependency injection: Receive executor as constructor parameter
3. Composition over inheritance: Use FTXUI components directly
4. Resource management: RAII with smart pointers
5. Testability: All components accessible via getters

**Header File** (~80 lines):
```cpp
#pragma once

#include <memory>
#include <string>
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"

// Forward declarations
class GraphExecutor;
class MetricsPanel;
class LoggingWindow;
class CommandWindow;
class StatusBar;

struct WindowHeightConfig {
    int metrics_height_percent = 40;       // Fixed
    int logging_height_percent = 35;       // Adjustable
    int command_height_percent = 18;       // Adjustable
    int status_height_percent = 2;         // Fixed
};

class DashboardApplication {
public:
    // Constructor receives executor and window heights
    explicit DashboardApplication(
        std::shared_ptr<GraphExecutor> executor,
        const WindowHeightConfig& heights);
    
    // Destructor - cleanup resources
    ~DashboardApplication();
    
    // Initialization: create UI panels, validate heights, setup layout
    void Initialize();
    
    // Main event loop: 30 FPS rendering until user exits
    void Run();
    
    // Getters for testing/debugging
    const std::shared_ptr<MetricsPanel>& GetMetricsPanel() const;
    const std::shared_ptr<LoggingWindow>& GetLoggingWindow() const;
    const std::shared_ptr<CommandWindow>& GetCommandWindow() const;
    const std::shared_ptr<StatusBar>& GetStatusBar() const;
    
    // For unit testing
    const WindowHeightConfig& GetWindowHeights() const;
    bool AreHeightsValid() const;
    
private:
    // Executor reference
    std::shared_ptr<GraphExecutor> executor_;
    
    // Window components (created in Initialize())
    std::shared_ptr<MetricsPanel> metrics_panel_;      // 40%
    std::shared_ptr<LoggingWindow> logging_window_;    // 35%
    std::shared_ptr<CommandWindow> command_window_;    // 18%
    std::shared_ptr<StatusBar> status_bar_;            // 2%
    
    // Layout state
    WindowHeightConfig window_heights_;
    bool should_exit_ = false;
    
    // FTXUI screen and component
    ftxui::ScreenInteractive* screen_ = nullptr;
    std::shared_ptr<ftxui::ComponentBase> main_component_;
    
    // Private methods
    void ValidateHeights();
    void ApplyHeights();
    ftxui::Element BuildLayout();
    void HandleUserInput(ftxui::Event event);
};
```

**Implementation File** (~350 lines):
```cpp
#include "ui/DashboardApplication.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include <iostream>
#include <thread>
#include <chrono>

DashboardApplication::DashboardApplication(
    std::shared_ptr<GraphExecutor> executor,
    const WindowHeightConfig& heights)
    : executor_(executor), window_heights_(heights) {
    
    if (!executor_) {
        throw std::invalid_argument("Executor cannot be null");
    }
    
    ValidateHeights();
    std::cerr << "DashboardApplication constructed: " 
              << window_heights_.DebugString() << "\n";
}

DashboardApplication::~DashboardApplication() {
    // RAII cleanup - components destroyed automatically via shared_ptr
    std::cerr << "DashboardApplication destroyed\n";
}

void DashboardApplication::Initialize() {
    std::cerr << "[Initialize] Starting dashboard setup\n";
    
    try {
        // 1. Create window components
        std::cerr << "[Initialize] Creating window components...\n";
        
        metrics_panel_ = std::make_shared<MetricsPanel>("Metrics");
        metrics_panel_->SetHeight(window_heights_.metrics_height_percent);
        
        logging_window_ = std::make_shared<LoggingWindow>("Logs");
        logging_window_->SetHeight(window_heights_.logging_height_percent);
        logging_window_->AddLogLine("[2025-01-24 12:00:00] Dashboard initialized");
        
        command_window_ = std::make_shared<CommandWindow>("Command");
        command_window_->SetHeight(window_heights_.command_height_percent);
        
        status_bar_ = std::make_shared<StatusBar>();
        
        std::cerr << "[Initialize] Created 4 window components\n";
        
        // 2. Validate heights
        ValidateHeights();
        
        // 3. Apply heights to layout
        ApplyHeights();
        
        // 4. Create main FTXUI component (for Phase 2)
        // main_component_ = CreateMainComponent();
        
        std::cerr << "[Initialize] Dashboard initialization complete\n";
        
    } catch (const std::exception& e) {
        std::cerr << "[Initialize] Error: " << e.what() << "\n";
        throw;
    }
}

void DashboardApplication::ValidateHeights() {
    const int total = window_heights_.metrics_height_percent +
                      window_heights_.logging_height_percent +
                      window_heights_.command_height_percent +
                      window_heights_.status_height_percent;
    
    if (total != 100) {
        std::cerr << "[ValidateHeights] ERROR: Heights sum to " << total 
                  << "%, expected 100%\n";
        std::cerr << "[ValidateHeights] Resetting to defaults\n";
        window_heights_ = WindowHeightConfig();
    }
}

void DashboardApplication::ApplyHeights() {
    if (metrics_panel_) metrics_panel_->SetHeight(window_heights_.metrics_height_percent);
    if (logging_window_) logging_window_->SetHeight(window_heights_.logging_height_percent);
    if (command_window_) command_window_->SetHeight(window_heights_.command_height_percent);
    
    std::cerr << "[ApplyHeights] Applied: " << window_heights_.DebugString() << "\n";
}

ftxui::Element DashboardApplication::BuildLayout() {
    // Using FTXUI flexbox with percentages
    // Metrics (40%) | Logging (35%) | Command (18%) | Status (2%)
    
    using namespace ftxui;
    
    // Metrics panel (40%)
    auto metrics_element = metrics_panel_->Render() | 
                          yflex(window_heights_.metrics_height_percent);
    
    // Logging window (35%)
    auto logging_element = logging_window_->Render() | 
                          yflex(window_heights_.logging_height_percent);
    
    // Command window (18%)
    auto command_element = command_window_->Render() | 
                          yflex(window_heights_.command_height_percent);
    
    // Status bar (2%)
    auto status_element = status_bar_->Render() | 
                         yflex(window_heights_.status_height_percent);
    
    // Compose vertical layout
    return vbox({
        metrics_element,
        separator(),
        logging_element,
        separator(),
        command_element,
        separator(),
        status_element
    });
}

void DashboardApplication::Run() {
    std::cerr << "[Run] Starting event loop (30 FPS)\n";
    
    const auto frame_time = std::chrono::milliseconds(33);  // ~30 FPS
    auto last_frame = std::chrono::high_resolution_clock::now();
    int frame_count = 0;
    
    while (!should_exit_) {
        try {
            // Check for user exit ('q' key)
            // TODO: Integrate with FTXUI event handling (Phase 2)
            
            // Render layout
            // auto layout = BuildLayout();
            // ftxui::Screen::Create()->Print(layout);
            
            frame_count++;
            
            // Sleep to maintain 30 FPS
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_frame);
            
            if (elapsed < frame_time) {
                std::this_thread::sleep_for(frame_time - elapsed);
            }
            
            last_frame = std::chrono::high_resolution_clock::now();
            
            // Exit for Phase 1 testing - in Phase 2 integrate real event loop
            if (frame_count >= 30) {  // Run for ~1 second then exit
                should_exit_ = true;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[Run] Error in event loop: " << e.what() << "\n";
            should_exit_ = true;
        }
    }
    
    std::cerr << "[Run] Event loop exited after " << frame_count << " frames\n";
}

void DashboardApplication::HandleUserInput(ftxui::Event event) {
    // TODO: Implement event handling (Phase 2)
    // if (event == ftxui::Event::Character('q')) {
    //     should_exit_ = true;
    // }
}

// Getters
const std::shared_ptr<MetricsPanel>& DashboardApplication::GetMetricsPanel() const {
    return metrics_panel_;
}

const std::shared_ptr<LoggingWindow>& DashboardApplication::GetLoggingWindow() const {
    return logging_window_;
}

const std::shared_ptr<CommandWindow>& DashboardApplication::GetCommandWindow() const {
    return command_window_;
}

const std::shared_ptr<StatusBar>& DashboardApplication::GetStatusBar() const {
    return status_bar_;
}

const WindowHeightConfig& DashboardApplication::GetWindowHeights() const {
    return window_heights_;
}

bool DashboardApplication::AreHeightsValid() const {
    const int total = window_heights_.metrics_height_percent +
                      window_heights_.logging_height_percent +
                      window_heights_.command_height_percent +
                      window_heights_.status_height_percent;
    return total == 100;
}
```

**Key Implementation Notes**:
1. Constructor: Validate executor pointer, validate heights
2. Initialize(): Create all 4 window components in order
3. ValidateHeights(): Check sum = 100%, log errors, reset on failure
4. ApplyHeights(): Propagate heights to each window
5. BuildLayout(): Use FTXUI vbox + yflex for percentage-based layout
6. Run(): Simple event loop with 30 FPS timing (Phase 2 integrates FTXUI event loop)
7. Error handling: Log all errors to stderr, re-throw for test visibility

#### D1.4: Window Component Base Class & Implementations
**File**: `include/ui/Window.hpp` (base class)

**Base Class** (~60 lines):
```cpp
#pragma once

#include "ftxui/component/component.hpp"
#include <string>
#include <memory>

// Base class for all window components
class Window {
public:
    explicit Window(const std::string& title = "");
    virtual ~Window() = default;
    
    // Rendering - must be implemented by subclasses
    virtual ftxui::Element Render() const = 0;
    
    // Event handling - optional, default no-op
    virtual bool HandleEvent(ftxui::Event event) { return false; }
    
    // Height management
    void SetHeight(int percent) { height_percent_ = percent; }
    int GetHeight() const { return height_percent_; }
    
    // Title management
    void SetTitle(const std::string& title) { title_ = title; }
    const std::string& GetTitle() const { return title_; }
    
protected:
    std::string title_;
    int height_percent_ = 100;  // Default to full height
};
```

**MetricsPanel** (`include/ui/MetricsPanel.hpp` + implementation, ~120 lines):
```cpp
#pragma once

#include "Window.hpp"
#include <vector>
#include <deque>

class MetricsPanel : public Window {
public:
    explicit MetricsPanel(const std::string& title = "Metrics");
    
    // Window interface
    ftxui::Element Render() const override;
    bool HandleEvent(ftxui::Event event) override;
    
    // Metrics placeholder management
    void AddPlaceholderMetric(const std::string& metric_name, double value);
    void ClearPlaceholders();
    
    // Scrolling support (for Phase 2+)
    void ScrollUp() { if (scroll_offset_ > 0) scroll_offset_--; }
    void ScrollDown() { scroll_offset_++; }
    
private:
    // Placeholder metrics (Phase 1 only)
    struct PlaceholderMetric {
        std::string name;
        double value;
    };
    
    std::vector<PlaceholderMetric> metrics_;
    size_t scroll_offset_ = 0;
    
    ftxui::Element RenderPlaceholder() const;
    ftxui::Element RenderGrid() const;  // For Phase 2
};
```

**Implementation** (~100 lines):
```cpp
#include "ui/MetricsPanel.hpp"
#include "ftxui/dom/elements.hpp"
#include <iomanip>
#include <sstream>

MetricsPanel::MetricsPanel(const std::string& title)
    : Window(title) {
    
    // Add initial placeholder metrics
    metrics_ = {
        {"Throughput", 523.4},
        {"Processing Time", 48.2},
        {"Queue Depth", 12},
        {"Error Count", 0},
        {"Node A Status", 1.0},
        {"Node B Status", 1.0},
        {"Cache Hit Rate", 0.95},
        {"Memory Usage", 0.42},
        {"CPU Usage", 0.38},
    };
}

ftxui::Element MetricsPanel::Render() const {
    using namespace ftxui;
    
    std::vector<Element> tiles;
    
    for (const auto& metric : metrics_) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << metric.value;
        
        auto tile = vbox({
            text("  " + metric.name + "  ") | bold,
            text("  " + ss.str() + "  "),
        }) | border;
        
        tiles.push_back(flex(tile));
    }
    
    // Grid layout: 3 columns
    std::vector<Element> rows;
    for (size_t i = 0; i < tiles.size(); i += 3) {
        std::vector<Element> row;
        for (size_t j = 0; j < 3 && i + j < tiles.size(); ++j) {
            row.push_back(tiles[i + j]);
        }
        rows.push_back(hbox(std::move(row)));
    }
    
    return vbox(std::move(rows)) | 
           border | 
           title_decorator(text(" " + title_ + " "));
}

bool MetricsPanel::HandleEvent(ftxui::Event event) {
    // Phase 2: Handle scroll events
    return false;
}

void MetricsPanel::AddPlaceholderMetric(const std::string& metric_name, double value) {
    metrics_.push_back({metric_name, value});
}

void MetricsPanel::ClearPlaceholders() {
    metrics_.clear();
}
```

**LoggingWindow** (`include/ui/LoggingWindow.hpp` + implementation, ~120 lines):
```cpp
#pragma once

#include "Window.hpp"
#include <deque>

class LoggingWindow : public Window {
public:
    explicit LoggingWindow(const std::string& title = "Logs");
    
    ftxui::Element Render() const override;
    bool HandleEvent(ftxui::Event event) override;
    
    // Log management
    void AddLogLine(const std::string& line);
    void ClearLogs();
    size_t GetLogCount() const { return log_lines_.size(); }
    
    // Scrolling
    void ScrollUp() { if (scroll_offset_ > 0) scroll_offset_--; }
    void ScrollDown() { scroll_offset_++; }
    
private:
    std::deque<std::string> log_lines_;
    size_t scroll_offset_ = 0;
    static constexpr size_t MAX_LINES = 500;
    
    ftxui::Element RenderLogs() const;
};
```

**Implementation** (~100 lines):
```cpp
#include "ui/LoggingWindow.hpp"
#include "ftxui/dom/elements.hpp"
#include <vector>

LoggingWindow::LoggingWindow(const std::string& title)
    : Window(title) {
    
    // Add initial placeholder logs
    log_lines_.push_back("[2025-01-24 12:00:00.123] [INFO]  Dashboard starting");
    log_lines_.push_back("[2025-01-24 12:00:00.456] [DEBUG] GraphExecutor created");
    log_lines_.push_back("[2025-01-24 12:00:00.789] [INFO]  Metrics capability available");
}

ftxui::Element LoggingWindow::Render() const {
    using namespace ftxui;
    
    std::vector<Element> log_elements;
    
    // Show recent logs (scrollable)
    size_t start = scroll_offset_;
    size_t end = std::min(start + 10, log_lines_.size());  // Show 10 lines
    
    for (size_t i = start; i < end; ++i) {
        log_elements.push_back(text(log_lines_[i]) | dim);
    }
    
    if (log_elements.empty()) {
        log_elements.push_back(text("  No log entries"));
    }
    
    return vbox(std::move(log_elements)) | 
           border | 
           title_decorator(text(" " + title_ + " "));
}

bool LoggingWindow::HandleEvent(ftxui::Event event) {
    // Phase 2: Handle scroll events
    return false;
}

void LoggingWindow::AddLogLine(const std::string& line) {
    log_lines_.push_back(line);
    if (log_lines_.size() > MAX_LINES) {
        log_lines_.pop_front();
    }
}

void LoggingWindow::ClearLogs() {
    log_lines_.clear();
    scroll_offset_ = 0;
}
```

**CommandWindow** (`include/ui/CommandWindow.hpp` + implementation, ~120 lines):
```cpp
#pragma once

#include "Window.hpp"
#include <string>
#include <deque>

class CommandWindow : public Window {
public:
    explicit CommandWindow(const std::string& title = "Command");
    
    ftxui::Element Render() const override;
    bool HandleEvent(ftxui::Event event) override;
    
    // Command/output management
    void AddOutputLine(const std::string& line);
    void ClearOutput();
    std::string GetInputBuffer() const { return input_buffer_; }
    
private:
    std::string input_buffer_;
    std::deque<std::string> output_lines_;
    static constexpr size_t MAX_OUTPUT = 100;
    
    ftxui::Element RenderPrompt() const;
    ftxui::Element RenderOutput() const;
};
```

**Implementation** (~100 lines):
```cpp
#include "ui/CommandWindow.hpp"
#include "ftxui/dom/elements.hpp"
#include <vector>

CommandWindow::CommandWindow(const std::string& title)
    : Window(title) {
    
    // Add initial prompt message
    output_lines_.push_back("Type 'help' for available commands");
    output_lines_.push_back("Type 'q' to quit");
}

ftxui::Element CommandWindow::Render() const {
    using namespace ftxui;
    
    // Render output history
    std::vector<Element> output_elements;
    for (const auto& line : output_lines_) {
        output_elements.push_back(text(line) | dim);
    }
    
    if (output_elements.empty()) {
        output_elements.push_back(text(""));
    }
    
    auto output_box = vbox(std::move(output_elements));
    
    // Render input prompt
    auto input_prompt = hbox({
        text("> "),
        text(input_buffer_) | color(ftxui::Color::White),
        text("_") | blink,  // Cursor
    });
    
    return vbox({
        output_box | flex,
        separator(),
        input_prompt,
    }) | border | title_decorator(text(" " + title_ + " "));
}

bool CommandWindow::HandleEvent(ftxui::Event event) {
    // Phase 2: Handle keyboard input
    return false;
}

void CommandWindow::AddOutputLine(const std::string& line) {
    output_lines_.push_back(line);
    if (output_lines_.size() > MAX_OUTPUT) {
        output_lines_.pop_front();
    }
}

void CommandWindow::ClearOutput() {
    output_lines_.clear();
}
```

**StatusBar** (`include/ui/StatusBar.hpp` + implementation, ~100 lines):
```cpp
#pragma once

#include "Window.hpp"
#include <string>

class StatusBar {
public:
    StatusBar();
    
    ftxui::Element Render() const;
    
    void SetStatus(const std::string& status) { status_ = status; }
    void SetNodeCount(int active, int total) { 
        active_nodes_ = active; 
        total_nodes_ = total; 
    }
    void SetErrorCount(int errors) { error_count_ = errors; }
    
private:
    std::string status_ = "READY";
    int active_nodes_ = 0;
    int total_nodes_ = 0;
    int error_count_ = 0;
};
```

**Implementation** (~80 lines):
```cpp
#include "ui/StatusBar.hpp"
#include "ftxui/dom/elements.hpp"
#include <sstream>

StatusBar::StatusBar() {
    // Initialize with default values
}

ftxui::Element StatusBar::Render() const {
    using namespace ftxui;
    
    std::stringstream ss;
    ss << "Status: " << status_ 
       << " | Nodes: " << active_nodes_ << "/" << total_nodes_
       << " | Errors: " << error_count_;
    
    return hbox({
        text(ss.str()) | bold | color(ftxui::Color::Cyan),
    });
}
```

### Testing Strategy (Phase 1)

#### Unit Tests
```cpp
// test/ui/test_dashboard_application.cpp
TEST(DashboardApplicationTest, ConstructsWithValidParameters) { }
TEST(DashboardApplicationTest, ValidatesWindowHeights) { }
TEST(DashboardApplicationTest, RaisesErrorOnInvalidHeights) { }
TEST(DashboardApplicationTest, GettersReturnValidPointers) { }

// test/ui/test_metrics_panel.cpp
TEST(MetricsPanelTest, RendersPlaceholderContent) { }
TEST(MetricsPanelTest, HandleScrolling) { }

// test/ui/test_logging_window.cpp
TEST(LoggingWindowTest, AddsLogLines) { }
TEST(LoggingWindowTest, MaintainsMaxLineLimit) { }

// test/ui/test_command_window.cpp
TEST(CommandWindowTest, AcceptsInput) { }

// test/ui/test_status_bar.cpp
TEST(StatusBarTest, UpdatesStatus) { }
```

#### Integration Tests
```cpp
// test/integration/test_phase1_layout.cpp
TEST(Phase1LayoutTest, DashboardLaunchesWithValidConfig) { }
TEST(Phase1LayoutTest, DisplaysAllFourSections) { }
TEST(Phase1LayoutTest, MaintainsHeightRatios) { }
TEST(Phase1LayoutTest, ExitsCleanlyOnQuit) { }
```

#### Manual Testing Checklist
- [ ] `./gdashboard --graph-config config/mock_graph.json` launches
- [ ] Dashboard displays 4 sections with correct proportions (40/35/18/2)
- [ ] Pressing 'q' exits cleanly
- [ ] Window heights can be adjusted via CLI
- [ ] Invalid heights trigger error message and use defaults
- [ ] No artifacts or rendering issues on 80x24 terminal

### Acceptance Criteria (Phase 1)
- ✅ gdashboard executable compiles without errors
- ✅ Application launches and displays 4 sections
- ✅ Height ratios maintained (metrics 40%, logs 35%, command 23%, status 2% = 100%)
- ✅ CLI parameters parsed correctly (--graph-config, --log-config, heights)
- ✅ Window heights configurable and validated
- ✅ All 22 Phase 1 unit tests pass
- ✅ MockGraphExecutor integration complete
- ✅ 30 FPS event loop initialized
- ✅ Window state management working
- ✅ Placeholder content for all 4 sections

**Phase 1 Summary**:
- **Test Results**: 22/22 passing ✅
  - DashboardApplication: 5 tests
  - MetricsPanel: 4 tests
  - LoggingWindow: 4 tests  
  - CommandWindow: 4 tests
  - StatusBar: 4 tests
  - Plus integration tests
- **Components Implemented**: DashboardApplication, MetricsPanel, LoggingWindow, CommandWindow, StatusBar
- **FTXUI Rendering**: ✅ **RESTORED** - All windows render with borders, colors, and styled text at 30 FPS
  - MetricsPanel: Green bordered box with metric list
  - LoggingWindow: Blue bordered box with log lines (dim text)
  - CommandWindow: Yellow bordered box with command prompt
  - StatusBar: White text on black background (centered)
  - BuildLayout() composes all windows in vbox with Run() rendering each frame
- **CLI Features**: Argument parsing, height validation, graceful error handling
- **Architecture**: Window-based layout with MockGraphExecutor from Phase 0
- **Deliverables**: Executable (./src/gdashboard) with --graph-config parameter and full FTXUI rendering

---

## Detailed Phase 2: Metrics Integration

### Objective
Integrate MetricsCapability with Dashboard to discover metrics schemas, create tiles dynamically, and display live updates via callbacks.

### Duration
**2-3 weeks** (Phase 1 complete)

### Prerequisites
- Phase 0: MockGraphExecutor + MetricsCapability complete
- Phase 1: Dashboard layout complete

### Deliverables

#### D2.1: MetricsTileWindow Implementation
**File**: `include/ui/MetricsTileWindow.hpp` + `src/ui/MetricsTileWindow.cpp`

**Status**: ✅ **COMPLETE** (Commit f68b02b)

**Represents**: Single metric tile with specific display type and alert status

**Completed Features**:
- ✅ 4 rendering methods: RenderValue() (3 lines), RenderGauge() (5 lines), RenderSparkline() (7 lines), RenderState() (3 lines)
- ✅ Alert threshold support with color coding (OK=Green, WARNING=Yellow, CRITICAL=Red)
- ✅ UpdateValue() for frame-by-frame data updates
- ✅ Circular history buffer (60 samples) for sparkline rendering
- ✅ MetricDescriptor struct for metadata (node, name, unit, thresholds)
- ✅ AlertStatus enum for status determination
- ✅ Format support with precision control

**Public Interface** (~200 lines total):
```cpp
class MetricsTileWindow {
public:
    // Constructor from NodeMetricsSchema field
    MetricsTileWindow(
        const std::string& node_name,
        const std::string& metric_name,
        const nlohmann::json& field_descriptor);
    
    // Rendering
    ftxui::Element Render() const;
    
    // Update metric value (called from OnMetricsEvent callback)
    void UpdateValue(double value);
    
    // Accessors
    std::string GetNodeName() const { return node_name_; }
    std::string GetMetricName() const { return metric_name_; }
    std::string GetDisplayType() const { return display_type_; }
    double GetCurrentValue() const { return current_value_; }
    
private:
    // Metadata
    std::string node_name_;          // "DataValidator"
    std::string metric_name_;        // "validation_errors"
    std::string display_type_;       // "value", "gauge", "sparkline", "state"
    std::string unit_;               // "percent", "count", "ms", etc.
    std::string description_;
    
    // State
    double current_value_ = 0.0;
    double min_value_ = 0.0;
    double max_value_ = 0.0;
    std::chrono::system_clock::time_point last_update_;
    
    // History for sparklines (thread-safe)
    std::deque<double> value_history_;
    mutable std::mutex history_lock_;
    static constexpr size_t MAX_HISTORY = 120;  // ~4 seconds at 30 FPS
    
    // Alert configuration (from field descriptor)
    struct AlertRule {
        std::array<double, 2> ok;
        std::array<double, 2> warning;
        std::string critical_range_type;  // "outside" or "inside"
    } alert_rule_;
    
    // Rendering helpers
    ftxui::Element RenderValue() const;      // 3 lines
    ftxui::Element RenderGauge() const;      // 5 lines
    ftxui::Element RenderSparkline() const;  // 7 lines
    ftxui::Element RenderState() const;      // 3 lines
    
    // Status determination
    enum class AlertStatus { OK, Warning, Critical };
    AlertStatus GetAlertStatus() const;
    ftxui::Color GetStatusColor() const;
};
```

**Implementation Highlights**:

1. **Value Update Thread Safety**:
   - Use `std::mutex` to protect value_history_
   - Call `UpdateValue()` from MetricsPublisher thread
   - Call `GetCurrentValue()` from FTXUI render thread

2. **Sparkline Rendering**:
   - Maintain circular buffer of last 120 values
   - Normalize to 5-line height range
   - Display trend with simple ASCII characters: ╱ ─ ╲

3. **Alert Status Coloring**:
   - Parse `alert_rule` from field descriptor
   - Compare current value to ranges
   - Return ftxui::Color: GREEN (OK), YELLOW (Warning), RED (Critical)

4. **Memory Efficiency**:
   - Bounded history (120 values max)
   - No dynamic allocation per update
   - Minimal overhead per tile

#### D2.1.5: MetricsTilePanel Implementation
**File**: `include/ui/MetricsTilePanel.hpp` + `src/ui/MetricsTilePanel.cpp`

**Status**: ✅ **COMPLETE** (Commit f68b02b)

**Represents**: Container for managing multiple MetricsTileWindow instances

**Completed Features**:
- ✅ Tile lifecycle management (AddTile, GetTile, RemoveAll)
- ✅ Tile indexing by metric ID for O(1) lookups
- ✅ Composite rendering of all tiles (current: vbox layout)
- ✅ UpdateAllMetrics() placeholder for frame-by-frame updates
- ✅ Thread-safe for concurrent updates from executor
- ✅ Support for dynamic tile creation during Initialize()

**Public Interface** (~100 lines):
```cpp
class MetricsTilePanel {
public:
    explicit MetricsTilePanel(std::shared_ptr<MetricsCapability> metrics_cap = nullptr);
    
    // Tile management
    void AddTile(std::shared_ptr<MetricsTileWindow> tile);
    std::shared_ptr<MetricsTileWindow> GetTile(const std::string& metric_id) const;
    
    // Updates
    void UpdateAllMetrics();  // Called each frame
    
    // Rendering
    ftxui::Element Render() const;
    
    // Accessors
    size_t GetTileCount() const { return tiles_.size(); }
    void SetMetricsCapability(std::shared_ptr<MetricsCapability> metrics_cap);
    
private:
    std::vector<std::shared_ptr<MetricsTileWindow>> tiles_;
    std::map<std::string, size_t> tile_index_;
    std::shared_ptr<MetricsCapability> metrics_cap_;
};
```

#### D2.2: Metrics Discovery from NodeMetricsSchemas
**File**: `src/ui/MetricsPanel.cpp` - UpdateMetricsList() method

**Responsibilities**:
- Query executor's MetricsCapability
- Call `GetNodeMetricsSchemas()`
- For each node and field, create MetricsTileWindow
- Populate 3-column grid layout
- Store tile lookup map: `node_name::metric_name` → tile

**Code** (~150 lines):
```cpp
void MetricsPanel::DiscoverAndCreateTiles(
    std::shared_ptr<MetricsCapability> metrics_cap) {
    
    // Clear existing tiles
    tiles_.clear();
    tile_lookup_.clear();
    
    // Get all node metrics schemas
    auto node_schemas = metrics_cap->GetNodeMetricsSchemas();
    
    for (const auto& node_schema : node_schemas) {
        // Extract fields from schema
        if (!node_schema.metrics_schema.contains("fields")) {
            continue;  // No metrics in this node
        }
        
        const auto& fields = node_schema.metrics_schema["fields"];
        for (const auto& field : fields) {
            // Create tile from field descriptor
            auto tile = std::make_shared<MetricsTileWindow>(
                node_schema.node_name,
                field["name"].get<std::string>(),
                field
            );
            
            // Store in lookup map
            std::string key = node_schema.node_name + "::" + field["name"].get<std::string>();
            tile_lookup_[key] = tile;
            
            // Add to tiles vector (for grid layout)
            tiles_.push_back(tile);
        }
    }
    
    LOG(INFO) << "Discovered " << tiles_.size() << " metrics across " 
              << node_schemas.size() << " nodes";
}
```

#### D2.3: DashboardApplication Integration with Metrics
**File**: `src/gdashboard/DashboardApplication.cpp` - Update Initialize()

**Execution Order**:
1. Get MetricsCapability from executor
2. Call metrics_panel_->DiscoverAndCreateTiles()
3. Register callback via metrics_cap->RegisterMetricsCallback()
4. All tiles created before executor.Init()

**Code** (~100 lines):
```cpp
void DashboardApplication::Initialize() {
    // Get MetricsCapability from executor
    metrics_cap_ = executor_->GetCapability<MetricsCapability>();
    
    // Discover metrics from NodeMetricsSchemas
    metrics_panel_->DiscoverAndCreateTiles(metrics_cap_);
    
    // Register callback for live updates
    metrics_cap_->RegisterMetricsCallback(
        [this](const MetricsEvent& event) {
            this->OnMetricsEvent(event);
        }
    );
    
    LOG(INFO) << "DashboardApplication initialized";
}

void DashboardApplication::OnMetricsEvent(const MetricsEvent& event) {
    // Called by MetricsPublisher thread
    // Must be extremely fast (<1ms)
    
    std::string key = event.source + "::" + event.data.at("metric_name");
    auto it = tile_lookup_.find(key);
    
    if (it != tile_lookup_.end()) {
        double value = std::stod(event.data.at("value"));
        it->second->UpdateValue(value);
    }
}
```

#### D2.4: Metrics Grid Layout Rendering
**File**: `src/ui/MetricsPanel.cpp` - RenderGrid() method

**Algorithm**:
- 3 columns per row
- Calculate max height for each row
- Render tiles in grid order (left-to-right, top-to-bottom)
- Support scrolling if panel height insufficient

**Code** (~120 lines):
```cpp
ftxui::Element MetricsPanel::RenderGrid() const {
    if (tiles_.empty()) {
        return ftxui::text("No metrics discovered");
    }
    
    std::vector<ftxui::Element> rows;
    
    for (size_t i = 0; i < tiles_.size(); i += 3) {
        std::vector<ftxui::Element> row_elements;
        
        // Collect 3 tiles for this row
        for (size_t j = 0; j < 3 && i + j < tiles_.size(); ++j) {
            row_elements.push_back(
                ftxui::flex(tiles_[i + j]->Render())
            );
        }
        
        // Pad incomplete rows
        while (row_elements.size() < 3) {
            row_elements.push_back(ftxui::text(""));
        }
        
        // Create row with flexbox
        rows.push_back(
            ftxui::hbox(std::move(row_elements)) | ftxui::border
        );
    }
    
    // Apply scrolling if needed
    auto grid = ftxui::vbox(std::move(rows));
    
    if (current_scroll_offset_ > 0) {
        // TODO: Implement scrolling wrapper
    }
    
    return grid;
}
```

#### D2.5: Initialization Sequence with Metrics
**File**: `src/gdashboard/dashboard_main.cpp` - Update main()

**9-Step Sequence**:
```cpp
int main(int argc, char* argv[]) {
    // Step 1: Parse command-line arguments
    auto heights = ParseCommandLine(argc, argv);
    
    try {
        // Step 2: Build GraphExecutor
        auto executor = GraphExecutorBuilder()
            .LoadGraphConfig(graph_config)
            .SetTimeout(timeout_ms)
            .Build();
        
        // Step 3: Create DashboardApplication
        auto app = std::make_shared<DashboardApplication>(executor, heights);
        
        // Step 4: Initialize (discovers metrics, creates tiles, registers callback)
        app->Initialize();
        
        // Step 5: Call executor.Init()
        executor->Init();
        
        // Step 6: Call executor.Start() (MetricsPublisher thread begins)
        executor->Start();
        
        // Step 7: Wait 100ms for metrics to accumulate
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Step 8: Run dashboard (30 FPS loop)
        app->Run();
        
        // Step 9: Cleanup
        executor->Stop();
        executor->Join();
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Fatal error: " << e.what();
        return 1;
    }
    
    return 0;
}
```

### Testing Strategy (Phase 2)

#### Unit Tests
```cpp
// test/ui/test_metrics_tile_window.cpp
TEST(MetricsTileWindowTest, ConstructsFromFieldDescriptor) { }
TEST(MetricsTileWindowTest, UpdatesValueThreadSafely) { }
TEST(MetricsTileWindowTest, DeterminesAlertStatusCorrectly) { }
TEST(MetricsTileWindowTest, RendersValueDisplay) { }
TEST(MetricsTileWindowTest, RendersGaugeDisplay) { }
TEST(MetricsTileWindowTest, RendersSparklineDisplay) { }
TEST(MetricsTileWindowTest, RendersStateDisplay) { }
TEST(MetricsTileWindowTest, MaintainsSparklineHistory) { }

// test/ui/test_metrics_panel.cpp
TEST(MetricsPanelTest, DiscoversMetricsFromSchemas) { }
TEST(MetricsPanelTest, CreatesCorrectNumberOfTiles) { }
TEST(MetricsPanelTest, PopulatesLookupMapCorrectly) { }
TEST(MetricsPanelTest, RendersGridLayout) { }
```

#### Integration Tests
```cpp
// test/integration/test_phase2_metrics_flow.cpp
TEST(Phase2MetricsFlowTest, ExecutorPublishesMetrics) { }
TEST(Phase2MetricsFlowTest, CallbacksInvokedCorrectly) { }
TEST(Phase2MetricsFlowTest, TilesUpdateWithMetrics) { }
TEST(Phase2MetricsFlowTest, RenderingKeepsUpWith30FPS) { }
TEST(Phase2MetricsFlowTest, NoDataRacesBetweenThreads) { }
```

#### System Tests
```cpp
// test/system/test_phase2_end_to_end.cpp
TEST(Phase2EndToEndTest, DiscoverMetrics) {
    // 1. Launch gdashboard with mock_graph.json
    // 2. Verify 48 metrics discovered
    // 3. Verify tiles created for each metric
    // 4. Verify metrics update in real-time
    // 5. Verify no artifacts in rendering
}
```

### Acceptance Criteria (Phase 2)
- ✅ All 48 metrics discovered from MockGraphExecutor
- ✅ MetricsTileWindow implements all 4 display types
- ✅ Tiles update in real-time as executor publishes metrics
- ✅ No data races between MetricsPublisher thread and FTXUI thread
- ✅ Dashboard maintains 30 FPS rendering
- ✅ Sparkline tiles display historical trends correctly
- ✅ All 12+ Phase 2 unit tests pass
- ✅ All 4+ Phase 2 integration tests pass
- ✅ End-to-end system test passes

---

## Detailed Phase 3: Enhanced Features

### Objective
Implement optional/polish features: commands, logging, layout persistence, advanced layouts (tabbed mode).

### Duration
**2-3 weeks** (Phase 2 complete)

### Deliverables

#### D3.1: CommandWindow Implementation (Extensible)

**Purpose**: Accept user commands, execute handlers, display results

**Commands to Implement**:
- `help` - List all available commands
- `clear` - Clear command history
- `run_graph` - Start graph execution
- `pause_execution` - Pause (if supported)
- `stop_execution` - Stop execution
- `status` - Show current status
- `reset_layout` - Reset dashboard to defaults

#### D3.2: LoggingWindow with log4cxx Integration

**Purpose**: Capture log4cxx output, display with filtering, search

**Features**:
- Custom log4cxx appender
- Level filtering (DEBUG, INFO, WARN, ERROR, FATAL)
- Text search
- Auto-scroll
- Circular buffer (max 1000 lines)

#### D3.3: Layout Persistence (JSON Config)

**Purpose**: Remember user preferences across sessions

**Saves**:
- Selected tabs (if tabbed mode)
- Scroll positions
- Command history
- Log level filter
- Custom window heights

**Storage**: `~/.gdashboard/layout.json`

#### D3.4: Advanced Layout Strategies

**Tabbed Mode**:
- Automatically activated if metrics don't fit in grid
- 6-12 metrics per tab
- Tab navigation via arrow keys
- Labeled by node name

**Vertical/Horizontal Modes**:
- Single-column or single-row layouts
- Alternative to grid for few metrics

### Acceptance Criteria (Phase 3)
- ✅ All commands implemented and functional
- ✅ log4cxx messages appear in LoggingWindow
- ✅ Filtering and search work correctly
- ✅ Layout state persists across sessions
- ✅ Tabbed mode automatically activates for many metrics
- ✅ All 10+ Phase 3 unit tests pass
- ✅ All 3+ Phase 3 integration tests pass

---

## Testing Strategy

### Overall Test Coverage Goals
- **Unit Tests**: 80%+ code coverage
- **Integration Tests**: All major workflows tested
- **System Tests**: End-to-end from CLI to display

### Test Pyramid

```
       /\
      /  \  System Tests (5%)
     /────\
    /      \
   /        \ Integration Tests (25%)
  /──────────\
 /            \
/              \ Unit Tests (70%)
/────────────────\
```

### Test Execution Plan
```bash
# Run all unit tests
cmake --build . --target test

# Run with coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make test-coverage
open coverage/index.html

# Run integration tests
test/integration/test_all.sh

# Run system tests (requires X11 or display emulation)
test/system/test_all.sh
```

### Key Test Scenarios

#### Thread Safety Testing
```cpp
// test/threading/test_metrics_concurrency.cpp
TEST(MetricsConcurrencyTest, MetricsPublisherAndRenderThreadDontRace) {
    // 1. Start MetricsPublisher thread (from MockGraphExecutor)
    // 2. Update 100 metrics simultaneously
    // 3. Read tile state from FTXUI render thread
    // 4. Verify all values are consistent (no torn reads)
    // 5. Repeat 100 times
}
```

#### Performance Testing
```cpp
// test/performance/test_rendering_fps.cpp
TEST(PerformanceTest, RenderingMaintains30FPS) {
    // 1. Setup 48 metric tiles
    // 2. Update metrics at 199 Hz
    // 3. Measure frame render time
    // 4. Verify average < 33ms
    // 5. Verify no missed frames
}
```

#### Memory Testing
```cpp
// test/memory/test_memory_leaks.cpp
TEST(MemoryTest, NoLeaksAfter1000Cycles) {
    // 1. Launch dashboard
    // 2. Update metrics 1000 times
    // 3. Measure memory usage
    // 4. Verify no growth
}
```

---

## Risk Management

### Identified Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| FTXUI rendering artifacts | Medium | High | Early visual testing, fallback to simpler layout |
| Thread safety bugs (race conditions) | Medium | High | Extensive testing, use atomic types, careful locking |
| Metrics updates too fast for rendering | Low | Medium | Buffer metrics, render at fixed 30 FPS |
| Memory leaks in tile creation/destruction | Medium | Medium | Use smart pointers, RAII, valgrind testing |
| log4cxx integration issues | Low | Low | Create custom appender early, test integration |
| CLI argument parsing edge cases | Low | Low | Comprehensive unit tests for parser |

### Mitigation Strategies

1. **Rendering Issues**:
   - Start with simple grid layout
   - Test on multiple terminal sizes (80x24, 120x40, etc.)
   - Have fallback text-only mode

2. **Thread Safety**:
   - Use `std::atomic<>` for numeric values
   - Use `std::mutex` only for complex structures
   - Run with ThreadSanitizer (clang)
   - Extensive concurrent testing

3. **Performance**:
   - Profile rendering with flamegraph
   - Measure callback latency
   - Keep callback code < 1ms

4. **Memory**:
   - Use valgrind for leak detection
   - Profile with heaptrack
   - Test with 100+ metrics

---

## Success Criteria

### Functional Requirements
- ✅ CLI: Parse all command-line arguments correctly
- ✅ Discovery: Discover all metrics from NodeMetricsSchemas
- ✅ Display: Show metrics in 3-column grid with 4 display types
- ✅ Update: Real-time updates via callbacks (no polling)
- ✅ Layout: 4-section dashboard with correct height ratios
- ✅ Commands: Execute commands from CommandWindow
- ✅ Logging: Display log4cxx output in LoggingWindow
- ✅ Configuration: Load/save layout state

### Non-Functional Requirements
- ✅ Performance: Maintain 30 FPS rendering
- ✅ Thread Safety: No race conditions or data races
- ✅ Memory: No memory leaks, bounded memory usage
- ✅ Scalability: Handle 100+ metrics without degradation
- ✅ Responsiveness: CLI arguments parsed in <100ms

### Code Quality
- ✅ Code Coverage: 80%+ unit test coverage
- ✅ Static Analysis: 0 warnings (clang-tidy)
- ✅ Documentation: All public APIs documented
- ✅ Formatting: Consistent code style (clang-format)

### Testing Coverage
- ✅ Phase 1: 8+ unit tests + 4+ integration tests
- ✅ Phase 2: 12+ unit tests + 4+ integration tests
- ✅ Phase 3: 10+ unit tests + 3+ integration tests
- ✅ System: 5+ end-to-end tests

---

## Implementation Dependencies

### Must Be Complete Before Starting
1. ✅ ARCHITECTURE.md (finalized)
2. ✅ CMakeLists.txt updated with gdashboard target
3. ✅ Phase 0: MockGraphExecutor + MetricsCapability
4. ✅ FTXUI 6.1.9 integrated in build
5. ✅ Test framework (Google Test) configured

### Optional (Nice-to-Have Before Starting)
- Demo mock_graph.json with sample metrics
- Log4cxx configuration file example
- Terminal size detection utility

---

## Continuous Integration/Deployment

### Build Pipeline
```
Commit → Compile → Unit Tests → Integration Tests → System Tests → Deployment
```

### CI Configuration
```yaml
# .github/workflows/build.yml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Configure
        run: cmake -B build
      - name: Build
        run: cmake --build build
      - name: Unit Tests
        run: cmake --build build --target test
      - name: Coverage Report
        run: cmake --build build --target coverage
```

---

## Rollout Plan

### Development Timeline

**Week 1-2: Phase 1**
- Complete window structure and layout
- 8+ unit tests passing
- Visual verification on multiple terminal sizes

**Week 3-4: Phase 2**
- MetricsTileWindow implementation
- Metrics discovery and callback integration
- 12+ unit tests + 4+ integration tests passing
- 48 metrics displaying correctly

**Week 5-6: Phase 3 Part A**
- CommandWindow and LoggingWindow
- log4cxx integration
- Layout persistence

**Week 7-8: Phase 3 Part B**
- Advanced layout strategies (tabbed, vertical)
- Polish and optimization
- System tests passing
- Performance profiling

**Week 9: Testing and Documentation**
- Comprehensive system testing
- Update user documentation
- Performance benchmarking
- Code review and cleanup

**Week 10: Release**
- Final integration tests
- Release notes
- Deployment

---

## Review Checklist

Before approving implementation, verify:

- [ ] All architecture decisions documented and understood
- [ ] Phase dependencies clear and agreed upon
- [ ] Test strategy comprehensive
- [ ] Risk mitigation plans in place
- [ ] Success criteria measurable and achievable
- [ ] Timeline realistic given team capacity
- [ ] Build system configuration ready
- [ ] Development environment set up
- [ ] Communication plan established

---

## Phase 2 Progress & Continuation Plan

### Completed in Phase 2 (as of January 25, 2026)

✅ **MetricsTileWindow** - Complete metric display component
- RenderValue(), RenderGauge(), RenderSparkline(), RenderState() methods
- Alert threshold logic (OK/WARNING/CRITICAL color coding)
- Circular history buffer (60 samples) for trend visualization
- Thread-safe UpdateValue() for executor integration
- Commit: f68b02b

✅ **MetricsTilePanel** - Container for metric tile management
- AddTile(), GetTile() for tile lifecycle
- Tile indexing by metric ID (O(1) lookup)
- Composite rendering with vbox layout
- UpdateAllMetrics() placeholder for frame updates
- Thread-safe for concurrent executor updates
- Commit: f68b02b

✅ **FTXUI Rendering Integration** (Phase 1 completion)
- All 4 window components (MetricsPanel, LoggingWindow, CommandWindow, StatusBar) rendering with colors and borders
- 30 FPS rendering loop with frame-by-frame updates
- Full layout composition in BuildLayout()
- Commit: 3c3d7b4

✅ **Build System** - CMakeLists.txt updated
- MetricsTileWindow.cpp and MetricsTilePanel.cpp added
- No compilation errors
- All 54 tests passing (20 Phase 0 + 12 Phase 0 integration + 22 Phase 1)

### Remaining Phase 2 Tasks (Priority Order)

#### Task D2.5: Integrate MetricsCapability Discovery
**Objective**: Connect DashboardApplication::Initialize() to discover metrics from executor

**Implementation**:
1. Modify `MetricsPanel::UpdateMetricsList()` to:
   - Query `executor_->GetMetricsCapability()`
   - Parse all NodeMetricsSchemas
   - For each metric field, create MetricsTileWindow
   - Register tiles in MetricsTilePanel
   - Create tile lookup map: `node_name + "::" + field_name → tile`

2. Update `MetricsPanel::Render()` to use MetricsTilePanel for rendering

3. Register MetricsCapability callbacks:
   ```cpp
   executor_->GetMetricsCapability()->RegisterCallback(
       [this](const Event& event) { HandleMetricUpdate(event); }
   );
   ```

**Test Cases** (3-4 new tests):
- TEST: Discovers all 48 metrics from mock_graph.json
- TEST: Creates correct MetricsTileWindow instances
- TEST: Tile lookup map is properly initialized

#### Task D2.6: Implement UpdateAllMetrics() and Frame Updates
**Objective**: Ensure MetricsTilePanel updates every frame with latest executor data

**Implementation**:
1. Store latest metric values in MetricsTilePanel:
   ```cpp
   std::map<std::string, double> latest_values_;
   std::mutex values_lock_;
   ```

2. Implement UpdateAllMetrics():
   ```cpp
   void MetricsTilePanel::UpdateAllMetrics() {
       std::lock_guard<std::mutex> lock(values_lock_);
       for (auto& [key, value] : latest_values_) {
           if (auto tile = GetTile(key)) {
               tile->UpdateValue(value);
           }
       }
   }
   ```

3. Update DashboardApplication::Run() to call:
   ```cpp
   while (keepRunning) {
       metrics_panel_->UpdateAllMetrics();  // Update all tiles
       auto screen = ftxui::Screen::Create(...);
       ftxui::Render(screen, BuildLayout());
       output(screen.ToString());
   }
   ```

4. Handle incoming events from executor:
   ```cpp
   void MetricsPanel::HandleMetricUpdate(const Event& event) {
       auto key = event.source + "::" + event.data.at("metric_name");
       metrics_tile_panel_->SetLatestValue(key, value);
   }
   ```

**Test Cases** (3-4 new tests):
- TEST: UpdateAllMetrics() updates all tiles
- TEST: Tiles maintain 60-sample history across frames
- TEST: No memory leaks from repeated updates

#### Task D2.7: Verify Color-Coded Alert Status
**Objective**: Ensure tiles render correct colors based on alert thresholds

**Implementation**:
1. Verify MetricsTileWindow::GetStatusColor() returns:
   - Green (OK) when value within ok_min/ok_max
   - Yellow (WARNING) when value within warning_min/warning_max
   - Red (CRITICAL) when value outside critical_range

2. Update tile Render() methods to apply color:
   ```cpp
   ftxui::Element MetricsTileWindow::RenderValue() const {
       return ftxui::text(FormatValue()) | ftxui::color(GetStatusColor());
   }
   ```

3. Test with mock_graph.json metrics:
   - CPU metrics (should show alerts at 80%, 95%)
   - Memory metrics (should show alerts at 70%, 90%)
   - Network throughput (should stay green under 1000 Mbps)

**Test Cases** (3-4 new tests):
- TEST: Correct color for OK status (green)
- TEST: Correct color for WARNING status (yellow)
- TEST: Correct color for CRITICAL status (red)
- TEST: Color changes as value crosses thresholds

#### Task D2.8: End-to-End Phase 2 Testing
**Objective**: Verify complete metrics flow from executor to display

**System Test** (test/system/test_phase2_end_to_end.cpp):
```cpp
TEST(Phase2EndToEndTest, CompleteMetricsFlow) {
    // 1. Launch gdashboard with mock_graph.json
    // 2. Wait for initialization (should discover 48 metrics)
    // 3. Verify MetricsPanel contains 48 tiles
    // 4. Wait 100ms for metrics to start flowing
    // 5. Verify tile values change over time
    // 6. Verify colors update for alerts
    // 7. Verify no rendering glitches
    // 8. Verify consistent metrics at 30 FPS
}
```

**Performance Test** (test/performance/test_rendering_fps.cpp):
```cpp
TEST(PerformanceTest, Phase2RenderingMaintains30FPS) {
    // 1. Setup 48 metric tiles
    // 2. Measure frame render time with metrics updating
    // 3. Verify average frame time < 33ms
    // 4. Verify tail latency < 50ms (99th percentile)
    // 5. Verify no memory leaks over 1000 frames
}
```

### Continuation Path (After Phase 2 Completion)

Once all Phase 2 tasks complete:
1. Run full test suite: `cmake --build . --target test`
2. Verify all 54+ tests passing
3. Manual system test: `./build/gdashboard --config=config/mock_graph.json`
4. Verify visual output shows all 48 metrics with proper colors/spacing
5. Clean git state: `git status` should show clean
6. Begin Phase 3: CommandWindow and LoggingWindow implementation

---

## Next Steps

1. **Immediate** (Today):
   - Complete IMPLEMENTATION_PLAN.md update ✅
   - Begin Task D2.5 (MetricsCapability integration)
   - Target completion: End of today

2. **Short Term** (Next 2-3 hours):
   - Complete Task D2.6 (UpdateAllMetrics implementation)
   - Complete Task D2.7 (Color-coded alerts)
   - Begin Task D2.8 testing

3. **Phase 2 Completion** (Within 4 hours):
   - All 4 Phase 2 tasks complete
   - 54+ tests passing
   - Ready for Phase 3

4. **Ongoing**:
   - Continuous testing and validation
   - Update documentation as implementation progresses
   - Maintain clean git history

---

**Document Version**: 1.1  
**Last Updated**: January 25, 2026  
**Prepared By**: Architecture & Design Team  
**Status**: Phase 2 In Progress - Foundation Classes Complete, Integration Next
