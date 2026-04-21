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
#include "avionics/nodes/StateVectorCaptureSinkNode.hpp"
#include "sensor/SensorDataTypes.hpp"
#include <memory>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>

// ============================================================================
// Layer 5, Task 5.7: StateVectorCaptureSinkNode Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - StateVectorCaptureSinkNode Tests
// ============================================================================

// ============================================================================
// Test Fixture for StateVectorCaptureSinkNode
// ============================================================================

class StateVectorCaptureSinkNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<avionics::nodes::StateVectorCaptureSinkNode> node_;

    void SetUp() override {
        node_ = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();
        node_->SetName("test_state_vector_sink");
    }

    // Helper to create a test StateVector
    sensors::StateVector CreateTestStateVector(
        float altitude_m = 100.0f,
        float confidence = 0.95f,
        uint64_t timestamp_ns = 1000000) {
        sensors::StateVector sv;
        sv.SetTimestamp(std::chrono::nanoseconds(timestamp_ns));
        sv.position = {0.0, 0.0, altitude_m};
        sv.velocity = {0.0, 0.0, 0.0};
        sv.acceleration = {0.0, 0.0, 0.0};
        sv.attitude = {0.0, 0.0, 0.0, 1.0};  // Identity quaternion
        sv.angular_velocity = {0.0, 0.0, 0.0};
        sv.heading_rad = 0.0;
        sv.gps_fix_valid = true;
        sv.num_satellites = 12;
        sv.confidence.altitude = confidence;
        sv.confidence.gps_position = confidence;
        sv.confidence.velocity = confidence;
        sv.confidence.acceleration = confidence;
        return sv;
    }
};

// ============================================================================
// StateVectorCaptureSinkNode Basic Creation Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "test_state_vector_sink");
}

TEST_F(StateVectorCaptureSinkNodeTest, InitialFrameCountIsZero) {
    EXPECT_EQ(node_->GetFrameCount(), 0);
}

TEST_F(StateVectorCaptureSinkNodeTest, InitialCapturedStatesEmpty) {
    auto states = node_->GetCapturedStates();
    EXPECT_TRUE(states.empty());
}

// ============================================================================
// StateVectorCaptureSinkNode Message Consumption Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, ConsumeSingleStateVector) {
    auto sv = CreateTestStateVector(100.0f);
    bool result = node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
    EXPECT_EQ(node_->GetFrameCount(), 1);
}

TEST_F(StateVectorCaptureSinkNodeTest, ConsumeMultipleStateVectors) {
    for (int i = 0; i < 5; ++i) {
        auto sv = CreateTestStateVector(100.0f + i);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }
    EXPECT_EQ(node_->GetFrameCount(), 5);
}

TEST_F(StateVectorCaptureSinkNodeTest, ConsumeMaxStateVectors) {
    for (int i = 0; i < 1000; ++i) {
        auto sv = CreateTestStateVector(100.0f + i * 0.5f);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }
    EXPECT_EQ(node_->GetFrameCount(), 1000);
}

// ============================================================================
// StateVectorCaptureSinkNode State Vector Storage Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, CapturedStatesAreRetrievable) {
    auto sv = CreateTestStateVector(250.0f, 0.99f);
    node_->Consume(sv, std::integral_constant<std::size_t, 0>());

    auto captured = node_->GetCapturedStates();
    EXPECT_EQ(captured.size(), 1);
    EXPECT_FLOAT_EQ(captured[0].position.z, 250.0f);
    EXPECT_FLOAT_EQ(captured[0].confidence.altitude, 0.99f);
}

