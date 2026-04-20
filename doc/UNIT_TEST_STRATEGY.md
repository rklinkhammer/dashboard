# Comprehensive Unit Test Strategy for Non-UI Components

**Purpose**: Define systematic unit testing for all non-UI components in the gdashboard project before web framework migration.

**Test Reference Configuration**: `config/graph_csv_pipeline_dual_port_integration.json`
- 11 nodes (5 CSV sensors, 5 core processing, 1 aggregator)
- 14 edges (9 main pipeline, 5 completion path)
- Dual-port architecture (Port 0: data, Port 1: completion signals)
- 4-thread execution with backpressure and deadlock detection

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│ Core Layer                                                  │
│  • ThreadPool (src/graph/ThreadPool.cpp)                    │
│  • ActiveQueue (core/ActiveQueue.hpp)                       │
│  • TypeInfo (core/TypeInfo.hpp)                             │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│ Graph Layer                                                 │
│  • Port/PortTypes - message routing & metadata              │
│  • NodeFactory/Registry - node creation & discovery         │
│  • GraphConfigParser - JSON parsing                         │
│  • JsonDynamicGraphLoader - dynamic graph construction      │
│  • GraphExecutor/Builder - execution engine                 │
│  • EdgeRegistry/EdgeRegistration - topology management      │
│  • CapabilityBus - cross-cutting concerns                   │
│  • Message types - serialization                            │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│ Application Layer (Policies & Capabilities)                 │
│  • MetricsPolicy/Capability - metrics collection            │
│  • DataInjectionPolicy/Capability - CSV injection           │
│  • CSVInjectionPolicy - CSV-specific handling               │
│  • CommandPolicy/Capability - command execution             │
│  • CompletionPolicy - scenario completion tracking          │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│ Node Implementation Layer                                   │
│  • DataInjectionXxxNode (5 CSV sensor types)                │
│  • FlightFSMNode - state machine & fusion                   │
│  • AltitudeFusionNode - multi-source blending               │
│  • EstimationPipelineNode - filtering & refinement          │
│  • FlightMonitorNode - adaptive feedback                    │
│  • FeedbackTestSinkNode - result collection                 │
│  • CompletionAggregatorNode - completion tracking           │
└─────────────────────────────────────────────────────────────┘
```

---

## Testing Strategy by Layer

### Layer 1: Core Components (Foundation)

**Objective**: Verify fundamental building blocks work correctly in isolation.

#### 1.1 ThreadPool Tests

**File**: `test/core/test_thread_pool.cpp` (EXISTING - verify current)

**What to Test**:
- [x] Initialization with default and custom thread counts
- [x] Task queueing and execution
- [x] Graceful shutdown (Stop/Join)
- [x] Timeout handling (JoinWithTimeout)
- [x] Statistics tracking (tasks_completed, tasks_failed, enqueue_timeouts)
- [x] Deadlock detection watchdog
- [x] Lock-free atomic guarantees
- [ ] **NEW**: Queue backpressure handling
- [ ] **NEW**: Task rejection when stopped
- [ ] **NEW**: Average task execution time calculation

**Test Cases**:
```cpp
// Existing tests to verify
TEST(ThreadPool, Init)
TEST(ThreadPool, StartAndJoin)
TEST(ThreadPool, QueueMultipleTasks)
TEST(ThreadPool, StopAcceptsNoMoreTasks)
TEST(ThreadPool, DeadlockDetection)

