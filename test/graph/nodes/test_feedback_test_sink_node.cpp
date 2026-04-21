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
#include "avionics/nodes/FeedbackTestSinkNode.hpp"
#include "avionics/messages/AvionicsMessages.hpp"
#include <memory>
#include <chrono>

// ============================================================================
// Layer 5, Task 5.6: FeedbackTestSinkNode Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - FeedbackTestSinkNode Tests
// ============================================================================

// ============================================================================
// Test Fixture for FeedbackTestSinkNode
// ============================================================================

class FeedbackTestSinkNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<avionics::FeedbackTestSinkNode> node_;

    void SetUp() override {
        node_ = std::make_unique<avionics::FeedbackTestSinkNode>();
        node_->SetName("test_feedback_sink");
    }

    // Helper to create a test PhaseAdaptiveFeedbackMsg
    avionics::PhaseAdaptiveFeedbackMsg CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase phase =
            avionics::FlightPhaseClassifier::FlightPhase::Rail,
        float altitude_m = 100.0f,
        float velocity_mps = 0.0f) {
        avionics::PhaseAdaptiveFeedbackMsg msg;
        msg.SetTimestamp(std::chrono::nanoseconds(1000000));
        msg.current_phase = phase;
        msg.previous_phase = phase;
        msg.altitude_m = altitude_m;
        msg.velocity_mps = velocity_mps;
        msg.acceleration_mps2 = 0.0f;
        msg.time_in_phase_us = 0;
        msg.is_phase_transition = false;
        msg.transition_progress = 1.0f;
        msg.sample_count = 1;
        return msg;
    }
};

// ============================================================================
// FeedbackTestSinkNode Basic Creation Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "test_feedback_sink");
}

TEST_F(FeedbackTestSinkNodeTest, DefaultConstructor) {
    auto default_node = std::make_unique<avionics::FeedbackTestSinkNode>();
    ASSERT_NE(default_node, nullptr);
    // Default constructor creates a node; name may or may not be set
    // but the node is properly initialized
    EXPECT_EQ(default_node->GetMessagesReceived(), 0);
}

TEST_F(FeedbackTestSinkNodeTest, InitialMessageCountIsZero) {
    uint64_t count = node_->GetMessagesReceived();
    EXPECT_EQ(count, 0);
}

TEST_F(FeedbackTestSinkNodeTest, InitialMessagesEmpty) {
    const auto& messages = node_->GetMessages();
    EXPECT_TRUE(messages.empty());
}

// ============================================================================
// FeedbackTestSinkNode Message Consumption Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, ConsumeSingleMessage) {
    auto msg = CreateTestFeedbackMsg();
    bool result = node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
    EXPECT_EQ(node_->GetMessagesReceived(), 1);
}

TEST_F(FeedbackTestSinkNodeTest, ConsumeMultipleMessages) {
    for (int i = 0; i < 5; ++i) {
        auto msg = CreateTestFeedbackMsg();
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }
    EXPECT_EQ(node_->GetMessagesReceived(), 5);
}

TEST_F(FeedbackTestSinkNodeTest, ConsumeMaxMessages) {
    // Test with large number of messages
    for (int i = 0; i < 1000; ++i) {
        auto msg = CreateTestFeedbackMsg(
            avionics::FlightPhaseClassifier::FlightPhase::Rail,
            100.0f + i);
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }
    EXPECT_EQ(node_->GetMessagesReceived(), 1000);
}

// ============================================================================
// FeedbackTestSinkNode Message Storage Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, StoredMessagesAreRetrievable) {
    auto msg = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        250.0f);
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored.size(), 1);
    EXPECT_EQ(stored[0].altitude_m, 250.0f);
}

TEST_F(FeedbackTestSinkNodeTest, MessagesStoredInOrder) {
    float altitudes[] = {100.0f, 150.0f, 200.0f, 250.0f};
    for (float alt : altitudes) {
        auto msg = CreateTestFeedbackMsg(
            avionics::FlightPhaseClassifier::FlightPhase::Rail,
            alt);
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    const auto& stored = node_->GetMessages();
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(stored[i].altitude_m, altitudes[i]);
    }
}

TEST_F(FeedbackTestSinkNodeTest, PhaseTransitionMessagesStored) {
    auto msg1 = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail);
    msg1.is_phase_transition = false;
    node_->Consume(msg1, std::integral_constant<std::size_t, 0>());

    auto msg2 = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Coast);
    msg2.is_phase_transition = true;
    node_->Consume(msg2, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FALSE(stored[0].is_phase_transition);
    EXPECT_TRUE(stored[1].is_phase_transition);
}

// ============================================================================
// FeedbackTestSinkNode Flight Phase Tracking Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, TrackFlightPhaseRail) {
    auto msg = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail);
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored[0].current_phase,
              avionics::FlightPhaseClassifier::FlightPhase::Rail);
}

