# Rocket Recovery Deployment System - LLM Generation Prompt

## System Context

You are designing a **Rocket Recovery Deployment System** for real-time control and monitoring of recovery mechanisms during flight and landing phases. This system must integrate with an existing **Graph Execution Engine** (gdashboard) that provides:

- Real-time metrics collection via callbacks
- CLI-based command execution with step control
- Data injection for sensor simulation
- Plugin architecture for extensible node types
- 30 FPS responsive UI for operator feedback

---

## Core Requirements

### 1. Safety & Reliability

**Safety-Critical Constraints**:
- All deployment decisions must require **explicit operator confirmation** (no automatic deployment)
- System must handle **graceful degradation** if any sensor fails
- Must maintain **failsafe state** (safe to deploy at any time, no system-induced failures)
- All state transitions must be **logged and auditable**
- Deployment confirmation must be **time-gated** (cannot deploy twice within 500ms)

**Reliability**:
- Zero data loss on sensor readings
- Sub-100ms latency for deployment command execution
- Recovery state must be **recoverable** from logs
- All critical operations must have **redundant pathways**

### 2. System Architecture

**Three-Layer Design** (matching gdashboard):

#### Layer 1: Sensor & Data Collection
- Altitude sensor (barometric pressure, GPS)
- Accelerometer (descent rate, shock detection)
- Gyroscope (orientation, tumble detection)
- Temperature, battery voltage, system health
- **Output**: Metrics via `MetricsCapability` callback pattern

#### Layer 2: Recovery Decision Engine
- **State Machine**: Idle → Armed → Detecting → Deploying → Deployed → Complete
- **Decision Policy**: Evaluate altitude, descent rate, orientation, time-of-flight
- **Deployment Triggers**: 
  - Automatic: Apogee detection (descent rate crosses zero), low altitude threshold
  - Manual: Operator command with confirmation
- **Output**: Commands via CLI (`deploy <mechanism> <force>`)

#### Layer 3: Deployment Control & Feedback
- Solenoid control signals (redundant channels)
- Pyro igniter firing (primary + backup)
- Deployment sequence timing
- Real-time feedback to UI
- **Output**: Deployment status via metrics

### 3. Deployment Mechanisms

Support **multiple recovery mechanisms**:

```
Mechanism Types:
├─ Drogue Chute (high altitude, stabilization)
│  └─ Trigger: Apogee + stable orientation
├─ Main Chute (low altitude, descent control)
│  └─ Trigger: Low altitude threshold + stable velocity
├─ Grid Fins (supersonic descent control)
│  └─ Trigger: Hypersonic detection + stable orientation
└─ Airbag System (landing shock absorption)
    └─ Trigger: Impact detection + ground proximity
```

**Deployment States**:
```
Mechanism::State
├─ ARMED: Ready to deploy, awaiting trigger
├─ PRIMED: All safety checks passed, confirmation waiting
├─ DEPLOYING: Actuators firing, sequence in progress
├─ DEPLOYED: Mechanism confirmed deployed (feedback sensor)
└─ USED: Mechanism exhausted, cannot refire
```

### 4. Operator Interaction Model

**CLI Commands** (integrate with gdashboard):

```bash
# Status queries
status                          # Show current state, sensor readings
recovery status                 # Show all mechanisms + states
recovery sensors               # Dump all sensor values
recovery logs [--since 30s]    # Show deployment log tail

# Manual deployment (explicit operator control)
recovery arm <mechanism>       # Arm mechanism for deployment
recovery ready <mechanism>     # Check if ready to deploy
recovery deploy <mechanism> --confirm  # Deploy with explicit confirmation

# Safety operations
recovery disarm <mechanism>    # Safely disarm mechanism
recovery failsafe check        # Verify all failsafes active
recovery reset                 # Reset state machine (ground only)

# Simulation/Testing
recovery inject <sensor> <value>  # Inject test data
recovery simulate <profile>       # Run pre-recorded flight profile
recovery test <mechanism>         # Self-test mechanism readiness
```

### 5. State Machine & Decision Logic

**Simplified Flowchart**:

```
[IDLE] 
  ↓ (operator arms system)
[ARMED] ← waiting for flight signature
  ↓ (acceleration detected, time > 0)
[DETECTING] ← evaluating altitude, descent rate
  ├─ Apogee detected + orientation stable? → [PRIMED for drogue]
  ├─ Low altitude + descent rate nominal? → [PRIMED for main]
  ├─ Impact detected + velocity > threshold? → [PRIMED for airbag]
  └─ Any sensor invalid? → [FAILSAFE] (emergency deploy)
  ↓ (operator confirms deployment)
[DEPLOYING] → fire solenoids → wait for feedback
  ↓ (feedback sensor confirms)
[DEPLOYED] → log success → enable next mechanism
  ↓ (landed)
[COMPLETE] → summarize recovery → await operator shutdown
```

### 6. Metrics & Telemetry

**Real-time Metrics** (30 FPS update rate):