// New tests to add
TEST(ThreadPool, QueueTaskRejectsWhenStopped)
TEST(ThreadPool, BackpressureEnforcement)
TEST(ThreadPool, AverageTaskTimeCalculation)
TEST(ThreadPool, StatisticsAccuracy)
TEST(ThreadPool, ConcurrentTaskExecution)  // 4 threads running simultaneously
```

**Setup Fixtures**:
```cpp
class ThreadPoolTest : public ::testing::Test {
    ThreadPool pool{4, DeadlockConfig{true, 10ms}};
    std::atomic<int> task_count{0};
    std::vector<int> execution_order;
    std::mutex execution_order_mtx;
};
```

---

#### 1.2 ActiveQueue Tests

**File**: `test/core/test_active_queue.cpp` (EXISTING - verify current)

**What to Test**:
- [x] Enqueue/Dequeue operations
- [x] Lock-free semantics
- [x] Size tracking
- [x] Disable/Enable functionality
- [ ] **NEW**: Concurrent reader/writer stress
- [ ] **NEW**: Boundary conditions (capacity limits)
- [ ] **NEW**: Memory ordering verification

**Test Cases**:
```cpp
TEST(ActiveQueue, EnqueueDequeue)
TEST(ActiveQueue, SizeTracking)
TEST(ActiveQueue, DisableBlocksDequeue)
TEST(ActiveQueue, NonBlockingDequeue)
TEST(ActiveQueue, ConcurrentStress)  // N writers, M readers
TEST(ActiveQueue, MemoryOrdering)     // Verify acquire/release semantics
```

---

#### 1.3 TypeInfo Tests

**File**: `test/core/test_type_info.cpp` (NEW)

**What to Test**:
- Compile-time type name extraction via consteval
- Namespace stripping correctness
- Type hashing consistency
- Type equality operations

**Test Cases**:
```cpp
TEST(TypeInfo, ConstexprTypeName)
TEST(TypeInfo, NamespaceStripping)
TEST(TypeInfo, TypeHashing)
TEST(TypeInfo, TypeEquality)
TEST(TypeInfo, StdTypeIndexIntegration)
```

---

### Layer 2: Graph Infrastructure (Topology & Mechanics)

**Objective**: Verify graph structure, node lifecycle, and message routing work correctly.

#### 2.1 GraphConfigParser Tests

**File**: `test/graph/test_graph_config_parser.cpp` (✅ COMPLETE - 23/24 passing)

**Status**: DONE
- [x] Parse valid JSON configurations
- [x] Node extraction with proper structure
- [x] Edge extraction and validation
- [x] Port specification parsing
- [x] Error handling for malformed configs
- [x] Dual-port architecture support
- [x] Node configuration application
- [x] Edge buffer sizes and backpressure settings

**Results**: 23/24 tests passing (95.8%), 1 conditional skip

---

#### 2.2 JsonDynamicGraphLoader Tests

**File**: `test/graph/test_json_dynamic_graph_loader.cpp` (✅ COMPLETE - 18/18 passing)

**Status**: DONE
- [x] Load graph from valid configuration
- [x] Node registry population
- [x] Edge creation and connection
- [x] Port validation
- [x] Graph topology correctness after loading
- [x] Configuration parsing (minimal, single-node, multi-node, dual-port)
- [x] Edge extraction and validation
- [x] Error handling (malformed JSON, missing arrays, duplicate IDs)
- [x] File I/O operations
- [x] Integration scenarios (Phase 2 multi-sensor pipelines)

**Results**: 18/18 tests passing (100%)

---

#### 2.3 NodeFactory Tests

**File**: `test/graph/test_node_factory.cpp` (🔄 IN PROGRESS)

**What to Test**:
- Node type registration
- Node creation by type
- Node configuration application
- Error handling for unknown types
- Plugin loading (if applicable)

**Test Cases**:
```cpp
TEST(NodeFactory, RegisterNodeType)
TEST(NodeFactory, CreateNodeByType)
TEST(NodeFactory, ApplyNodeConfiguration)
TEST(NodeFactory, RejectUnknownType)
TEST(NodeFactory, DiscoverAvailableTypes)
```

---

#### 2.4 EdgeRegistry Tests

**File**: `test/graph/test_edge_registry.cpp` (NEW)

**What to Test**:
- Edge registration between nodes/ports
- Port validation
- Edge retrieval
- Cycle detection (if applicable)
- Buffer size management

**Test Cases**:
```cpp
TEST(EdgeRegistry, RegisterEdge)
TEST(EdgeRegistry, ValidatePortCompatibility)
TEST(EdgeRegistry, RejectInvalidConnections)
TEST(EdgeRegistry, RetrieveEdgeBySourcePort)
TEST(EdgeRegistry, CycleDetection)
```

---

#### 2.5 PortTypes & Message Routing Tests

**File**: `test/graph/test_port_types.cpp` (NEW)

**What to Test**:
- Port metadata extraction
- Message type matching
- Port compatibility verification
- Payload type validation
- Buffer allocation correctness

**Test Cases**:
```cpp
TEST(PortTypes, ExtractPortMetadata)
TEST(PortTypes, MatchMessageTypes)
TEST(PortTypes, ValidateCompatibility)
TEST(PortTypes, HandleDualPortArchitecture)  // Port 0 vs Port 1
TEST(PortTypes, AllocateBuffers)
```

---

### Layer 3: Graph Execution Engine

**Objective**: Verify graph execution lifecycle, policy chain, and state management.

#### 3.1 GraphExecutor Tests

**File**: `test/graph/test_graph_executor.cpp` (NEW - CRITICAL)

**What to Test**:
- Executor state transitions (STOPPED → INITIALIZING → RUNNING → STOPPING → STOPPED)
- Init/Start/Stop/Join lifecycle
- Node initialization before execution
- Graceful shutdown
- Thread pool integration
- Error propagation from nodes
- Execution timeout handling
- Deadlock detection integration

**Test Cases**:
```cpp
TEST(GraphExecutor, StateTransitions)
TEST(GraphExecutor, InitializeNodes)
TEST(GraphExecutor, ExecuteGraphSequentially)
TEST(GraphExecutor, GracefulShutdown)
TEST(GraphExecutor, TimeoutHandling)
TEST(GraphExecutor, ErrorPropagation)
TEST(GraphExecutor, ThreadPoolCoordination)
TEST(GraphExecutor, IntegrationWithPhase2Pipeline)
TEST(GraphExecutor, CompletionSignalDetection)
```

**Setup Fixtures**:
```cpp
class GraphExecutorTest : public ::testing::Test {
    std::shared_ptr<GraphManager> graph;
    std::unique_ptr<GraphExecutor> executor;
    ExecutionPolicyChain policies;
    