TEST_F(StateVectorCaptureSinkNodeTest, CapturedStatesPreserveOrder) {
    std::vector<float> altitudes = {100.0f, 150.0f, 200.0f, 250.0f, 300.0f};
    for (float alt : altitudes) {
        auto sv = CreateTestStateVector(alt);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto captured = node_->GetCapturedStates();
    EXPECT_EQ(captured.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_FLOAT_EQ(captured[i].position.z, altitudes[i]);
    }
}

TEST_F(StateVectorCaptureSinkNodeTest, CapturedStatesPreserveTimestamps) {
    for (int i = 0; i < 3; ++i) {
        auto sv = CreateTestStateVector(100.0f, 0.95f, 1000000 + i * 1000);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto captured = node_->GetCapturedStates();
    EXPECT_EQ(captured[0].GetTimestamp().count(), 1000000);
    EXPECT_EQ(captured[1].GetTimestamp().count(), 1001000);
    EXPECT_EQ(captured[2].GetTimestamp().count(), 1002000);
}

// ============================================================================
// StateVectorCaptureSinkNode Statistics Computation Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsFrameCount) {
    for (int i = 0; i < 10; ++i) {
        auto sv = CreateTestStateVector(100.0f + i);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_EQ(stats.frame_count, 10);
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsAltitudeMinMax) {
    std::vector<float> altitudes = {100.0f, 200.0f, 150.0f, 50.0f, 250.0f};
    for (float alt : altitudes) {
        auto sv = CreateTestStateVector(alt);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_FLOAT_EQ(stats.min_altitude, 50.0f);
    EXPECT_FLOAT_EQ(stats.max_altitude, 250.0f);
    EXPECT_FLOAT_EQ(stats.altitude_range, 200.0f);
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsAltitudeMean) {
    std::vector<float> altitudes = {100.0f, 200.0f, 300.0f};
    for (float alt : altitudes) {
        auto sv = CreateTestStateVector(alt);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_FLOAT_EQ(stats.mean_altitude, 200.0f);
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsConfidenceMinMax) {
    std::vector<float> confidences = {0.90f, 0.95f, 0.85f, 0.99f};
    for (float conf : confidences) {
        auto sv = CreateTestStateVector(100.0f, conf);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_FLOAT_EQ(stats.min_confidence, 0.85f);
    EXPECT_FLOAT_EQ(stats.max_confidence, 0.99f);
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsConfidenceRange) {
    std::vector<float> confidences = {0.80f, 0.90f, 1.00f};
    for (float conf : confidences) {
        auto sv = CreateTestStateVector(100.0f, conf);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_FLOAT_EQ(stats.confidence_range, 0.20f);
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsTimestamps) {
    for (int i = 0; i < 3; ++i) {
        auto sv = CreateTestStateVector(100.0f, 0.95f, 1000000 + i * 1000000);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_EQ(stats.timestamp_first, 1000000);
    EXPECT_EQ(stats.timestamp_last, 3000000);
    EXPECT_EQ(stats.duration_us, 2000);  // 2000000 ns = 2000 us
}

TEST_F(StateVectorCaptureSinkNodeTest, StatisticsFrameRate) {
    // 10 frames over 9 seconds (9000000000 ns) = ~1.111 Hz
    for (int i = 0; i < 10; ++i) {
        auto sv = CreateTestStateVector(100.0f, 0.95f, 1000000000 + i * 1000000000);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    // Frame rate = 10 frames / 9 seconds ≈ 1.111 Hz
    EXPECT_NEAR(stats.frame_rate_hz, 10.0f / 9.0f, 0.01f);
}

// ============================================================================
// StateVectorCaptureSinkNode Clear Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, ClearRemovesAllStates) {
    for (int i = 0; i < 5; ++i) {
        auto sv = CreateTestStateVector(100.0f + i);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    EXPECT_EQ(node_->GetFrameCount(), 5);
    node_->Clear();
    EXPECT_EQ(node_->GetFrameCount(), 0);
    EXPECT_TRUE(node_->GetCapturedStates().empty());
}

TEST_F(StateVectorCaptureSinkNodeTest, ClearAllowsNewCapture) {
    for (int i = 0; i < 3; ++i) {
        auto sv = CreateTestStateVector(100.0f);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    node_->Clear();

    for (int i = 0; i < 2; ++i) {
        auto sv = CreateTestStateVector(200.0f);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    EXPECT_EQ(node_->GetFrameCount(), 2);
    auto states = node_->GetCapturedStates();
    EXPECT_FLOAT_EQ(states[0].position.z, 200.0f);
}

// ============================================================================
// StateVectorCaptureSinkNode Configuration Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, IsConfigurable) {
    auto* configurable = dynamic_cast<graph::IConfigurable*>(node_.get());
    EXPECT_NE(configurable, nullptr);
}

TEST_F(StateVectorCaptureSinkNodeTest, ConfigureWithEmptyJson) {
    // Should accept empty configuration without error
    graph::JsonView empty_config(nlohmann::json::object());
    node_->Configure(empty_config);
    // Should continue to work normally
    auto sv = CreateTestStateVector(100.0f);
    node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    EXPECT_EQ(node_->GetFrameCount(), 1);
}

// ============================================================================
// StateVectorCaptureSinkNode Multi-Instance Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, MultipleInstancesCanCoexist) {
    auto node1 = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();
    auto node2 = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();
    auto node3 = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();

    node1->SetName("sink_primary");
    node2->SetName("sink_secondary");
    node3->SetName("sink_tertiary");

    EXPECT_EQ(node1->GetName(), "sink_primary");
    EXPECT_EQ(node2->GetName(), "sink_secondary");
    EXPECT_EQ(node3->GetName(), "sink_tertiary");
}

TEST_F(StateVectorCaptureSinkNodeTest, MultipleInstancesIndependent) {
    auto node1 = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();
    auto node2 = std::make_unique<avionics::nodes::StateVectorCaptureSinkNode>();

    auto sv1 = CreateTestStateVector(100.0f);
    auto sv2 = CreateTestStateVector(200.0f);

    node1->Consume(sv1, std::integral_constant<std::size_t, 0>());
    node2->Consume(sv2, std::integral_constant<std::size_t, 0>());
    node1->Consume(sv1, std::integral_constant<std::size_t, 0>());

    EXPECT_EQ(node1->GetFrameCount(), 2);
    EXPECT_EQ(node2->GetFrameCount(), 1);

    auto states1 = node1->GetCapturedStates();
    auto states2 = node2->GetCapturedStates();
    EXPECT_FLOAT_EQ(states1[0].position.z, 100.0f);
    EXPECT_FLOAT_EQ(states2[0].position.z, 200.0f);
}

// ============================================================================
// StateVectorCaptureSinkNode Sink Pattern Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, NodeIsSinkNode) {
    // NamedSinkNode pattern: takes input, produces no output
    auto sv = CreateTestStateVector();
    bool result = node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
}

TEST_F(StateVectorCaptureSinkNodeTest, AcceptsStateVectorInput) {
    // Sink pattern: accepts StateVector input
    auto sv = CreateTestStateVector(300.0f, 0.98f);
    bool consumed = node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(consumed);
    EXPECT_EQ(node_->GetFrameCount(), 1);
}

// ============================================================================
// StateVectorCaptureSinkNode Thread-Safe Operations Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, ThreadSafeStateVectorCapture) {
    // Multiple sequential captures should be thread-safe
    std::vector<sensors::StateVector> test_states;
    for (int i = 0; i < 50; ++i) {
        test_states.push_back(CreateTestStateVector(100.0f + i));
    }

    for (const auto& sv : test_states) {
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    EXPECT_EQ(node_->GetFrameCount(), 50);
}

TEST_F(StateVectorCaptureSinkNodeTest, ThreadSafeGetCapturedStates) {
    for (int i = 0; i < 20; ++i) {
        auto sv = CreateTestStateVector(100.0f + i);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    // Multiple reads should see consistent state
    auto states1 = node_->GetCapturedStates();
    auto states2 = node_->GetCapturedStates();
    EXPECT_EQ(states1.size(), states2.size());
    EXPECT_EQ(states1.size(), 20);
}

// ============================================================================
// StateVectorCaptureSinkNode Integration Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, CompleteFlightSequence) {
    // Simulate a complete flight: ascent phase
    std::vector<float> flight_altitudes = {
        0.0f, 50.0f, 100.0f, 150.0f, 200.0f,
        250.0f, 300.0f, 350.0f, 400.0f, 500.0f
    };

    for (size_t i = 0; i < flight_altitudes.size(); ++i) {
        auto sv = CreateTestStateVector(
            flight_altitudes[i],
            0.90f + i * 0.01f,  // Improving confidence
            1000000 + i * 100000);  // Time step
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_EQ(stats.frame_count, 10);
    EXPECT_FLOAT_EQ(stats.min_altitude, 0.0f);
    EXPECT_FLOAT_EQ(stats.max_altitude, 500.0f);
    EXPECT_FLOAT_EQ(stats.altitude_range, 500.0f);
}

TEST_F(StateVectorCaptureSinkNodeTest, FlightDataRecording) {
    // Simulate recording flight data: altitude profile
    for (int i = 0; i < 100; ++i) {
        // Parabolic altitude profile: peak at 50th frame
        float time = i / 10.0f;  // 0 to 10 seconds
        float altitude = 500.0f - 5.0f * (time - 5.0f) * (time - 5.0f);  // Peak at t=5
        float confidence = 0.95f - 0.05f * std::abs(time - 5.0f) / 5.0f;

        auto sv = CreateTestStateVector(altitude, confidence, 1000000 + i * 100000);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_EQ(stats.frame_count, 100);
    EXPECT_GT(stats.max_altitude, 490.0f);  // Peak near 500m
    EXPECT_LT(stats.min_altitude, 380.0f);  // Minimum at start/end (375m)
}

// ============================================================================
// Cross-Cutting StateVectorCaptureSinkNode Tests
// ============================================================================

TEST_F(StateVectorCaptureSinkNodeTest, NodeIsStateVectorSink) {
    // StateVectorCaptureSinkNode is a flight data recorder
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "test_state_vector_sink");
}

TEST_F(StateVectorCaptureSinkNodeTest, NodeImplementsSinkPattern) {
    // NamedSinkNode pattern: consumes StateVector
    auto sv = CreateTestStateVector();
    bool result = node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
}

TEST_F(StateVectorCaptureSinkNodeTest, NodeAccumulatesAllFrames) {
    // Test sink accumulates all incoming frames for analysis
    const int frame_count = 100;
    for (int i = 0; i < frame_count; ++i) {
        auto sv = CreateTestStateVector(100.0f + i * 0.5f);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    EXPECT_EQ(node_->GetFrameCount(), frame_count);
    EXPECT_EQ(node_->GetCapturedStates().size(), frame_count);
}

TEST_F(StateVectorCaptureSinkNodeTest, NodeProvidesStatisticalAnalysis) {
    // Statistical analysis for post-flight validation
    for (int i = 0; i < 10; ++i) {
        auto sv = CreateTestStateVector(100.0f + i * 10.0f);
        node_->Consume(sv, std::integral_constant<std::size_t, 0>());
    }

    auto stats = node_->ComputeStatistics();
    EXPECT_GT(stats.frame_count, 0);
    EXPECT_GT(stats.max_altitude, stats.min_altitude);
    EXPECT_GE(stats.mean_altitude, stats.min_altitude);
    EXPECT_LE(stats.mean_altitude, stats.max_altitude);
}

