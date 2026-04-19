# Rocket Recovery Deployment System - Graph Architecture Prompt

## System Context

You are implementing a **Rocket Recovery Deployment System** using the existing **gdashboard Graph Architecture**. This system will be deployed as **graph plugins** that execute within the existing graph execution engine, reusing established patterns for node execution, metrics publishing, and CLI command handling.

The goal is to create flight-critical recovery logic that integrates seamlessly with gdashboard's 3-layer architecture:
- **Layer 1**: Graph nodes for sensors, decision logic, and actuators
- **Layer 2**: Capabilities (MetricsCapability, GraphCapability) for inter-node communication
- **Layer 3**: Dashboard UI for real-time visualization and operator control

---

## Part 1: Understanding the Existing Graph Architecture

### 1.1 Core Patterns to Reuse

**Graph Execution Model** (from GraphExecutor.hpp):
```cpp
GraphManager graph;
// Add nodes to graph
graph.AddNode(sensor_node);          // Source
graph.AddNode(decision_node);        // Process
graph.AddNode(executor_node);        // Sink

// Connect ports
graph.Connect(sensor_node, 0, decision_node, 0);
graph.Connect(decision_node, 0, executor_node, 0);

GraphExecutor executor(graph);
executor.Start();
executor.Run();  // Blocking, executes until graph completes
```

**Node Lifecycle** (INode interface):
```cpp
enum LifecycleState {
    Uninitialized,
    Initialized,    // After Init()
    Started,        // After Start()
    Stopped,        // After Stop()
    Joined          // After Join()
};

// Every node implements:
- Init()              // One-time setup (no threads yet)
- Start()             // Begin port threads
- Execute()           // Main work (polled by executor)
- Stop()              // Graceful shutdown
- Join()              // Wait for completion
```

**Port System** (data flow between nodes):
```cpp
// Nodes have input and output ports
class MySensorNode : public INode {
    OutputPort<SensorReading> sensor_data_;  // Publishes readings
    
    bool Execute() override {
        SensorReading reading = ReadSensor();
        sensor_data_.Send(reading);  // Send to connected nodes
        return true;  // Continue executing
    }
};

class MyDecisionNode : public INode {
    InputPort<SensorReading> sensor_input_;
    OutputPort<DeploymentDecision> decision_output_;
    
    bool Execute() override {
        SensorReading reading = sensor_input_.Receive(timeout);
        auto decision = EvaluateDeployment(reading);
        decision_output_.Send(decision);
        return true;
    }
};
```

**Capabilities Pattern** (MetricsCapability, GraphCapability):
```cpp
// Nodes register metrics schemas
class MyNode : public IMetricsCallbackProvider {
    std::vector<MetricDescriptor> GetMetricDescriptors() override {
        return {
            {"altitude_m", "meters", 0, 100000},
            {"descent_rate_m_s", "m/s", -300, 0}
        };
    }
    
    void PublishMetrics(MetricsCapability* cap) {
        cap->OnMetricsUpdate("MyNode::altitude_m", altitude_);
        cap->OnMetricsUpdate("MyNode::descent_rate_m_s", descent_rate_);
    }
};

// Dashboard discovers and subscribes
auto metrics_cap = bus->GetCapability<MetricsCapability>();
auto schemas = metrics_cap->GetNodeMetricsSchemas();  // Discover
metrics_cap->RegisterMetricsCallback(
    [this](const MetricsEvent& event) {
        OnMetricsUpdate(event);  // Real-time updates
    }
);
```

**Execution Policies** (customize execution behavior):
```cpp
// Policies can inject data, modify behavior, etc.
class RecoveryDataInjectionPolicy : public IExecutionPolicy {
    ExecutionResult OnExecuteNode(INode* node) override {
        if (auto recovery_node = dynamic_cast<RecoveryNode*>(node)) {
            // Inject test data if in simulation mode
            if (simulation_mode_) {
                recovery_node->InjectSensorData(test_reading_);
            }
        }
        return ExecutionResult::Continue();
    }
};
```

### 1.2 Plugin Architecture

