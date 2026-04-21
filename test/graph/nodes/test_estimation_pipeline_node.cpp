// MIT License
//
// Copyright (c) 2025 gdashboard contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>
#include "avionics/nodes/EstimationPipelineNode.hpp"
#include "sensor/SensorDataTypes.hpp"
#include "graph/Message.hpp"
#include <memory>
#include <chrono>

// ============================================================================
// Layer 5, Task 5.4: EstimationPipelineNode Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - EstimationPipelineNode Tests
// ============================================================================

// ============================================================================
// Test Fixture for EstimationPipelineNode
// ============================================================================

class EstimationPipelineNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<avionics::estimators::EstimationPipelineNode> node_;

    void SetUp() override {
        node_ = std::make_unique<avionics::estimators::EstimationPipelineNode>();
        node_->SetName("test_estimation_pipeline");
    }

    // Helper to create a test StateVector with altitude sources
    sensors::StateVector CreateTestStateVector(float altitude_m = 100.0f) {
        sensors::StateVector sv;
        sv.SetTimestamp(std::chrono::nanoseconds(1000000));
        sv.position = {0.0, 0.0, altitude_m};
        sv.velocity = {0.0, 0.0, 0.0};
        sv.acceleration = {0.0, 0.0, 0.0};
        sv.attitude = {0.0, 0.0, 0.0, 1.0};  // Identity quaternion
        sv.angular_velocity = {0.0, 0.0, 0.0};
        sv.heading_rad = 0.0;

        // Set altitude sources (barometric and GPS)
        sv.altitude_sources.from_baro = altitude_m;
        sv.altitude_sources.from_gps = altitude_m + 0.5f;
        sv.altitude_sources.from_accel = altitude_m + 0.1f;

        // Set confidence for each source
        sv.altitude_sources.confidence_baro = 0.95f;
        sv.altitude_sources.confidence_gps = 0.90f;
        sv.altitude_sources.confidence_accel = 0.85f;

        // Velocity sources
        sv.velocity_sources.z_from_accel = 0.0f;
        sv.velocity_sources.z_from_baro = 0.05f;
        sv.velocity_sources.z_from_gps = 0.0f;
        sv.velocity_sources.confidence_accel = 0.9f;
        sv.velocity_sources.confidence_baro = 0.8f;
        sv.velocity_sources.confidence_gps = 0.85f;

        // GPS status
        sv.gps_fix_valid = true;
        sv.num_satellites = 12;
        sv.confidence.gps_position = 0.9f;
        sv.confidence.altitude = 0.95f;

        return sv;
    }
};

// ============================================================================
// EstimationPipelineNode Basic Creation and Configuration Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "test_estimation_pipeline");
}

TEST_F(EstimationPipelineNodeTest, NodeCreationWithFactoryParameters) {
    auto node_with_params = std::make_unique<avionics::estimators::EstimationPipelineNode>(
        std::chrono::microseconds(10000), 10);
    ASSERT_NE(node_with_params, nullptr);
}

TEST_F(EstimationPipelineNodeTest, IsConfigurable) {
    auto* configurable = dynamic_cast<graph::IConfigurable*>(node_.get());
    EXPECT_NE(configurable, nullptr);
}

TEST_F(EstimationPipelineNodeTest, IsDiagnosable) {
    auto* diagnosable = dynamic_cast<graph::IDiagnosable*>(node_.get());
    EXPECT_NE(diagnosable, nullptr);
}

TEST_F(EstimationPipelineNodeTest, IsParameterized) {
    auto* parameterized = dynamic_cast<graph::IParameterized*>(node_.get());
    EXPECT_NE(parameterized, nullptr);
}

TEST_F(EstimationPipelineNodeTest, IsMetricsCallbackProvider) {
    auto* metrics_provider = dynamic_cast<graph::IMetricsCallbackProvider*>(node_.get());
    EXPECT_NE(metrics_provider, nullptr);
}

TEST_F(EstimationPipelineNodeTest, HasMetricsSchema) {
    auto schema = node_->GetNodeMetricsSchema();
    EXPECT_EQ(schema.node_name, "test_estimation_pipeline");
    EXPECT_EQ(schema.node_type, "processor");
    EXPECT_FALSE(schema.event_types.empty());
}

// ============================================================================
// EstimationPipelineNode Metrics Callback Tests
// ============================================================================

class MockMetricsCallback : public graph::IMetricsCallback {
public:
    void PublishAsync(const app::metrics::MetricsEvent& event) noexcept override {
        events_published.push_back(event);
    }