TEST_F(FeedbackTestSinkNodeTest, TrackFlightPhaseCoast) {
    auto msg = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Coast);
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored[0].current_phase,
              avionics::FlightPhaseClassifier::FlightPhase::Coast);
}

TEST_F(FeedbackTestSinkNodeTest, MultiplePhaseSequence) {
    std::vector<avionics::FlightPhaseClassifier::FlightPhase> phases = {
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        avionics::FlightPhaseClassifier::FlightPhase::Coast,
        avionics::FlightPhaseClassifier::FlightPhase::Descent
    };

    for (auto phase : phases) {
        auto msg = CreateTestFeedbackMsg(phase);
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored.size(), 3);
    EXPECT_EQ(stored[0].current_phase, phases[0]);
    EXPECT_EQ(stored[1].current_phase, phases[1]);
    EXPECT_EQ(stored[2].current_phase, phases[2]);
}

// ============================================================================
// FeedbackTestSinkNode State Information Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, TrackAltitudeValues) {
    auto msg = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        500.5f);
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FLOAT_EQ(stored[0].altitude_m, 500.5f);
}

TEST_F(FeedbackTestSinkNodeTest, TrackVelocityValues) {
    auto msg = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        100.0f);
    msg.velocity_mps = 50.0f;
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FLOAT_EQ(stored[0].velocity_mps, 50.0f);
}

TEST_F(FeedbackTestSinkNodeTest, TrackAccelerationValues) {
    auto msg = CreateTestFeedbackMsg();
    msg.acceleration_mps2 = 9.81f;
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FLOAT_EQ(stored[0].acceleration_mps2, 9.81f);
}

// ============================================================================
// FeedbackTestSinkNode Adaptive Parameter Tracking Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, TrackParametersInMessage) {
    auto msg = CreateTestFeedbackMsg();
    msg.parameters.p_gain = 0.5f;
    msg.parameters.i_gain = 0.1f;
    msg.parameters.d_gain = 0.2f;
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FLOAT_EQ(stored[0].parameters.p_gain, 0.5f);
    EXPECT_FLOAT_EQ(stored[0].parameters.i_gain, 0.1f);
    EXPECT_FLOAT_EQ(stored[0].parameters.d_gain, 0.2f);
}

TEST_F(FeedbackTestSinkNodeTest, TrackTransitionProgress) {
    auto msg = CreateTestFeedbackMsg();
    msg.transition_progress = 0.5f;
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_FLOAT_EQ(stored[0].transition_progress, 0.5f);
}

TEST_F(FeedbackTestSinkNodeTest, TrackSampleCount) {
    auto msg = CreateTestFeedbackMsg();
    msg.sample_count = 42;
    node_->Consume(msg, std::integral_constant<std::size_t, 0>());

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored[0].sample_count, 42u);
}

// ============================================================================
// FeedbackTestSinkNode Multi-Instance Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, MultipleInstancesCanCoexist) {
    auto node1 = std::make_unique<avionics::FeedbackTestSinkNode>();
    auto node2 = std::make_unique<avionics::FeedbackTestSinkNode>();
    auto node3 = std::make_unique<avionics::FeedbackTestSinkNode>();

    node1->SetName("sink_primary");
    node2->SetName("sink_secondary");
    node3->SetName("sink_tertiary");

    EXPECT_EQ(node1->GetName(), "sink_primary");
    EXPECT_EQ(node2->GetName(), "sink_secondary");
    EXPECT_EQ(node3->GetName(), "sink_tertiary");
}

TEST_F(FeedbackTestSinkNodeTest, MultipleInstancesIndependent) {
    auto node1 = std::make_unique<avionics::FeedbackTestSinkNode>();
    auto node2 = std::make_unique<avionics::FeedbackTestSinkNode>();

    auto msg1 = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        100.0f);
    auto msg2 = CreateTestFeedbackMsg(
        avionics::FlightPhaseClassifier::FlightPhase::Coast,
        200.0f);

    node1->Consume(msg1, std::integral_constant<std::size_t, 0>());
    node2->Consume(msg2, std::integral_constant<std::size_t, 0>());
    node1->Consume(msg1, std::integral_constant<std::size_t, 0>());

    EXPECT_EQ(node1->GetMessagesReceived(), 2);
    EXPECT_EQ(node2->GetMessagesReceived(), 1);
}

// ============================================================================
// FeedbackTestSinkNode Sink Pattern Verification Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, NodeIsSinkNode) {
    // NamedSinkNode pattern: takes input, produces no output
    // Verified by successful instantiation and Consume method
    auto msg = CreateTestFeedbackMsg();
    bool result = node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
}