**Plugin Registration Pattern** (existing model):
```cpp
// plugins/recovery_system_plugin.cpp
extern "C" {
    std::shared_ptr<graph::INode> CreateRecoverySensorNode() {
        return std::make_shared<RecoverySensorNode>();
    }
    
    std::shared_ptr<graph::INode> CreateRecoveryDecisionNode() {
        return std::make_shared<RecoveryDecisionNode>();
    }
    
    std::shared_ptr<graph::INode> CreateRecoveryExecutorNode() {
        return std::make_shared<RecoveryExecutorNode>();
    }
}

// JSON config loads plugins
{
    "nodes": [
        {"type": "RecoverySensorNode", "name": "sensors"},
        {"type": "RecoveryDecisionNode", "name": "decisions"},
        {"type": "RecoveryExecutorNode", "name": "executor"}
    ],
    "connections": [
        {"from": "sensors", "to": "decisions"},
        {"from": "decisions", "to": "executor"}
    ]
}
```

---

## Part 2: Recovery System Node Architecture

### 2.1 Data Structures (Using Graph Ports)

**Sensor Port Data**:
```cpp
// Types to flow through ports
struct SensorReading {
    float altitude_m;
    float descent_rate_m_s;
    float acceleration_g;
    float pitch_deg;
    float roll_deg;
    std::chrono::microseconds timestamp;
    std::vector<std::string> failed_sensors;  // Which sensors failed
};

struct MechanismStatus {
    std::string mechanism_name;  // "drogue", "main", "airbag"
    enum State {ARMED, PRIMED, DEPLOYING, DEPLOYED, FAILED};
    State state;
    float confidence_pct;
    int redundant_channels_ok;
};

struct DeploymentCommand {
    std::string mechanism_name;
    bool fire_redundant_channel;  // Use backup if primary fails
    std::string operator_token;   // Authorization
    std::chrono::microseconds timestamp;
};

struct DeploymentFeedback {
    std::string mechanism_name;
    bool deployment_success;
    std::string reason;  // Why deployed or why failed
};
```

### 2.2 Node 1: RecoverySensorNode (Source)

**Purpose**: Read sensors, publish metrics, output SensorReading packets

```cpp
class RecoverySensorNode : public INode, public IMetricsCallbackProvider {
    // Port
    OutputPort<SensorReading> sensor_output_;
    
    // Config
    std::vector<I2CDevice> barometers_;     // Altitude sensors
    std::vector<I2CDevice> accelerometers_; // Acceleration sensors
    std::vector<I2CDevice> gyroscopes_;     // Orientation sensors
    
    // Lifecycle
    bool Init() override {
        // Initialize I2C devices, check connectivity
        return ValidateAllSensors();
    }
    
    bool Start() override {
        // Start polling threads
        return INode::Start();
    }
    
    // Main execution loop (called repeatedly by executor)
    bool Execute() override {
        SensorReading reading;
        
        // Read all sensors (with fallback if one fails)
        try {
            reading.altitude_m = ReadBarometer();
        } catch (...) {
            reading.failed_sensors.push_back("barometer");
            reading.altitude_m = EstimateAltitudeFromAccel();
        }
        
        try {
            reading.descent_rate_m_s = CalculateDescentRate();
        } catch (...) {
            reading.failed_sensors.push_back("descent_rate");
        }
        
        reading.timestamp = std::chrono::high_resolution_clock::now();
        
        // Send to next node
        sensor_output_.Send(reading);
        
        return true;  // Continue executing
    }
    
    // Metrics for dashboard
    std::vector<MetricDescriptor> GetMetricDescriptors() override {
        return {
            {"altitude_m", "meters", 0, 100000, AlertLevel::OK},
            {"descent_rate_m_s", "m/s", -300, 0, AlertLevel::OK},
            {"acceleration_g", "g", -10, 50, AlertLevel::OK},
            {"sensor_health", "count", 0, 6, AlertLevel::WARNING},
        };
    }
    
    void PublishMetrics(MetricsCapability* cap) override {
        cap->OnMetricsUpdate("RecoverySensorNode::altitude_m", 
                             current_reading_.altitude_m);
        cap->OnMetricsUpdate("RecoverySensorNode::descent_rate_m_s", 
                             current_reading_.descent_rate_m_s);
        cap->OnMetricsUpdate("RecoverySensorNode::sensor_health", 
                             SensorHealthScore());
    }
};
```

### 2.3 Node 2: RecoveryDecisionNode (Process)

**Purpose**: Evaluate deployment triggers, manage state machine, output commands