    void SetUp() override {
        // Load Phase 2 config
        // Build graph
        // Create executor
    }
};
```

---

#### 3.2 ExecutionPolicyChain Tests

**File**: `test/graph/test_execution_policy_chain.cpp` (NEW)

**What to Test**:
- Policy chain initialization
- Policy invocation order
- Policy error handling
- Policy state management
- Integration with executor

**Test Cases**:
```cpp
TEST(ExecutionPolicyChain, InvokePoliciesInOrder)
TEST(ExecutionPolicyChain, HandlePolicyErrors)
TEST(ExecutionPolicyChain, ChainMultiplePolicies)
TEST(ExecutionPolicyChain, StateManagement)
```

---

### Layer 4: Application Policies & Capabilities

**Objective**: Verify cross-cutting concerns work correctly with graph execution.

#### 4.1 MetricsPolicy Tests

**File**: `test/app/policies/test_metrics_policy.cpp` (NEW)

**What to Test**:
- Metrics collection from executing nodes
- Metrics event generation
- Publisher notifications
- Metrics aggregation
- Counter/gauge/histogram accuracy

**Test Cases**:
```cpp
TEST(MetricsPolicy, CollectNodeMetrics)
TEST(MetricsPolicy, PublishMetricsEvents)
TEST(MetricsPolicy, AggregateMetrics)
TEST(MetricsPolicy, TrackExecutionTime)
TEST(MetricsPolicy, CountNodeExecutions)
```

---

#### 4.2 DataInjectionPolicy Tests

**File**: `test/app/policies/test_data_injection_policy.cpp` (NEW)

**What to Test**:
- Data injection source registration
- Injection scheduling
- Data format validation
- Backpressure handling
- Source lifecycle (start/stop)

**Test Cases**:
```cpp
TEST(DataInjectionPolicy, RegisterSource)
TEST(DataInjectionPolicy, ScheduleInjection)
TEST(DataInjectionPolicy, ValidateDataFormat)
TEST(DataInjectionPolicy, BackpressureHandling)
```

---

#### 4.3 CSVInjectionPolicy Tests

**File**: `test/app/policies/test_csv_injection_policy.cpp` (NEW)

**What to Test**:
- CSV file parsing
- Row-by-row injection
- Rate limiting (10ms intervals per config)
- Data type casting
- Sensor type mapping
- Completion signal generation
- Error handling for malformed CSV

**Test Cases**:
```cpp
TEST(CSVInjectionPolicy, ParseCSVFile)
TEST(CSVInjectionPolicy, InjectRowSequentially)
TEST(CSVInjectionPolicy, RateLimiting)
TEST(CSVInjectionPolicy, SensorTypeMapping)
TEST(CSVInjectionPolicy, GenerateCompletionSignals)
TEST(CSVInjectionPolicy, HandleMalformedRows)
TEST(CSVInjectionPolicy, AllFiveSensorTypesCorrectly)
```

**Test Data**:
Create test CSV files for each sensor type:
- `test/data/accel_samples.csv` (3-axis accelerometer)
- `test/data/gyro_samples.csv` (3-axis gyroscope)
- `test/data/mag_samples.csv` (3-axis magnetometer)
- `test/data/baro_samples.csv` (altitude/pressure)
- `test/data/gps_samples.csv` (latitude/longitude/altitude)

---

#### 4.4 CommandPolicy Tests

**File**: `test/app/policies/test_command_policy.cpp` (NEW)

**What to Test**:
- Command registration
- Command dispatch
- Command execution
- Command result handling
- Error handling

**Test Cases**:
```cpp
TEST(CommandPolicy, RegisterCommand)
TEST(CommandPolicy, DispatchCommand)
TEST(CommandPolicy, ExecuteCommand)
TEST(CommandPolicy, HandleCommandErrors)
```

---

#### 4.5 CompletionPolicy Tests

**File**: `test/app/policies/test_completion_policy.cpp` (NEW)

**What to Test**:
- Completion signal detection
- Aggregation from multiple sources (5 sensors)
- Completion event generation
- Timeout handling
- Completion callback invocation

**Test Cases**:
```cpp
TEST(CompletionPolicy, DetectCompletionSignals)
TEST(CompletionPolicy, AggregateMultipleSources)
TEST(CompletionPolicy, TimeoutHandling)
TEST(CompletionPolicy, InvokeCompletionCallbacks)
TEST(CompletionPolicy, AllFiveSensorsRequired)
```

---

#### 4.6 CapabilityBus Tests

**File**: `test/graph/test_capability_bus.cpp` (NEW)

**What to Test**:
- Capability registration
- Capability lookup by type
- Type-erased storage/retrieval
- Thread safety
- Capability lifecycle

**Test Cases**:
```cpp
TEST(CapabilityBus, RegisterCapability)
TEST(CapabilityBus, LookupCapabilityByType)
TEST(CapabilityBus, TypeErasedRetrieval)
TEST(CapabilityBus, MultipleCapabilities)
TEST(CapabilityBus, ThreadSafety)
```

---

### Layer 5: Node Implementation

**Objective**: Verify individual node behavior, transformations, and message handling.

#### 5.1 DataInjectionNode Tests

**File**: `test/graph/nodes/test_data_injection_nodes.cpp` (NEW)

**What to Test**:
- Node initialization with CSV file
- Data extraction from CSV rows
- Message creation with correct payload
- Completion signal generation
- Rate limiting (10ms per row from config)
- Dual-port output (Port 0 data, Port 1 completion)
- All 5 sensor types

**Test Cases**:
```cpp
// Test each sensor type
TEST(DataInjectionAccelerometerNode, OutputCorrectMessageType)
TEST(DataInjectionAccelerometerNode, ExtractAccelerometerData)
TEST(DataInjectionAccelerometerNode, GenerateCompletionSignals)
TEST(DataInjectionAccelerometerNode, DualPortOutput)