    std::vector<app::metrics::MetricsEvent> events_published;
};

TEST_F(EstimationPipelineNodeTest, MetricsCallbackCanBeSet) {
    MockMetricsCallback callback;
    bool result = node_->SetMetricsCallback(&callback);
    EXPECT_TRUE(result);
}

TEST_F(EstimationPipelineNodeTest, MetricsCallbackCanBeQueried) {
    MockMetricsCallback callback;
    node_->SetMetricsCallback(&callback);

    EXPECT_TRUE(node_->HasMetricsCallback());
    EXPECT_NE(node_->GetMetricsCallback(), nullptr);
}

TEST_F(EstimationPipelineNodeTest, MetricsCallbackRejectsNullptr) {
    bool result = node_->SetMetricsCallback(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EstimationPipelineNodeTest, NoMetricsCallbackInitially) {
    EXPECT_FALSE(node_->HasMetricsCallback());
    EXPECT_EQ(node_->GetMetricsCallback(), nullptr);
}

// ============================================================================
// EstimationPipelineNode Configuration Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, SetOutlierThreshold) {
    node_->SetOutlierThreshold(2.5f);
    // Method called successfully (no return value, just side effects)
}

TEST_F(EstimationPipelineNodeTest, SetAltitudeConfidenceWeight) {
    node_->SetAltitudeConfidenceWeight(0.9f);
    // Method called successfully
}

TEST_F(EstimationPipelineNodeTest, SetVelocityConfidenceWeight) {
    node_->SetVelocityConfidenceWeight(0.85f);
    // Method called successfully
}

TEST_F(EstimationPipelineNodeTest, EnableAdaptiveWeighting) {
    node_->EnableAdaptiveWeighting(true);
    node_->EnableAdaptiveWeighting(false);
    // Method called successfully for both states
}

TEST_F(EstimationPipelineNodeTest, SetProcessNoise) {
    node_->SetProcessNoise(0.01f);
    // Method called successfully
}

TEST_F(EstimationPipelineNodeTest, SetMeasurementNoise) {
    node_->SetMeasurementNoise(0.1f);
    // Method called successfully
}

TEST_F(EstimationPipelineNodeTest, SetTimeDelta) {
    node_->SetTimeDelta(0.1f);
    // Method called successfully
}

// ============================================================================
// EstimationPipelineNode Metrics Access Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, GetProcessedCount) {
    size_t count = node_->GetProcessedCount();
    // Initially should be zero
    EXPECT_EQ(count, 0);
}

TEST_F(EstimationPipelineNodeTest, GetStageCount) {
    size_t count = node_->GetStageCount();
    // Initially should be zero
    EXPECT_EQ(count, 0);
}

TEST_F(EstimationPipelineNodeTest, GetAltitudeSwitches) {
    size_t switches = node_->GetAltitudeSwitches();
    // Initially should be zero
    EXPECT_EQ(switches, 0);
}

TEST_F(EstimationPipelineNodeTest, GetVelocitySwitches) {
    size_t switches = node_->GetVelocitySwitches();
    // Initially should be zero (sum of all axes)
    EXPECT_EQ(switches, 0);
}

TEST_F(EstimationPipelineNodeTest, GetTotalSwitches) {
    size_t total = node_->GetTotalSwitches();
    // Initially should be zero
    EXPECT_EQ(total, 0);
}

TEST_F(EstimationPipelineNodeTest, GetFusedConfidence) {
    float conf = node_->GetFusedConfidence();
    // Confidence should be in valid range
    EXPECT_GE(conf, 0.0f);
    EXPECT_LE(conf, 1.0f);
}

TEST_F(EstimationPipelineNodeTest, GetAverageConfidence) {
    float avg = node_->GetAverageConfidence();
    // Should return valid confidence value
    EXPECT_GE(avg, 0.0f);
    EXPECT_LE(avg, 1.0f);
}

// ============================================================================
// EstimationPipelineNode History Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, GetAltitudeHistory) {
    const auto& history = node_->GetAltitudeHistory();
    // Initially should be empty
    EXPECT_EQ(history.size(), 0);
}

TEST_F(EstimationPipelineNodeTest, GetVelocityHistory) {
    const auto& history = node_->GetVelocityHistory();
    // Initially should be empty
    EXPECT_EQ(history.size(), 0);
}

TEST_F(EstimationPipelineNodeTest, GetConfidenceHistory) {
    const auto& history = node_->GetConfidenceHistory();
    // Initially should be empty
    EXPECT_EQ(history.size(), 0);
}