TEST_F(FeedbackTestSinkNodeTest, AcceptsPhaseAdaptiveFeedbackMsg) {
    // SinkNode pattern: accepts PhaseAdaptiveFeedbackMsg input
    // Port 0 is the feedback input
    auto msg = CreateTestFeedbackMsg();
    bool consumed = node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(consumed);
    EXPECT_EQ(node_->GetMessagesReceived(), 1);
}

// ============================================================================
// FeedbackTestSinkNode Atomic Thread Safety Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, MessageCountIsAtomic) {
    // GetMessagesReceived uses atomic load
    for (int i = 0; i < 100; ++i) {
        auto msg = CreateTestFeedbackMsg();
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    // Multiple reads should see consistent value
    size_t count1 = node_->GetMessagesReceived();
    size_t count2 = node_->GetMessagesReceived();
    EXPECT_EQ(count1, count2);
    EXPECT_EQ(count1, 100);
}

TEST_F(FeedbackTestSinkNodeTest, MessagesVectorSyncWithCount) {
    // Ensure messages_ vector size matches messages_received_ count
    for (int i = 0; i < 10; ++i) {
        auto msg = CreateTestFeedbackMsg();
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    const auto& messages = node_->GetMessages();
    uint64_t count = node_->GetMessagesReceived();
    EXPECT_EQ(messages.size(), static_cast<size_t>(count));
}

// ============================================================================
// FeedbackTestSinkNode Integration Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, FeedbackSinkCompleteMessageFlow) {
    // Simulate a complete feedback flow: multiple phases with parameters
    std::vector<avionics::PhaseAdaptiveFeedbackMsg> test_msgs;

    for (int i = 0; i < 3; ++i) {
        auto msg = CreateTestFeedbackMsg(
            avionics::FlightPhaseClassifier::FlightPhase::Rail,
            100.0f + i * 50);
        msg.parameters.p_gain = 0.5f + i * 0.1f;
        test_msgs.push_back(msg);
    }

    for (const auto& msg : test_msgs) {
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored.size(), 3);

    for (size_t i = 0; i < 3; ++i) {
        EXPECT_FLOAT_EQ(stored[i].altitude_m, 100.0f + i * 50);
        EXPECT_FLOAT_EQ(stored[i].parameters.p_gain, 0.5f + i * 0.1f);
    }
}

TEST_F(FeedbackTestSinkNodeTest, FeedbackSinkTracksPhaseTransitionSequence) {
    // Simulate phase transitions: Rail -> Coast -> Descent
    std::vector<avionics::PhaseAdaptiveFeedbackMsg> msgs;
    std::vector<avionics::FlightPhaseClassifier::FlightPhase> phases = {
        avionics::FlightPhaseClassifier::FlightPhase::Rail,
        avionics::FlightPhaseClassifier::FlightPhase::Coast,
        avionics::FlightPhaseClassifier::FlightPhase::Descent
    };

    for (size_t i = 0; i < phases.size(); ++i) {
        auto msg = CreateTestFeedbackMsg(phases[i]);
        if (i > 0) {
            msg.previous_phase = phases[i - 1];
            msg.is_phase_transition = true;
        }
        msgs.push_back(msg);
    }

    for (const auto& msg : msgs) {
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    const auto& stored = node_->GetMessages();
    EXPECT_EQ(stored[0].current_phase,
              avionics::FlightPhaseClassifier::FlightPhase::Rail);
    EXPECT_EQ(stored[1].current_phase,
              avionics::FlightPhaseClassifier::FlightPhase::Coast);
    EXPECT_EQ(stored[2].current_phase,
              avionics::FlightPhaseClassifier::FlightPhase::Descent);
}

// ============================================================================
// Cross-Cutting FeedbackTestSinkNode Tests
// ============================================================================

TEST_F(FeedbackTestSinkNodeTest, NodeIsTestSink) {
    // FeedbackTestSinkNode is a test sink for feedback messages
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "test_feedback_sink");
}

TEST_F(FeedbackTestSinkNodeTest, NodeImplementsSinkPattern) {
    // NamedSinkNode pattern: consumes PhaseAdaptiveFeedbackMsg
    auto msg = CreateTestFeedbackMsg();
    bool result = node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    EXPECT_TRUE(result);
}

TEST_F(FeedbackTestSinkNodeTest, NodeAccumulatesAllMessages) {
    // Test sink accumulates all incoming messages for inspection
    const int message_count = 50;
    for (int i = 0; i < message_count; ++i) {
        auto msg = CreateTestFeedbackMsg();
        node_->Consume(msg, std::integral_constant<std::size_t, 0>());
    }

    EXPECT_EQ(node_->GetMessagesReceived(), message_count);
    EXPECT_EQ(node_->GetMessages().size(), message_count);
}

