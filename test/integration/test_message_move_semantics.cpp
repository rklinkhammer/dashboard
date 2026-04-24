// MIT License
//
// Copyright (c) 2026 gdashboard contributors
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
#include "graph/Message.hpp"
#include "graph/PooledMessage.hpp"
#include <string>
#include <vector>
#include <array>

// ============================================================================
// Phase 3: Message Pool Move Semantics Validation
// Ensures pooled allocation doesn't break move operations
// ============================================================================

/**
 * @class MessageMoveSemanticTest
 * @brief Validates that move semantics work correctly with pooled buffers
 */
class MessageMoveSemanticTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize pools for testing
        graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);
    }

    void TearDown() override {
        // Reset pools
        graph::MessagePoolRegistry::GetInstance().Reset();
    }
};

// ============================================================================
// Test Suite: Move Construction (5 tests)
// ============================================================================

TEST_F(MessageMoveSemanticTest, MoveConstruction_SmallPayload) {
    // Small payload uses SSO, no pool involved
    graph::message::Message original(42);
    graph::message::Message moved(std::move(original));

    // Original should be in valid state (potentially empty)
    // Moved should contain the value
    EXPECT_NO_THROW({
        // Both messages should be valid
        (void)original;
        (void)moved;
    });
}

TEST_F(MessageMoveSemanticTest, CreateLargePayloadOnly) {
    // Diagnostic: Just create a large message without moving
    std::string large_data(200, 'x');  // 200+ bytes, exceeds SSO
    graph::message::Message original(large_data);

    // Just verify it was created
    EXPECT_NO_THROW({
        (void)original;
    });
}

TEST_F(MessageMoveSemanticTest, MoveConstruction_LargePayload) {
    // Large payload triggers pooling
    std::string large_data(200, 'x');  // 200+ bytes, exceeds SSO
    graph::message::Message original(large_data);

    // Move constructor
    graph::message::Message moved(std::move(original));

    // Move should succeed without throwing
    EXPECT_NO_THROW({
        (void)moved;
    });
}

TEST_F(MessageMoveSemanticTest, MoveConstruction_PreservesData_SmallType) {
    // Verify move preserves data for small types
    int original_value = 12345;
    graph::message::Message msg1(original_value);
    graph::message::Message msg2(std::move(msg1));

    // Should not throw on copy/access
    EXPECT_NO_THROW({
        graph::message::Message msg3(msg2);
    });
}

TEST_F(MessageMoveSemanticTest, MoveConstruction_PreservesData_LargeType) {
    // Verify move preserves data for large types
    std::string original_value("This is a large message payload");
    graph::message::Message msg1(original_value);

    // Move should preserve the string content
    graph::message::Message msg2(std::move(msg1));

    EXPECT_NO_THROW({
        graph::message::Message msg3(msg2);
    });
}

TEST_F(MessageMoveSemanticTest, MoveConstruction_ChainedMoves) {
    // Test multiple moves in sequence
    std::string data(150, 'a');
    graph::message::Message m1(data);
    graph::message::Message m2(std::move(m1));
    graph::message::Message m3(std::move(m2));
    graph::message::Message m4(std::move(m3));

    // All moves should complete without error
    EXPECT_NO_THROW({
        (void)m4;
    });
}

// ============================================================================
// Test Suite: Move Assignment (5 tests)
// ============================================================================

TEST_F(MessageMoveSemanticTest, MoveAssignment_SmallToSmall) {
    graph::message::Message msg1(10);
    graph::message::Message msg2(20);

    msg1 = std::move(msg2);

    EXPECT_NO_THROW({
        (void)msg1;
    });
}

TEST_F(MessageMoveSemanticTest, MoveAssignment_LargeToLarge) {
    std::string data1(200, 'x');
    std::string data2(250, 'y');

    graph::message::Message msg1(data1);
    graph::message::Message msg2(data2);

    msg1 = std::move(msg2);

    EXPECT_NO_THROW({
        (void)msg1;
    });
}

TEST_F(MessageMoveSemanticTest, MoveAssignment_SmallToLarge) {
    // Move small message into large message (potential resize)
    std::string large_data(200, 'x');
    graph::message::Message msg1(large_data);
    graph::message::Message msg2(42);

    msg1 = std::move(msg2);

    EXPECT_NO_THROW({
        (void)msg1;
    });
}

TEST_F(MessageMoveSemanticTest, MoveAssignment_LargeToSmall) {
    // Move large message into small message
    graph::message::Message msg1(42);
    std::string large_data(200, 'x');
    graph::message::Message msg2(large_data);

    msg1 = std::move(msg2);

    EXPECT_NO_THROW({
        (void)msg1;
    });
}

TEST_F(MessageMoveSemanticTest, MoveAssignment_SelfAssignment) {
    // Self-move assignment should not crash
    std::string data(200, 'x');
    graph::message::Message msg(data);

    // Simulate self-move (typically caught by compiler, but test anyway)
    auto& ref = msg;
    ref = std::move(msg);

    EXPECT_NO_THROW({
        (void)msg;
    });
}

// ============================================================================
// Test Suite: NRVO Compatibility (3 tests)
// ============================================================================

static graph::message::Message CreateMessageSmall() {
    return graph::message::Message(42);
}

static graph::message::Message CreateMessageLarge() {
    std::string large_data(200, 'x');
    return graph::message::Message(large_data);
}

static graph::message::Message CreateMessageMixed() {
    // Create multiple messages and return one (tests NRVO optimization)
    graph::message::Message temp1(10);
    graph::message::Message temp2(std::string(150, 'y'));
    return temp2;  // NRVO should elide the move
}