TEST(DataInjectionGyroscopeNode, OutputCorrectMessageType)
TEST(DataInjectionGyroscopeNode, ExtractGyroscopeData)

TEST(DataInjectionMagnetometerNode, OutputCorrectMessageType)
TEST(DataInjectionMagnetometerNode, ExtractMagnetometerData)

TEST(DataInjectionBarometricNode, OutputCorrectMessageType)
TEST(DataInjectionBarometricNode, ExtractBarometricData)

TEST(DataInjectionGPSPositionNode, OutputCorrectMessageType)
TEST(DataInjectionGPSPositionNode, ExtractGPSData)

// Cross-cutting tests
TEST(DataInjectionNodes, RateLimitingEnforcement)
TEST(DataInjectionNodes, AllSensorTypesSupported)
```

---

#### 5.2 FlightFSMNode Tests

**File**: `test/graph/nodes/test_flight_fsm_node.cpp` (NEW)

**What to Test**:
- State machine state transitions
- Multi-input fusion (5 sensor types)
- Kalman filter integration
- StateVector output generation
- Filter parameter application
- Estimator configuration

**Test Cases**:
```cpp
TEST(FlightFSMNode, InitializeWithConfiguration)
TEST(FlightFSMNode, ProcessAccelerometerData)
TEST(FlightFSMNode, ProcessGyroscopeData)
TEST(FlightFSMNode, ProcessMagnetometerData)
TEST(FlightFSMNode, ProcessBarometricData)
TEST(FlightFSMNode, ProcessGPSData)
TEST(FlightFSMNode, ProducesStateVectorOutput)
TEST(FlightFSMNode, KalmanFilterIntegration)
TEST(FlightFSMNode, MultiSensorFusion)
TEST(FlightFSMNode, HandlesOutOfOrderInputs)
TEST(FlightFSMNode, ConsistentStateProduction)
```

---

#### 5.3 AltitudeFusionNode Tests

**File**: `test/graph/nodes/test_altitude_fusion_node.cpp` (NEW)

**What to Test**:
- StateVector input processing
- Barometer vs GPS altitude blending
- Confidence weighting (0.7 barometer, 0.3 GPS)
- Outlier detection (3.0 threshold)
- Adaptive weighting
- Fused StateVector output

**Test Cases**:
```cpp
TEST(AltitudeFusionNode, BlendAltitudeEstimates)
TEST(AltitudeFusionNode, ApplyConfidenceWeights)
TEST(AltitudeFusionNode, OutlierDetection)
TEST(AltitudeFusionNode, AdaptiveWeighting)
TEST(AltitudeFusionNode, ProducesStateVectorOutput)
TEST(AltitudeFusionNode, PreserveNonAltitudeFields)
```

---

#### 5.4 EstimationPipelineNode Tests

**File**: `test/graph/nodes/test_estimation_pipeline_node.cpp` (NEW)

**What to Test**:
- Advanced filtering
- Outlier detection and removal
- Adaptive weighting
- Altitude/velocity refinement
- Process/measurement noise handling
- Time delta application
- Metrics generation

**Test Cases**:
```cpp
TEST(EstimationPipelineNode, ApplyAdvancedFiltering)
TEST(EstimationPipelineNode, OutlierDetection)
TEST(EstimationPipelineNode, AdaptiveWeighting)
TEST(EstimationPipelineNode, RefinedStateOutput)
TEST(EstimationPipelineNode, NoiseHandling)
TEST(EstimationPipelineNode, MetricsGeneration)
```

---

#### 5.5 FlightMonitorNode Tests

**File**: `test/graph/nodes/test_flight_monitor_node.cpp` (NEW)

**What to Test**:
- State vector input processing
- Phase-adaptive feedback generation
- State transition tracking
- Feedback rate limiting (10 Hz from config)
- PhaseAdaptiveFeedbackMsg output
- Metrics tracking

**Test Cases**:
```cpp
TEST(FlightMonitorNode, ProcessStateVector)
TEST(FlightMonitorNode, GeneratePhaseAdaptiveFeedback)
TEST(FlightMonitorNode, TrackStateTransitions)
TEST(FlightMonitorNode, RateLimitFeedback)
TEST(FlightMonitorNode, ProduceFeedbackMessages)
```

---

#### 5.6 FeedbackTestSinkNode Tests

**File**: `test/graph/nodes/test_feedback_sink_node.cpp` (NEW)

**What to Test**:
- Input message capture
- Message buffering (1024 capacity from config)
- Result logging
- Buffer management
- Test result retrieval for verification

**Test Cases**:
```cpp
TEST(FeedbackTestSinkNode, CaptureMessages)
TEST(FeedbackTestSinkNode, BufferMessages)
TEST(FeedbackTestSinkNode, LogResults)
TEST(FeedbackTestSinkNode, RetrieveResultsForVerification)
```

---

#### 5.7 CompletionAggregatorNode Tests

**File**: `test/graph/nodes/test_completion_aggregator_node.cpp` (NEW)

**What to Test**:
- Completion signal input (5 ports, one per sensor)
- Signal aggregation
- Timeout handling (30s from config)
- All-sensors-required policy
- Completion event generation
- Metrics tracking

**Test Cases**:
```cpp
TEST(CompletionAggregatorNode, AcceptCompletionSignals)
TEST(CompletionAggregatorNode, AggregateAllFiveSensors)
TEST(CompletionAggregatorNode, TimeoutHandling)
TEST(CompletionAggregatorNode, AllSensorsRequiredPolicy)
TEST(CompletionAggregatorNode, GenerateCompletionEvent)
TEST(CompletionAggregatorNode, PartialCompletion_Waits)
TEST(CompletionAggregatorNode, PartialCompletion_Timeout)
```

---

### Layer 6: End-to-End Integration (Graph Execution)

**Objective**: Verify complete graph execution pipeline with all components working together.

#### 6.1 Complete Pipeline Execution Tests

**File**: `test/integration/test_phase2_complete_pipeline.cpp` (NEW - CRITICAL)

**What to Test**:
- Load Phase 2 config completely
- Execute graph with CSV injection
- Verify all 11 nodes initialize correctly
- Execute main pipeline (9 edges)
- Execute completion path (5 edges)
- Track metrics throughout execution
- Verify final output in sink nodes
- Ensure no deadlocks
- Verify completion aggregator receives all 5 signals
- Check execution time is within timeout

**Test Scenario**:
```
1. Load config/graph_csv_pipeline_dual_port_integration.json
2. Initialize all 11 nodes
3. Start executor with 4 threads
4. Inject CSV data via CSVInjectionPolicy (5 sensors × 10-100 rows each)
5. Main pipeline processes: CSV→FSM→AltitudeFusion→EstimationPipeline→FlightMonitor→FeedbackSink
6. Completion path processes: CSV (Port 1)→CompletionAggregator
7. Verify:
   - All messages flow correctly
   - StateVector evolves through pipeline
   - Altitude fused correctly (0.7 baro, 0.3 GPS)
   - Estimation pipeline refines estimates
   - Flight monitor generates feedback
   - All 5 completion signals received
   - No deadlocks or errors
   - Execution completes within 10s timeout
   - Metrics show correct operation