```
Sensor Metrics:
├─ altitude_m (meters above ground)
├─ descent_rate_m_s (velocity, negative = down)
├─ acceleration_g (raw acceleration)
├─ orientation_pitch_deg (nose angle)
├─ orientation_roll_deg (spin axis)
├─ temperature_c (airframe temperature)
└─ battery_v (power supply voltage)

Derived Metrics:
├─ mach_number (from altitude + speed estimation)
├─ apogee_detected (boolean, time at descent_rate = 0)
├─ time_of_flight_s (elapsed seconds)
├─ state_name (current state enum)
├─ next_trigger_reason (predicted next event)
└─ failsafe_active (boolean, emergency condition)

Deployment Metrics:
├─ mechanism_state (ARMED/PRIMED/DEPLOYING/DEPLOYED)
├─ confidence_pct (0-100% that trigger is correct)
├─ attempts_remaining (1-3, depends on redundancy)
└─ last_event_timestamp (when last state change occurred)
```

### 7. Testing & Validation

**Test Cases Required**:

1. **Unit Tests** (mechanism isolation):
   - Drogue deployment at apogee detection
   - Main chute deployment at low altitude
   - Airbag deployment on impact
   - Failsafe activation on sensor loss

2. **Integration Tests** (full flight simulation):
   - Normal flight profile: Launch → Apogee → Main chute → Landing
   - Sensor failure: Barometer fails at 500m → use accelerometer fallback
   - Operator intervention: Manual deployment override
   - Emergency: Multiple sensors fail simultaneously → failsafe

3. **Stress Tests**:
   - Rapid state transitions (10 deployments/second)
   - Sensor noise (±10% variance on all readings)
   - Dropped network packets (10% loss simulation)
   - Delayed confirmations (1-2 second latency)

4. **Deployment Readiness Checklist**:
   - [ ] All mechanisms test-fired
   - [ ] Redundant channels verified
   - [ ] Failsafe tested (confirmed safe to trigger anytime)
   - [ ] Operator trained on CLI commands
   - [ ] Logs reviewed for anomalies
   - [ ] Weather/range safety cleared

### 8. Data Structures & Interfaces

**Core Classes/Interfaces**:

```cpp
// Mechanism definition
struct RecoveryMechanism {
    string name;                    // "drogue", "main", "airbag"
    MechanismState state;           // ARMED, PRIMED, etc.
    float deployment_altitude_m;    // trigger threshold
    float deployment_confidence;    // 0-100%
    int redundant_channels;         // 1, 2, or 3
    timestamp last_deployed;        // when last fired
};

// Sensor reading
struct SensorReading {
    float altitude_m;
    float descent_rate_m_s;
    float acceleration_g;
    float orientation_pitch_deg;
    timestamp received_at;
    string sensor_id;               // "barometer_1", "accel_2"
};

// Decision output
struct DeploymentDecision {
    MechanismType mechanism;
    DeploymentReason reason;        // APOGEE_DETECTED, MANUAL_OPERATOR, etc.
    float confidence_pct;           // 0-100
    string log_message;
    bool requires_confirmation;
};

// Capability interface
class RecoveryDeploymentCapability : public ICapability {
    virtual vector<RecoveryMechanism> GetMechanisms() = 0;
    virtual DeploymentDecision EvaluateDeployment(vector<SensorReading>) = 0;
    virtual CommandResult Deploy(string mechanism, string confirmation_token) = 0;
    virtual void OnSensorUpdate(SensorReading) = 0;  // Callback from metrics
};
```

### 9. Error Handling & Failsafe

**Failure Modes & Responses**:

| Failure Mode | Detection | Response | Severity |
|---|---|---|---|
| Barometer fails | Reading invalid > 10 frames | Switch to accelerometer | Medium |
| Accelerometer fails | Range out of physics | Use barometer only | Medium |
| Both fail | Can't estimate altitude | FAILSAFE: deploy all | Critical |
| Solenoid no-fire | Continuity check fails | Try redundant channel | Critical |
| Both solenoids fail | Redundancy exhausted | Manual recovery system | Emergency |
| Software crash | Watchdog timeout | Reset state machine | Critical |
| Network loss | No metrics > 5 seconds | Use cached state | Low |

**Failsafe Deployment**:
- If any critical sensor fails AND altitude < threshold → auto-deploy main chute
- If all sensors fail → deploy everything (safe but not optimal)
- Operator override: `recovery failsafe trigger` (emergency manual)

### 10. Integration with gdashboard

**Plugin Node Types** to create:

```
recovery_sensor_node
├─ Input: Raw I2C/SPI sensor data
└─ Output: MetricsCapability updates (altitude, descent_rate, etc.)

recovery_decision_node
├─ Input: Sensor metrics (via MetricsCapability callback)
├─ State: State machine + decision logic
└─ Output: Deployment decision (via capability event)

recovery_executor_node
├─ Input: Operator CLI command + decision
├─ Action: Fire solenoids, trigger pyros
└─ Output: Deployment status + feedback metrics

recovery_validator_node
├─ Input: All system states
├─ Logic: Safety checks, failsafe verification
└─ Output: Ready/Not-ready status
```