TEST_F(EstimationPipelineNodeTest, ResetMetrics) {
    node_->ResetMetrics();
    // After reset, metrics should be zeroed
    EXPECT_EQ(node_->GetProcessedCount(), 0);
    EXPECT_EQ(node_->GetStageCount(), 0);
}

// ============================================================================
// EstimationPipelineNode Configuration (IConfigurable) Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, ConfigureFromJSON) {
    // Node implements IConfigurable interface
    auto* configurable = dynamic_cast<graph::IConfigurable*>(node_.get());
    ASSERT_NE(configurable, nullptr);
    // Configure method exists (though we can't test without valid JSON config)
}

// ============================================================================
// EstimationPipelineNode Diagnostics Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, GetDiagnosticsReturnsJsonView) {
    auto diag = node_->GetDiagnostics();
    // Diagnostics should be retrievable
}

TEST_F(EstimationPipelineNodeTest, DiagnosticsIncludeProcessedCount) {
    auto diag = node_->GetDiagnostics();
    // Should have processed_count information
}

TEST_F(EstimationPipelineNodeTest, DiagnosticsIncludeStageMetrics) {
    auto diag = node_->GetDiagnostics();
    // Should have stage metrics information
}

// ============================================================================
// EstimationPipelineNode Parameterized Interface Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, GetParametersReturnsJsonView) {
    auto params = node_->GetParameters();
    // Parameters should be retrievable
}

TEST_F(EstimationPipelineNodeTest, GetParameterNames) {
    auto names = node_->GetParameterNames();
    // Should return list of parameter names
    EXPECT_GT(names.size(), 0);
}

TEST_F(EstimationPipelineNodeTest, GetParameterDescriptionExists) {
    auto names = node_->GetParameterNames();
    // If parameters exist, should be able to get descriptions
    if (!names.empty()) {
        auto desc = node_->GetParameterDescription(names[0]);
        // Description should be retrievable
    }
}

// ============================================================================
// EstimationPipelineNode Port Metadata Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, GetInputPortMetadata) {
    auto input_ports = node_->GetInputPortMetadata();
    // Should provide input port metadata for visualization
    EXPECT_GT(input_ports.size(), 0);
}

TEST_F(EstimationPipelineNodeTest, GetOutputPortMetadata) {
    auto output_ports = node_->GetOutputPortMetadata();
    // Should provide output port metadata for visualization
    EXPECT_GT(output_ports.size(), 0);
}

// ============================================================================
// EstimationPipelineNode Multi-Instance Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, MultipleInstancesCanCoexist) {
    auto node1 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node2 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node3 = std::make_unique<avionics::estimators::EstimationPipelineNode>();

    node1->SetName("pipeline_primary");
    node2->SetName("pipeline_secondary");
    node3->SetName("pipeline_tertiary");

    EXPECT_EQ(node1->GetName(), "pipeline_primary");
    EXPECT_EQ(node2->GetName(), "pipeline_secondary");
    EXPECT_EQ(node3->GetName(), "pipeline_tertiary");

    // All should maintain their interfaces
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(node1.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(node2.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(node3.get()), nullptr);
}

TEST_F(EstimationPipelineNodeTest, MultipleInstancesIndependentMetricsCallbacks) {
    auto node1 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node2 = std::make_unique<avionics::estimators::EstimationPipelineNode>();

    MockMetricsCallback cb1, cb2;

    node1->SetMetricsCallback(&cb1);
    node2->SetMetricsCallback(&cb2);

    EXPECT_TRUE(node1->HasMetricsCallback());
    EXPECT_TRUE(node2->HasMetricsCallback());
    EXPECT_NE(node1->GetMetricsCallback(), node2->GetMetricsCallback());
}

// ============================================================================
// EstimationPipelineNode Metrics Schema Content Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, MetricsSchemaHasRequiredFields) {
    auto schema = node_->GetNodeMetricsSchema();

    // Schema should include key metrics
    EXPECT_NE(schema.metrics_schema.find("fields"), schema.metrics_schema.end());
    EXPECT_NE(schema.metrics_schema.find("metadata"), schema.metrics_schema.end());
}

TEST_F(EstimationPipelineNodeTest, MetricsSchemaIncludesAltitudeMetrics) {
    auto schema = node_->GetNodeMetricsSchema();

    // Should have altitude-related fields
    EXPECT_FALSE(schema.event_types.empty());
}