```

**Test Cases**:
```cpp
TEST(Phase2PipelineIntegration, LoadConfiguration)
TEST(Phase2PipelineIntegration, InitializeAllNodes)
TEST(Phase2PipelineIntegration, ExecuteMainPipeline)
TEST(Phase2PipelineIntegration, ExecuteCompletionPath)
TEST(Phase2PipelineIntegration, CSVDataFlowCorrectly)
TEST(Phase2PipelineIntegration, DualPortArchitecture)
TEST(Phase2PipelineIntegration, AltitudeFusionBlending)
TEST(Phase2PipelineIntegration, EstimationRefinement)
TEST(Phase2PipelineIntegration, CompletionAggregation)
TEST(Phase2PipelineIntegration, NoDeadlocks)
TEST(Phase2PipelineIntegration, MetricsAccurate)
TEST(Phase2PipelineIntegration, CompletesWithinTimeout)
```

---

## Test Data Strategy

### CSV Test Data Files

Create minimal but representative test data in `test/data/`:

**1. Accelerometer Data** (`accel_samples.csv`)
```
timestamp,x_accel,y_accel,z_accel
0.000,0.10,0.05,-9.81
0.010,0.12,0.04,-9.80
0.020,0.11,0.06,-9.82
```

**2. Gyroscope Data** (`gyro_samples.csv`)
```
timestamp,x_rate,y_rate,z_rate
0.000,0.001,0.002,0.001
0.010,0.001,0.002,0.001
0.020,0.001,0.002,0.001
```

**3. Magnetometer Data** (`mag_samples.csv`)
```
timestamp,x_mag,y_mag,z_mag
0.000,24000,2000,41000
0.010,24100,2100,41100
0.020,24000,2000,41000
```

**4. Barometric Data** (`baro_samples.csv`)
```
timestamp,altitude_m,pressure_pa
0.000,100.0,101325
0.010,100.1,101320
0.020,100.0,101325
```

**5. GPS Data** (`gps_samples.csv`)
```
timestamp,latitude,longitude,altitude_m
0.000,37.7749,-122.4194,50.0
0.100,37.7750,-122.4195,50.5
0.200,37.7751,-122.4196,50.0
```

---

## Threading & Concurrency Testing

### Thread Safety Strategy

**1. ThreadPool Concurrency**:
- Test 4-thread execution (from config)
- Verify no race conditions
- Confirm all tasks execute
- Verify completion signals

**2. ActiveQueue Stress**:
- Multiple producers, multiple consumers
- Varying queue depths
- Backpressure scenarios

**3. Policy Execution**:
- Policies run in executor threads
- No cross-thread state corruption
- Proper synchronization

**4. Deadlock Testing**:
- Enable deadlock detection (from config)
- Set timeout to 10000ms (from config)
- Verify watchdog detects hangs

---

## Metrics Capture Strategy

### What to Measure

**Per Node**:
- Execution count
- Total execution time
- Average execution time
- Error count
- Messages processed
- Messages produced
- Backpressure events

**Per Edge**:
- Messages routed
- Transfer time
- Buffer utilization
- Dropped messages (if any)

**Per Policy**:
- Invocation count
- Execution time
- Errors/exceptions

**Overall**:
- Total execution time
- Deadlock detections (should be 0)
- Task rejections (should be 0)
- Completion signal count (should be 5)

---

## CMakeLists.txt Changes

### New Test Targets to Add

```cmake
# Layer 1: Core Components
add_executable(test_core_type_info
    core/test_type_info.cpp
)