```cpp
class RecoveryDecisionNode : public INode, public IMetricsCallbackProvider {
    // Ports
    InputPort<SensorReading> sensor_input_;
    OutputPort<DeploymentCommand> command_output_;
    
    // State machine
    enum State {IDLE, ARMED, DETECTING, DEPLOYING, COMPLETE};
    State current_state_ = State::IDLE;
    
    // Mechanisms
    std::map<std::string, MechanismStatus> mechanisms_;  // drogue, main, airbag
    
    // Configuration
    float apogee_descent_rate_threshold_ = 0.5;  // m/s
    float main_chute_altitude_ = 500.0;          // meters
    float impact_altitude_ = 5.0;                // meters
    
    bool Init() override {
        // Initialize mechanisms from config
        mechanisms_["drogue"] = {
            .mechanism_name = "drogue",
            .state = State::ARMED,
            .confidence_pct = 0.0,
            .redundant_channels_ok = 2
        };
        mechanisms_["main"] = {...};
        mechanisms_["airbag"] = {...};
        return true;
    }
    
    bool Execute() override {
        // State machine loop
        SensorReading reading = sensor_input_.Receive(timeout_ms);
        
        if (!reading) return true;  // No data yet
        
        // Check for sensor failures → failsafe
        if (reading.failed_sensors.size() > 2) {
            TriggerFailsafe();
            return true;
        }
        
        // State transitions
        switch (current_state_) {
            case State::IDLE:
                HandleIdleState(reading);
                break;
            case State::ARMED:
                HandleArmedState(reading);
                break;
            case State::DETECTING:
                HandleDetectingState(reading);
                break;
            case State::DEPLOYING:
                HandleDeployingState(reading);
                break;
            case State::COMPLETE:
                HandleCompleteState();
                break;
        }
        
        return true;  // Continue executing
    }
    
    void HandleDetectingState(const SensorReading& reading) {
        // Apogee detection
        if (reading.descent_rate_m_s > apogee_descent_rate_threshold_ &&
            IsOrientationStable(reading)) {
            
            auto decision = EvaluateDeployment("drogue", reading);
            if (decision.confidence_pct > 90.0) {
                PromptOperatorDeployment("drogue", decision);
                current_state_ = State::DEPLOYING;
            }
        }
        
        // Low altitude detection
        if (reading.altitude_m < main_chute_altitude_ &&
            reading.descent_rate_m_s < 0) {
            
            auto decision = EvaluateDeployment("main", reading);
            if (decision.confidence_pct > 85.0) {
                PromptOperatorDeployment("main", decision);
            }
        }
    }
    
    void PromptOperatorDeployment(const std::string& mechanism, 
                                   const DeploymentDecision& decision) {
        // Send command to executor node, dashboard will show prompt
        DeploymentCommand cmd;
        cmd.mechanism_name = mechanism;
        cmd.confidence_pct = decision.confidence_pct;
        cmd.requires_confirmation = true;
        
        command_output_.Send(cmd);
        
        // Publish metrics so dashboard sees decision
        PublishDecisionMetrics(mechanism, decision);
    }
    
    DeploymentDecision EvaluateDeployment(const std::string& mechanism,
                                          const SensorReading& reading) {
        // Logic to calculate confidence
        float confidence = 100.0;
        
        if (mechanism == "drogue") {
            // Check apogee
            bool apogee_detected = (reading.descent_rate_m_s > 0);
            confidence *= (apogee_detected ? 1.0 : 0.5);
            
            // Check orientation
            bool oriented = (std::abs(reading.pitch_deg) < 45.0);
            confidence *= (oriented ? 1.0 : 0.7);
        }
        
        return {mechanism, confidence, "evaluated"};
    }
    
    std::vector<MetricDescriptor> GetMetricDescriptors() override {
        return {
            {"state", "enum", 0, 5, AlertLevel::OK},
            {"drogue_confidence", "%", 0, 100, AlertLevel::WARNING},
            {"main_confidence", "%", 0, 100, AlertLevel::WARNING},
            {"airbag_confidence", "%", 0, 100, AlertLevel::WARNING},
            {"next_trigger", "timestamp", 0, 1e9, AlertLevel::OK},
        };
    }
    
    void PublishMetrics(MetricsCapability* cap) override {
        cap->OnMetricsUpdate("RecoveryDecisionNode::state", 
                             static_cast<int>(current_state_));
        cap->OnMetricsUpdate("RecoveryDecisionNode::drogue_confidence",
                             mechanisms_["drogue"].confidence_pct);
        cap->OnMetricsUpdate("RecoveryDecisionNode::main_confidence",
                             mechanisms_["main"].confidence_pct);
    }
};
```

### 2.4 Node 3: RecoveryExecutorNode (Sink)