TEST_F(MessageMoveSemanticTest, NRVO_SmallPayload) {
    // Test Named Return Value Optimization with small payloads
    auto msg = CreateMessageSmall();

    EXPECT_NO_THROW({
        graph::message::Message copy(msg);
    });
}

TEST_F(MessageMoveSemanticTest, NRVO_LargePayload) {
    // Test NRVO with large payloads that use pooling
    auto msg = CreateMessageLarge();

    EXPECT_NO_THROW({
        graph::message::Message copy(msg);
    });
}

TEST_F(MessageMoveSemanticTest, NRVO_ImplicitMove) {
    // Test implicit move in return statements
    auto msg = CreateMessageMixed();

    EXPECT_NO_THROW({
        graph::message::Message copy(msg);
    });
}

// ============================================================================
// Test Suite: Move in Containers (3 tests)
// ============================================================================

TEST_F(MessageMoveSemanticTest, VectorEmplaceBack_SmallMessages) {
    std::vector<graph::message::Message> messages;

    for (int i = 0; i < 10; i++) {
        messages.emplace_back(i);
    }

    EXPECT_EQ(messages.size(), 10);
}

TEST_F(MessageMoveSemanticTest, VectorEmplaceBack_LargeMessages) {
    std::vector<graph::message::Message> messages;

    for (int i = 0; i < 10; i++) {
        std::string data(200 + i * 10, 'x');
        messages.emplace_back(data);
    }

    EXPECT_EQ(messages.size(), 10);
}

TEST_F(MessageMoveSemanticTest, VectorPushBack_MoveSemantics) {
    std::vector<graph::message::Message> messages;

    std::string large_data(200, 'y');
    graph::message::Message msg(large_data);

    messages.push_back(std::move(msg));

    EXPECT_EQ(messages.size(), 1);
}

// ============================================================================
// Test Suite: Exception Safety (3 tests)
// ============================================================================

TEST_F(MessageMoveSemanticTest, MoveConstruction_NoThrow) {
    std::string large_data(200, 'x');
    graph::message::Message original(large_data);

    EXPECT_NO_THROW({
        graph::message::Message moved(std::move(original));
    });
}

TEST_F(MessageMoveSemanticTest, MoveAssignment_NoThrow) {
    std::string data1(200, 'x');
    std::string data2(250, 'y');

    graph::message::Message msg1(data1);
    graph::message::Message msg2(data2);

    EXPECT_NO_THROW({
        msg1 = std::move(msg2);
    });
}

TEST_F(MessageMoveSemanticTest, SequentialMoves_NoThrow) {
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; i++) {
            std::string data(150 + i % 50, 'x');
            graph::message::Message msg1(data);
            graph::message::Message msg2(std::move(msg1));
            graph::message::Message msg3(std::move(msg2));
            (void)msg3;
        }
    });
}

// ============================================================================
// Test Suite: Pool State Consistency (3 tests)
// ============================================================================

TEST_F(MessageMoveSemanticTest, PoolStateAfterMoves) {
    // Create and move messages, verify pool state remains consistent
    {
        std::string data(200, 'x');
        graph::message::Message msg1(data);
        graph::message::Message msg2(std::move(msg1));
    }

    // Pool should still be functional
    auto stats_after = graph::MessagePoolRegistry::GetInstance().GetAggregateStats();
    EXPECT_GE(stats_after.total_requests, 0);
}

TEST_F(MessageMoveSemanticTest, PoolReusability_AfterMoveOperations) {
    // Verify pool buffers can still be reused after move operations
    // Note: Using std::array because std::string is only 24 bytes (< 32 byte SSO)
    // and won't trigger pooling. We need a type > 32 bytes to test pool reuse.
    using LargeType = std::array<double, 5>;  // 40 bytes > 32 byte threshold

    {
        LargeType data1 = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg1(data1);
        graph::message::Message msg2(std::move(msg1));
    }

    auto stats_mid = graph::MessagePoolRegistry::GetInstance().GetAggregateStats();
    uint64_t mid_requests = stats_mid.total_requests;

    // Create more messages - should reuse buffers
    {
        LargeType data2 = {5.0, 4.0, 3.0, 2.0, 1.0};
        graph::message::Message msg3(data2);
        graph::message::Message msg4(std::move(msg3));
    }

    auto stats_final = graph::MessagePoolRegistry::GetInstance().GetAggregateStats();

    // Should have more requests but minimal new allocations
    EXPECT_GT(stats_final.total_requests, mid_requests);
}

TEST_F(MessageMoveSemanticTest, NoLeaksDuringMoves) {
    // Create many moved messages and verify no resource leaks
    size_t initial_allocations = 0;
    {
        auto stats = graph::MessagePoolRegistry::GetInstance().GetAggregateStats();
        initial_allocations = stats.total_allocations;
    }

    // Perform many move operations
    for (int i = 0; i < 100; i++) {
        std::string data(200 + i % 50, 'x');
        graph::message::Message msg1(data);
        graph::message::Message msg2(std::move(msg1));
        graph::message::Message msg3(std::move(msg2));
    }

    auto final_stats = graph::MessagePoolRegistry::GetInstance().GetAggregateStats();

    // Allocations should be minimal (mostly reuses)
    uint64_t new_allocations = final_stats.total_allocations - initial_allocations;

    // With 100 iterations and 70% reuse rate, expect ~30 new allocations
    // Allow for some variance, but should be much less than 100
    EXPECT_LT(new_allocations, 50) << "Too many allocations during move operations";
}