# Layer 2: Graph Infrastructure
add_executable(test_graph_infrastructure
    graph/test_graph_config_parser.cpp
    graph/test_json_dynamic_graph_loader.cpp
    graph/test_node_factory.cpp
    graph/test_edge_registry.cpp
    graph/test_port_types.cpp
)

# Layer 3: Execution Engine
add_executable(test_graph_executor
    graph/test_graph_executor.cpp
    graph/test_execution_policy_chain.cpp
)

# Layer 4: Policies & Capabilities
add_executable(test_policies
    app/policies/test_metrics_policy.cpp
    app/policies/test_data_injection_policy.cpp
    app/policies/test_csv_injection_policy.cpp
    app/policies/test_command_policy.cpp
    app/policies/test_completion_policy.cpp
    graph/test_capability_bus.cpp
)

# Layer 5: Nodes
add_executable(test_nodes
    graph/nodes/test_data_injection_nodes.cpp
    graph/nodes/test_flight_fsm_node.cpp
    graph/nodes/test_altitude_fusion_node.cpp
    graph/nodes/test_estimation_pipeline_node.cpp
    graph/nodes/test_flight_monitor_node.cpp
    graph/nodes/test_feedback_sink_node.cpp
    graph/nodes/test_completion_aggregator_node.cpp
)