**Purpose**: Handle operator confirmation, fire solenoids, provide feedback

```cpp
class RecoveryExecutorNode : public INode, public IMetricsCallbackProvider {
    // Ports
    InputPort<DeploymentCommand> command_input_;
    OutputPort<DeploymentFeedback> feedback_output_;
    
    // Hardware interface
    std::map<std::string, HardwareSolenoid> solenoids_;  // Primary + backup
    
    bool Init() override {
        // Initialize solenoid controllers, verify continuity
        for (auto& [name, solenoid] : solenoids_) {
            solenoid.VerifyContinuity();  // Safety check
        }
        return true;
    }
    
    bool Execute() override {
        DeploymentCommand cmd = command_input_.Receive(timeout_ms);
        
        if (!cmd) return true;
        
        // Verify operator token/confirmation
        if (!VerifyOperatorConfirmation(cmd.operator_token)) {
            return true;  // Ignore unconfirmed commands
        }
        
        // Check failsafe conditions
        if (!PassFailsafeCheck()) {
            PublishFailsafeTriggered();
            return true;
        }
        
        // Execute deployment
        bool success = DeployMechanism(cmd);
        
        // Provide feedback
        DeploymentFeedback feedback;
        feedback.mechanism_name = cmd.mechanism_name;
        feedback.deployment_success = success;
        feedback.reason = success ? "Deployed" : "Failed - check logs";
        
        feedback_output_.Send(feedback);
        PublishDeploymentResult(feedback);
        
        return true;
    }
    
    bool DeployMechanism(const DeploymentCommand& cmd) {
        auto it = solenoids_.find(cmd.mechanism_name);
        if (it == solenoids_.end()) return false;
        
        auto& solenoid = it->second;
        
        try {
            // Fire primary channel
            solenoid.FirePrimary();
            
            // Verify deployment via feedback sensor
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (solenoid.ConfirmDeployment()) {
                return true;
            }
            
            // Try redundant channel if primary fails
            if (cmd.fire_redundant_channel) {
                solenoid.FireSecondary();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return solenoid.ConfirmDeployment();
            }
            
            return false;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Deployment failed: " << e.what();
            return false;
        }
    }
    
    std::vector<MetricDescriptor> GetMetricDescriptors() override {
        return {
            {"last_deployment", "timestamp", 0, 1e9, AlertLevel::OK},
            {"deployment_success_rate", "%", 0, 100, AlertLevel::OK},
            {"solenoid_continuity", "count", 0, 6, AlertLevel::WARNING},
        };
    }
    
    void PublishMetrics(MetricsCapability* cap) override {
        cap->OnMetricsUpdate("RecoveryExecutorNode::last_deployment",
                             last_deployment_time_);
        cap->OnMetricsUpdate("RecoveryExecutorNode::deployment_success_rate",
                             success_rate_pct_);
    }
};
```

---

## Part 3: Integration with gdashboard

### 3.1 Configuration (JSON)

```json
{
  "graph": {
    "execution_mode": "continuous",
    "nodes": [
      {
        "id": "recovery_sensors",
        "type": "RecoverySensorNode",
        "config": {
          "barometer_i2c_addr": "0x77",
          "accelerometer_range_g": 50,
          "gyroscope_range_deg_s": 500
        }
      },
      {
        "id": "recovery_decision",
        "type": "RecoveryDecisionNode",
        "config": {
          "apogee_threshold_m_s": 0.5,
          "main_chute_altitude_m": 500,
          "impact_altitude_m": 5
        }
      },
      {
        "id": "recovery_executor",
        "type": "RecoveryExecutorNode",
        "config": {
          "solenoid_gpio_primary": [17, 18, 22],   // Drogue, main, airbag
          "solenoid_gpio_backup": [23, 24, 25],
          "feedback_gpio": [27, 28, 29]
        }
      }
    ],
    "connections": [
      {"from": "recovery_sensors", "port": 0, "to": "recovery_decision", "port": 0},
      {"from": "recovery_decision", "port": 0, "to": "recovery_executor", "port": 0}
    ]
  },
  "capabilities": {
    "MetricsCapability": {
      "enabled": true,
      "update_interval_hz": 30
    },
    "GraphCapability": {
      "enabled": true
    }
  }
}
```

### 3.2 CLI Commands (via CommandRegistry)