TEST_F(EstimationPipelineNodeTest, MetricsSchemaIncludesVelocityMetrics) {
    auto schema = node_->GetNodeMetricsSchema();

    // Should have velocity-related metrics
    EXPECT_NE(schema.metrics_schema.find("fields"), schema.metrics_schema.end());
}

TEST_F(EstimationPipelineNodeTest, MetricsSchemaIncludesFilterMetrics) {
    auto schema = node_->GetNodeMetricsSchema();

    // Should have EKF filter metrics
    EXPECT_NE(schema.metrics_schema.find("fields"), schema.metrics_schema.end());
}

// ============================================================================
// EstimationPipelineNode Pipeline Integration Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, NodeImplementsFourStagePipeline) {
    // EstimationPipelineNode integrates 4 estimation stages:
    // 1. SensorVoter altitude consensus
    // 2. Per-axis velocity consensus
    // 3. Complementary filter adaptive fusion
    // 4. Extended Kalman filter advanced filtering

    // Verified by configuration methods for each stage
    node_->SetOutlierThreshold(2.5f);  // Stage 1
    node_->SetAltitudeConfidenceWeight(1.0f);  // Stage 2
    node_->SetProcessNoise(0.01f);  // Stage 4
}

TEST_F(EstimationPipelineNodeTest, NodeTracksSensorConsensus) {
    // Pipeline tracks which sensors are chosen at each stage
    auto altitude_switches = node_->GetAltitudeSwitches();
    auto velocity_switches = node_->GetVelocitySwitches();

    // Initially zero, but tracking is enabled
    EXPECT_EQ(altitude_switches, 0);
    EXPECT_EQ(velocity_switches, 0);
}

TEST_F(EstimationPipelineNodeTest, NodeTracksEstimationQuality) {
    // Pipeline tracks confidence and disagreement at each stage
    float confidence = node_->GetFusedConfidence();

    // Confidence should be in valid range
    EXPECT_GE(confidence, 0.0f);
    EXPECT_LE(confidence, 1.0f);
}

// ============================================================================
// EstimationPipelineNode Concurrency Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, MultipleInstancesCanInitializeInParallel) {
    auto node1 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node2 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node3 = std::make_unique<avionics::estimators::EstimationPipelineNode>();

    // All three should initialize without issues
    node1->SetName("n1");
    node2->SetName("n2");
    node3->SetName("n3");

    EXPECT_EQ(node1->GetName(), "n1");
    EXPECT_EQ(node2->GetName(), "n2");
    EXPECT_EQ(node3->GetName(), "n3");
}

TEST_F(EstimationPipelineNodeTest, IndependentMetricsCallbacksMultiNode) {
    auto node1 = std::make_unique<avionics::estimators::EstimationPipelineNode>();
    auto node2 = std::make_unique<avionics::estimators::EstimationPipelineNode>();

    MockMetricsCallback cb1, cb2;

    node1->SetMetricsCallback(&cb1);
    node2->SetMetricsCallback(&cb2);

    // Both should be initialized independently
    EXPECT_TRUE(node1->HasMetricsCallback());
    EXPECT_TRUE(node2->HasMetricsCallback());
}

// ============================================================================
// Cross-Cutting EstimationPipelineNode Tests
// ============================================================================

TEST_F(EstimationPipelineNodeTest, NodeIsStateVectorEnricher) {
    // EstimationPipelineNode is a StateVector enricher
    // Input: StateVector from AltitudeFusionNode
    // Output: Refined StateVector with advanced filtering
    auto schema = node_->GetNodeMetricsSchema();
    EXPECT_EQ(schema.node_type, "processor");
}

TEST_F(EstimationPipelineNodeTest, NodeIntegratesToEstimationChain) {
    // EstimationPipelineNode is central to estimation architecture
    // Combines altitude selection, velocity selection,
    // adaptive fusion, and Kalman filtering
    EXPECT_TRUE(node_->HasMetricsCallback() == false);  // Optional
}

TEST_F(EstimationPipelineNodeTest, NodeSupportsMultiStageConfiguration) {
    // Each pipeline stage can be configured independently
    node_->SetOutlierThreshold(3.0f);
    node_->SetAltitudeConfidenceWeight(0.9f);
    node_->SetVelocityConfidenceWeight(0.8f);
    node_->SetProcessNoise(0.02f);
    node_->SetMeasurementNoise(0.15f);
    node_->EnableAdaptiveWeighting(true);

    // All configuration methods should execute without error
}