# Layer 6: Integration
add_executable(test_phase2_integration
    integration/test_phase2_complete_pipeline.cpp
)

# Meta-target: All non-UI tests
add_custom_target(test_non_ui
    COMMAND ${CMAKE_CTEST_COMMAND} -R ".*" -V
    DEPENDS test_core_type_info test_graph_infrastructure test_graph_executor
            test_policies test_nodes test_phase2_integration
)
```

---

## Execution Plan

### Phase 1: Foundation (Weeks 1-2)
- [ ] Verify existing ThreadPool and ActiveQueue tests pass
- [ ] Add TypeInfo unit tests
- [ ] Add GraphConfigParser tests
- [ ] Add JsonDynamicGraphLoader tests

### Phase 2: Graph Engine (Weeks 2-3)
- [ ] Add NodeFactory tests
- [ ] Add EdgeRegistry tests
- [ ] Add PortTypes tests
- [ ] Add GraphExecutor tests (CRITICAL)

### Phase 3: Policies & Capabilities (Weeks 3-4)
- [ ] Add MetricsPolicy tests
- [ ] Add DataInjectionPolicy tests
- [ ] Add CSVInjectionPolicy tests
- [ ] Add CommandPolicy tests
- [ ] Add CompletionPolicy tests
- [ ] Add CapabilityBus tests

### Phase 4: Node Implementation (Weeks 4-5)
- [ ] Add DataInjectionNode tests (all 5 types)
- [ ] Add FlightFSMNode tests
- [ ] Add AltitudeFusionNode tests
- [ ] Add EstimationPipelineNode tests
- [ ] Add FlightMonitorNode tests
- [ ] Add FeedbackTestSinkNode tests
- [ ] Add CompletionAggregatorNode tests

### Phase 5: Integration (Week 6)
- [ ] Add Phase 2 complete pipeline integration tests
- [ ] Verify all 11 nodes + 14 edges work together
- [ ] Performance profiling
- [ ] Coverage analysis

---

## Success Criteria

1. **All tests pass** with green status
2. **Code coverage** ≥ 85% for non-UI components
3. **No flaky tests** (pass consistently)
4. **No deadlocks** in integration tests
5. **Execution time** within 10s timeout for Phase 2 pipeline
6. **Metrics accuracy** verified against expected values
7. **All edge cases** covered:
   - Empty queues
   - Full buffers
   - Missing CSV files
   - Malformed JSON
   - Node initialization failures
   - Message type mismatches

---

## Test Fixture and Helper Utilities

### Common Test Fixtures

**GraphTestFixture**: Base fixture for all graph tests
- Loads Phase 2 config
- Creates GraphManager
- Creates NodeFactory
- Provides helper methods for assertions

**NodeTestFixture**: Base for node tests
- Creates isolated node
- Provides mock input/output ports
- Tracks produced messages

**PolicyTestFixture**: Base for policy tests
- Creates CapabilityBus
- Registers mock capabilities
- Tracks policy invocations

### Helper Functions

```cpp
// In test/graph/test_helpers.hpp