```cpp
// In Dashboard or gdashboard main
registry->RegisterCommand(
    "recovery",
    "Rocket recovery system control",
    "recovery status | arm | deploy | failsafe",
    [](const std::vector<std::string>& args) {
        return HandleRecoveryCommand(args);
    }
);

// Usage examples:
//   recovery status                 # Show all mechanisms
//   recovery arm drogue             # Prepare drogue for deployment
//   recovery deploy drogue --confirm # Fire drogue with confirmation
//   recovery failsafe trigger        # Manual emergency deploy
```

### 3.3 Dashboard UI Integration

**Metrics tiles automatically created from schemas**:
- RecoverySensorNode metrics tile (altitude, descent_rate, acceleration)
- RecoveryDecisionNode metrics tile (state, confidence levels)
- RecoveryExecutorNode metrics tile (deployment status, success rate)

**Add UI elements**:
```cpp
// In Dashboard::Init()
// The recovery nodes' metrics will auto-populate

// For operator confirmation, extend CommandWindow
// to show deployment prompts with YES/NO buttons
```

---

## Part 4: Data Flow Diagram

```
┌─────────────────────┐
│ Hardware Sensors    │
├─────────────────────┤
│ Barometer, Accel,   │
│ Gyro, etc.          │
└──────────┬──────────┘
           │ I2C/GPIO
           ▼
┌─────────────────────────────────────────┐
│ RecoverySensorNode (Source)             │
├─────────────────────────────────────────┤
│ Reads: altitude, descent_rate,          │
│        acceleration, orientation        │
│                                         │
│ Publishes: SensorReading to port 0     │
│ Metrics: altitude_m, descent_rate_m_s  │
└──────────┬──────────────────────────────┘
           │ SensorReading
           │ (via port)
           ▼
┌─────────────────────────────────────────┐
│ RecoveryDecisionNode (Process)          │
├─────────────────────────────────────────┤
│ Evaluates: Apogee, low altitude,        │
│           impact, sensor failures       │
│                                         │
│ State Machine: IDLE→ARMED→DETECTING     │
│                  →DEPLOYING→COMPLETE    │
│                                         │
│ Receives: SensorReading                 │
│ Publishes: DeploymentCommand to port 0  │
│ Metrics: state, confidence %, etc.      │
└──────────┬──────────────────────────────┘
           │ DeploymentCommand
           │ (via port)
           ▼
┌──────────────────────────────────────────┐
│ RecoveryExecutorNode (Sink)              │
├──────────────────────────────────────────┤
│ Receives: DeploymentCommand              │
│ Requires: Operator confirmation token    │
│                                          │
│ Actions:                                 │
│ 1. Verify operator authorization        │
│ 2. Check failsafe conditions             │
│ 3. Fire solenoid (primary + backup)      │
│ 4. Verify deployment via feedback        │
│                                          │
│ Publishes: DeploymentFeedback            │
│ Metrics: deployment status, success rate │
└──────────┬───────────────────────────────┘
           │
           ▼
      Hardware
   (Solenoids,
     Ignitors)

ALSO:
All nodes → MetricsCapability → Dashboard UI
           (30 FPS updates)     (Tiles, graphs)

Operator → Dashboard UI → GraphCapability → CLI handler
(Keyboard input)         (recovery commands)
```

---

## Part 5: Test Strategy (Using GTest)

```cpp
#include <gtest/gtest.h>
#include "recovery_system.hpp"

class RecoverySensorNodeTest : public ::testing::Test {
    void SetUp() override {
        node_ = std::make_shared<RecoverySensorNode>();
        ASSERT_TRUE(node_->Init());
        ASSERT_TRUE(node_->Start());
    }
};

TEST_F(RecoverySensorNodeTest, PublishesSensorReadingsOnExecute) {
    EXPECT_TRUE(node_->Execute());
    auto reading = sensor_output_.Receive(1000);
    EXPECT_TRUE(reading);
    EXPECT_GT(reading.altitude_m, 0);
}

TEST_F(RecoverySensorNodeTest, HandlesBarometerFailureGracefully) {
    // Mock barometer to fail
    barometer_->SetFailed(true);
    
    // Should use alternate estimation
    EXPECT_TRUE(node_->Execute());
    auto reading = sensor_output_.Receive(1000);
    EXPECT_TRUE(reading);
    EXPECT_TRUE(std::find(reading.failed_sensors.begin(),
                         reading.failed_sensors.end(),
                         "barometer") != reading.failed_sensors.end());
}

class RecoveryDecisionNodeTest : public ::testing::Test {
    // Integration test: simulate full flight profile
};

TEST_F(RecoveryDecisionNodeTest, DeploysApogeeDroguOnDetection) {
    // Simulate ascent
    SensorReading ascent{.altitude_m = 100, .descent_rate_m_s = -50};
    command_input_.Send(ascent);
    EXPECT_TRUE(decision_node_->Execute());
    
    // Simulate apogee
    SensorReading apogee{.altitude_m = 1000, .descent_rate_m_s = 0.1};
    command_input_.Send(apogee);
    EXPECT_TRUE(decision_node_->Execute());
    
    // Should decide to deploy drogue
    auto cmd = command_output_.Receive(1000);
    EXPECT_EQ(cmd.mechanism_name, "drogue");
    EXPECT_GT(cmd.confidence_pct, 90);
}

TEST_F(RecoveryDecisionNodeTest, FailsafeOnMultipleSensorFailures) {
    // Simulate all sensors failing
    SensorReading failed{
        .altitude_m = 0,  // Invalid
        .descent_rate_m_s = 0,
        .failed_sensors = {"barometer", "accelerometer", "gyroscope"}
    };
    
    command_input_.Send(failed);
    EXPECT_TRUE(decision_node_->Execute());
    
    // Should trigger failsafe
    EXPECT_EQ(decision_node_->GetState(), State::FAILSAFE);
}
```