**Dashboard UI Elements**:
- Real-time altitude graph (sparkline from gdashboard Phase 7)
- Mechanism status panel (4 tiles, one per mechanism)
- Decision confidence indicator
- Operator confirmation button (large, safe to click)
- Event log (last 20 deployments)

---

## Deliverables

### Code Artifacts

1. **RecoveryDeploymentCapability.hpp** (interface definition, ~150 lines)
   - Sensor callbacks
   - Decision engine
   - Deployment executor

2. **RecoveryStateMachine.cpp** (state logic, ~300 lines)
   - State transitions
   - Trigger evaluation
   - Failsafe logic

3. **RecoveryMechanismController.hpp** (hardware abstraction, ~200 lines)
   - Solenoid firing
   - Redundancy management
   - Feedback reading

4. **recovery_plugin.cpp** (gdashboard integration, ~400 lines)
   - Sensor node implementation
   - Decision node implementation
   - CLI command handler

5. **test_recovery_deployment.cpp** (comprehensive tests, ~500 lines)
   - Unit tests per mechanism
   - Integration tests for flight profiles
   - Failsafe validation tests

### Documentation Artifacts

1. **RECOVERY_SYSTEM_ARCHITECTURE.md** (full specification, ~2000 lines)
   - Design decisions
   - Deployment procedures
   - Safety analysis

2. **RECOVERY_OPERATOR_MANUAL.md** (operator guide, ~500 lines)
   - CLI commands
   - Emergency procedures
   - Troubleshooting

3. **RECOVERY_TEST_PLAN.md** (validation approach, ~300 lines)
   - Test cases
   - Success criteria
   - Sign-off checklist

---

## Success Criteria

- ✅ All mechanisms deploy on correct triggers (apogee, low altitude, impact)
- ✅ Zero unintended deployments
- ✅ Operator can confirm/override any deployment
- ✅ Failsafe activates within 100ms of sensor failure
- ✅ All state transitions logged and auditable
- ✅ 230+ test cases passing (same rigor as gdashboard)
- ✅ Real-time metrics updating at 30 FPS
- ✅ CLI commands responsive (<200ms latency)
- ✅ Operator training materials complete
- ✅ Flight-ready safety certification achieved

---

## Implementation Notes

**Technology Stack** (consistent with gdashboard):
- Language: C++23
- Testing: GTest (comprehensive suite)
- Logging: log4cxx (all decisions logged)
- Configuration: JSON (deployment profiles)
- Build: CMake (modular architecture)
- UI: Integration with gdashboard NCurses UI

**Development Phases**:
1. **Phase 1**: Core state machine + 3 mechanisms
2. **Phase 2**: Sensor integration + metrics callbacks
3. **Phase 3**: CLI commands + operator interface
4. **Phase 4**: Failsafe logic + emergency procedures
5. **Phase 5**: Comprehensive testing + validation
6. **Phase 6**: Documentation + operator training
7. **Phase 7**: Flight trials + certification

**Safety Review Gate**: Before Phase 7, complete:
- [ ] Formal hazard analysis
- [ ] Redundancy verification
- [ ] Failsafe testing (confirmed safe at all times)
- [ ] Operator training sign-off
- [ ] Legal/regulatory compliance

---

## Example Flight Scenario

```
T+0s: Operator runs 'recovery arm drogue' via CLI
     → System enters ARMED state, waits for launch

T+2s: Acceleration detected (a > 1g)
     → State → DETECTING
     → Metrics: alt=50m, descent_rate=-5 m/s

T+45s: Altitude peaks, descent_rate crosses 0
     → Apogee detected!
     → Decision: Deploy drogue
     → Confidence: 98%
     → Operator sees: "Ready to deploy drogue? [YES/NO]"
     → Operator clicks YES (or CLI: recovery deploy drogue --confirm)

T+45.1s: Solenoid fires, drogue ejects
     → Feedback sensor confirms deployment
     → Metrics: mechanism_state=DEPLOYED
     → State → ARMED (ready for main at low altitude)

T+80s: Altitude drops below 500m, descent_rate stable
     → Decision: Deploy main chute
     → Confidence: 95%
     → Operator confirms

T+85s: Main chute deploys
     → Descent rate: 5 m/s (normal)
     → ETA to ground: 100 seconds

T+168s: Altitude < 1m, velocity < 2 m/s
     → Landing detected
     → State → COMPLETE
     → Summary: "Successful recovery. 2/2 mechanisms deployed."
     → Logs saved to /logs/recovery_2026-02-14_14:32:45.json
```

---

## Questions for Refinement

Before implementation, clarify:

1. **How many recovery mechanisms** do you need? (currently: 3)
2. **What altitude range** and **descent rates** are typical?
3. **How many redundant channels** per mechanism? (1, 2, or 3?)
4. **Is GPS available** for ground-relative altitude?
5. **What's the target descent rate** for safe landing?
6. **Should operator confirmation be time-limited?** (e.g., 30-second window)
7. **Any vehicle-specific constraints?** (e.g., mass, power budget)

---

**Generated for**: gdashboard Rocket Recovery System Integration  
**Target**: Production-ready, safety-certified deployment  
**Owner**: Flight Systems Team