// Load and validate config
GraphConfig LoadPhase2Config();

// Create complete graph from config
std::shared_ptr<GraphManager> BuildPhase2Graph();

// Execute graph with timeout
ExecutionResult ExecuteGraphWithTimeout(
    GraphExecutor& executor,
    std::chrono::seconds timeout
);

// Verify message flow
void VerifyMessagePath(
    const std::vector<std::shared_ptr<Message>>& messages,
    const std::string& expected_path
);

// Validate output sink
bool ValidateSinkOutput(
    const FeedbackTestSinkNode& sink,
    const std::vector<PhaseAdaptiveFeedbackMsg>& expected
);
```

---

## Continuous Integration Considerations

- Run all tests in GitHub Actions on every commit
- Collect coverage metrics (gcov/lcov)
- Generate coverage reports
- Fail PR if coverage drops below 85%
- Fail PR if any test times out
- Run integration tests with sanitizers (ASan, TSan)
- Generate performance benchmarks

---

## Notes for Implementation

1. **Use mocking strategically**: Mock external dependencies (files, network) but test real graph behavior
2. **Avoid timing-dependent tests**: Use condition variables, not sleep
3. **Test isolated components first**: Work up from Layer 1 to Layer 6
4. **Leverage Phase 2 config**: Use it as golden standard for integration tests
5. **Document test assumptions**: Why each test exists and what it verifies
6. **Keep fixtures lightweight**: Fast test execution is critical
7. **Reuse test data**: Share CSV files across multiple tests