---

## Part 6: Implementation Checklist

### Phase 1: Core Node Implementation
- [ ] RecoverySensorNode reads hardware sensors
- [ ] RecoveryDecisionNode implements state machine
- [ ] RecoveryExecutorNode fires solenoids
- [ ] All nodes implement INode lifecycle
- [ ] All nodes publish metrics via MetricsCapability
- [ ] Ports correctly connected in test graph

### Phase 2: Integration
- [ ] Plugin registration and dynamic loading
- [ ] JSON configuration parsing
- [ ] MetricsCapability integration with dashboard
- [ ] CLI command handlers
- [ ] Operator confirmation flow

### Phase 3: Testing & Validation
- [ ] 50+ unit tests for each node
- [ ] Integration tests for full flight profiles
- [ ] Failsafe verification tests
- [ ] Hardware simulation in test environment
- [ ] 85%+ code coverage (match gdashboard standard)

### Phase 4: Safety & Documentation
- [ ] Formal hazard analysis
- [ ] Failsafe certification
- [ ] Operator manual with CLI examples
- [ ] System architecture documentation
- [ ] Flight readiness checklist

---

## Key Advantages of Using Graph Architecture

1. **Decoupled Nodes**: Each node (sensor, decision, executor) is independent and testable
2. **Port-Based Data Flow**: Type-safe, zero-copy data movement between nodes
3. **Metrics Integration**: Auto-discovery and dashboard visualization
4. **Capability Pattern**: Clean inter-node communication
5. **Reusable Infrastructure**: Leverages existing gdashboard patterns
6. **Modular Testing**: Test each node in isolation before integration
7. **CLI Integration**: Existing CommandRegistry for operator control
8. **Real-time UI**: Existing Dashboard for 30 FPS updates

---

## File Structure

```
recovery_system/
├─ include/
│  ├─ RecoverySensorNode.hpp
│  ├─ RecoveryDecisionNode.hpp
│  ├─ RecoveryExecutorNode.hpp
│  ├─ RecoveryTypes.hpp       // SensorReading, DeploymentCommand, etc.
│  └─ RecoveryCapability.hpp  // Optional: extend MetricsCapability
├─ src/
│  ├─ RecoverySensorNode.cpp
│  ├─ RecoveryDecisionNode.cpp
│  ├─ RecoveryExecutorNode.cpp
│  └─ recovery_plugin.cpp     // Plugin registration
├─ test/
│  ├─ test_recovery_sensor_node.cpp
│  ├─ test_recovery_decision_node.cpp
│  ├─ test_recovery_executor_node.cpp
│  ├─ test_recovery_integration.cpp
│  └─ recovery_test_fixtures.hpp
├─ config/
│  └─ recovery_system.json     // Example configuration
└─ docs/
   ├─ RECOVERY_SYSTEM_ARCHITECTURE.md
   ├─ RECOVERY_OPERATOR_MANUAL.md
   └─ RECOVERY_TEST_PLAN.md
```

---

**Implementation Status**: Ready for development  
**Technology Stack**: C++23, GTest, NCurses (via gdashboard), CMake  
**Target Maturity**: Phase 7 (Production-ready, 230+ tests passing)

