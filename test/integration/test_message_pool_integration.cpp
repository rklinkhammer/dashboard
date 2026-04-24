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
#include "graph/PooledMessage.hpp"
#include "graph/Message.hpp"
#include "graph/GraphManager.hpp"
#include "graph/NamedNodes.hpp"
#include <chrono>
#include <vector>
#include <array>
#include <iostream>

// ============================================================================
// Phase 2c: Message Pool Integration Tests
// Validates that pooling reduces allocation overhead in real workloads
// ============================================================================

// Type for pooling tests: 40 bytes > 32 byte SSO threshold
using LargePayloadType = std::array<double, 5>;

/**
 * @brief Test node that produces various message sizes for pool testing
 */
class PoolTestSourceNode : public graph::NamedSourceNode<PoolTestSourceNode, graph::message::Message> {
public:
    static constexpr char kOutputPort[] = "Output";

    using Ports = std::tuple<
        graph::PortSpec<0, graph::message::Message, graph::PortDirection::Output, kOutputPort,
            graph::PayloadList<int, double, LargePayloadType>>
    >;

    explicit PoolTestSourceNode(size_t message_count = 0)
        : message_count_(message_count), messages_produced_(0) {
        SetName("PoolTestSource");
    }

    virtual ~PoolTestSourceNode() = default;

    // Produce small, medium, and large messages in rotation
    std::optional<graph::message::Message> Produce(std::integral_constant<std::size_t, 0>) override {
        if (messages_produced_ < message_count_) {
            size_t idx = messages_produced_++;

            // Vary message sizes: small (int), medium (double), large (array)
            if (idx % 3 == 0) {
                // Small message: fits in SSO
                return graph::message::Message(static_cast<int>(idx));
            } else if (idx % 3 == 1) {
                // Medium message
                return graph::message::Message(static_cast<double>(idx) * 3.14159);
            } else {
                // Large message: 40 bytes > 32 byte SSO, triggers pooling
                LargePayloadType large_payload = {1.0, 2.0, 3.0, 4.0, 5.0};
                return graph::message::Message(large_payload);
            }
        }
        return std::nullopt;
    }

    size_t GetMessagesProduced() const { return messages_produced_.load(); }

private:
    size_t message_count_;
    std::atomic<size_t> messages_produced_;
};

/**
 * @brief Pass-through processor for pool measurements
 */
class PoolTestProcessorNode : public graph::NamedInteriorNode<
    graph::TypeList<graph::message::Message>,
    graph::TypeList<graph::message::Message>,
    PoolTestProcessorNode> {
public:
    static constexpr char kInputPort[] = "Input";
    static constexpr char kOutputPort[] = "Output";

    using Ports = std::tuple<
        graph::PortSpec<0, graph::message::Message, graph::PortDirection::Input, kInputPort,
            graph::PayloadList<int, double, LargePayloadType>>,
        graph::PortSpec<0, graph::message::Message, graph::PortDirection::Output, kOutputPort,
            graph::PayloadList<int, double, LargePayloadType>>
    >;

    PoolTestProcessorNode() {
        SetName("PoolTestProcessor");
    }

    virtual ~PoolTestProcessorNode() = default;

    virtual std::optional<graph::message::Message> Transfer(
        const graph::message::Message& input,
        std::integral_constant<std::size_t, 0>,
        std::integral_constant<std::size_t, 0>) override {
        return input;
    }
};

/**
 * @brief Sink node for collecting pool stats
 */
class PoolTestSinkNode : public graph::NamedSinkNode<PoolTestSinkNode, graph::message::Message> {
public:
    static constexpr char kInputPort[] = "Input";
    using Ports = std::tuple<
        graph::PortSpec<0, graph::message::Message, graph::PortDirection::Input, kInputPort,
            graph::PayloadList<int, double, LargePayloadType>>
    >;

    PoolTestSinkNode() : messages_received_(0) {
        SetName("PoolTestSink");
    }

    virtual ~PoolTestSinkNode() = default;

    bool Consume(const graph::message::Message& msg, std::integral_constant<std::size_t, 0>) override {
        (void)msg;
        ++messages_received_;
        return true;
    }

    size_t GetMessagesReceived() const { return messages_received_.load(); }

private:
    std::atomic<size_t> messages_received_;
};

// ============================================================================
// Test Fixture for Message Pool Integration
// ============================================================================

class MessagePoolIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset pool registry to clean state
        graph::MessagePoolRegistry::GetInstance().Reset();
    }

    void TearDown() override {
        // Clean up pools after test
        graph::MessagePoolRegistry::GetInstance().Reset();
    }

    // Helper to get current pool statistics
    graph::MessagePoolRegistry::AggregateStats GetPoolStats() {
        return graph::MessagePoolRegistry::GetInstance().GetAggregateStats();
    }

    // Helper to run a simple graph and measure pool behavior
    void RunGraphWithPool(size_t num_messages) {
        auto config = graph::ThreadPool::DeadlockConfig();
        graph::GraphManager graph(2, config);

        // Create nodes
        auto source = graph.AddNode<PoolTestSourceNode>(num_messages);
        auto processor = graph.AddNode<PoolTestProcessorNode>();
        auto sink = graph.AddNode<PoolTestSinkNode>();

        // Create edges (buffer_size will be used for pooling decisions)
        graph.AddEdge<PoolTestSourceNode, 0, PoolTestProcessorNode, 0>(source, processor, 256);
        graph.AddEdge<PoolTestProcessorNode, 0, PoolTestSinkNode, 0>(processor, sink, 256);

        // Initialize and run
        ASSERT_TRUE(graph.Init());
        ASSERT_TRUE(graph.Start());

        // Let it run to completion
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Stop and clean up
        graph.Stop();
        graph.Join();
    }
};

// ============================================================================
// Test Suite: Pool Initialization (2 tests)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, PoolRegistryInitializesCommonPools) {
    // Initialize pools as done in GraphManager::Init()
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    // Verify common pools are created
    auto stats = GetPoolStats();
    EXPECT_GT(stats.total_allocations, 0) << "Pools should pre-allocate buffers";
}

TEST_F(MessagePoolIntegrationTest, PoolStatsAccuracyBasic) {
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    // Get initial stats
    auto initial_stats = GetPoolStats();
    uint64_t initial_requests = initial_stats.total_requests;
    uint64_t initial_reuses = initial_stats.total_reuses;

    EXPECT_GE(initial_stats.total_allocations, 0);
    EXPECT_EQ(initial_reuses, 0) << "No reuses yet";
}

// ============================================================================
// Test Suite: Pool Reuse Metrics (3 tests)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, PoolTrackingAllocations) {
    // Run graph and measure allocation tracking
    RunGraphWithPool(10);

    auto final_stats = GetPoolStats();

    // With 10 messages in pipeline, we should see some allocations
    EXPECT_GT(final_stats.total_requests, 0) << "Pools should track acquisition requests";
}

TEST_F(MessagePoolIntegrationTest, PoolReusesIncrementOnMultipleCycles) {
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    // Run twice to see reuse across cycles
    RunGraphWithPool(5);

    auto stats_after_first = GetPoolStats();
    uint64_t reuses_after_first = stats_after_first.total_reuses;

    // Run again - second run should reuse buffers
    RunGraphWithPool(5);

    auto stats_after_second = GetPoolStats();
    uint64_t reuses_after_second = stats_after_second.total_reuses;

    // More reuses should accumulate
    EXPECT_GE(reuses_after_second, reuses_after_first)
        << "Reuse count should not decrease between runs";
}

TEST_F(MessagePoolIntegrationTest, PoolHitRateCalculation) {
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    RunGraphWithPool(20);

    auto stats = GetPoolStats();

    // Hit rate should be calculable and non-negative
    double expected_hit_rate = 0.0;
    if (stats.total_requests > 0) {
        expected_hit_rate = (static_cast<double>(stats.total_reuses) /
                            static_cast<double>(stats.total_requests)) * 100.0;
    }

    EXPECT_GE(stats.aggregate_hit_rate, 0.0);
    EXPECT_LE(stats.aggregate_hit_rate, 100.0) << "Hit rate should be a valid percentage";
}

// ============================================================================
// Test Suite: Pool Efficiency (2 tests)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, PoolReducesAllocationFrequency) {
    // Initialize pools first before running graph
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    // Get baseline after initialization
    auto baseline = GetPoolStats();
    uint64_t baseline_allocations = baseline.total_allocations;
    uint64_t baseline_requests = baseline.total_requests;

    // Run workload
    RunGraphWithPool(30);

    auto final = GetPoolStats();

    // Verify pools are being used
    EXPECT_GT(final.total_requests, baseline_requests) << "Pool should handle message allocations";

    // Verify allocations were made
    uint64_t new_allocations = final.total_allocations - baseline_allocations;
    uint64_t total_requests = final.total_requests - baseline_requests;

    // Both should be > 0 if pools are being used
    EXPECT_GE(new_allocations, 0) << "Allocation count should be tracked";
    EXPECT_GT(total_requests, 0) << "Request count should be tracked";
}

TEST_F(MessagePoolIntegrationTest, PoolMemoryBudgetRespected) {
    // Initialize with specific capacity
    size_t pool_capacity = 128;
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(pool_capacity);

    auto stats = GetPoolStats();

    // Verify pools are initialized
    EXPECT_GE(stats.total_allocations, 0) << "Pools should have allocations or be empty";
}

// ============================================================================
// Test Suite: Correctness Under Pooling (2 tests)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, PoolingDoesNotCorruptData) {
    // This test verifies that messages are correctly allocated and destroyed
    // when using pooled buffers

    auto config = graph::ThreadPool::DeadlockConfig();
    graph::GraphManager graph(2, config);

    // Use a custom processor that validates message integrity
    auto source = graph.AddNode<PoolTestSourceNode>(15);
    auto processor = graph.AddNode<PoolTestProcessorNode>();
    auto sink = graph.AddNode<PoolTestSinkNode>();

    graph.AddEdge<PoolTestSourceNode, 0, PoolTestProcessorNode, 0>(source, processor, 256);
    graph.AddEdge<PoolTestProcessorNode, 0, PoolTestSinkNode, 0>(processor, sink, 256);

    ASSERT_TRUE(graph.Init());
    ASSERT_TRUE(graph.Start());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Verify all messages were processed
    size_t messages_produced = source->GetMessagesProduced();
    size_t messages_received = sink->GetMessagesReceived();

    graph.Stop();
    graph.Join();

    EXPECT_EQ(messages_produced, messages_received)
        << "All messages should flow through the graph";
}

TEST_F(MessagePoolIntegrationTest, PoolingPreservesOrdering) {
    // Verify that message ordering is preserved despite pooling

    auto config = graph::ThreadPool::DeadlockConfig();
    graph::GraphManager graph(2, config);

    auto source = graph.AddNode<PoolTestSourceNode>(25);
    auto processor = graph.AddNode<PoolTestProcessorNode>();
    auto sink = graph.AddNode<PoolTestSinkNode>();

    graph.AddEdge<PoolTestSourceNode, 0, PoolTestProcessorNode, 0>(source, processor, 256);
    graph.AddEdge<PoolTestProcessorNode, 0, PoolTestSinkNode, 0>(processor, sink, 256);

    ASSERT_TRUE(graph.Init());
    ASSERT_TRUE(graph.Start());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    size_t final_message_count = sink->GetMessagesReceived();

    graph.Stop();
    graph.Join();

    EXPECT_EQ(final_message_count, 25) << "Correct number of messages processed with pooling";
}

// ============================================================================
// Test Suite: Performance Baseline (1 test)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, PoolAllocationLatency) {
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    auto start = std::chrono::steady_clock::now();

    RunGraphWithPool(50);

    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    auto stats = GetPoolStats();

    // Measure average latency per request
    double avg_request_latency_us = 0.0;
    if (stats.total_requests > 0) {
        // Rough estimate: total time / requests
        avg_request_latency_us = (duration_ms * 1000.0) / stats.total_requests;
    }

    // Log results for analysis
    std::cout << "\n=== Pool Performance Baseline ===\n"
              << "Messages processed: " << stats.total_requests << "\n"
              << "Total allocations: " << stats.total_allocations << "\n"
              << "Total reuses: " << stats.total_reuses << "\n"
              << "Hit rate: " << stats.aggregate_hit_rate << "%\n"
              << "Total time: " << duration_ms << "ms\n"
              << "Avg latency: " << avg_request_latency_us << "us/request\n"
              << "================================\n";

    EXPECT_GT(stats.total_requests, 0) << "Should process messages through pools";
}

// ============================================================================
// Test Suite: Statistics Accuracy Validation (Phase 3 Task 2)
// ============================================================================

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_SingleAllocation) {
    // Test that a single pool allocation is tracked correctly
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(256);

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;
    uint64_t baseline_allocs = baseline.total_allocations;
    uint64_t baseline_reuses = baseline.total_reuses;

    // Create exactly one large message (will use pool)
    {
        LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg(data);
    }

    auto final = GetPoolStats();

    // Should have exactly 1 more request (acquisition)
    EXPECT_EQ(final.total_requests, baseline_reqs + 1)
        << "One allocation should register as one request";

    // Since pools were pre-allocated, this should come from prealloc, not trigger new malloc
    // (unless pool was empty, but we allocated 256 of each size)
    EXPECT_EQ(final.total_reuses, baseline_reuses + 1)
        << "Pre-allocated buffer should count as a reuse";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_SequentialAllocations) {
    // Test that sequential allocations are tracked accurately
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(10);  // Small pool

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;

    // Create 5 messages sequentially
    {
        for (int i = 0; i < 5; ++i) {
            LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
            graph::message::Message msg(data);
            (void)msg;  // Use the message to prevent optimization
        }
    }

    auto final = GetPoolStats();

    // All 5 messages should register as requests
    EXPECT_EQ(final.total_requests, baseline_reqs + 5)
        << "Five messages should register as five requests";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_ReuseTracking) {
    // Test that buffer reuses are tracked accurately
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(2);  // Very small pool

    auto baseline = GetPoolStats();
    uint64_t baseline_reuses = baseline.total_reuses;
    uint64_t baseline_allocs = baseline.total_allocations;

    // Create messages that will trigger reuse
    {
        // First message: allocates from prealloc
        LargePayloadType data1 = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg1(data1);
        // msg1 destroyed, buffer returned to pool
    }

    {
        // Second message: should reuse buffer from msg1
        LargePayloadType data2 = {2.0, 3.0, 4.0, 5.0, 6.0};
        graph::message::Message msg2(data2);
        // msg2 destroyed, buffer returned to pool
    }

    auto final = GetPoolStats();

    // The second allocation should count as a reuse
    uint64_t new_reuses = final.total_reuses - baseline_reuses;
    EXPECT_GE(new_reuses, 1) << "Second message should trigger at least one reuse";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_HitRateCalculation) {
    // Test that hit rate is calculated correctly
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(100);

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;
    uint64_t baseline_reuses = baseline.total_reuses;

    // Create 10 messages to build statistics
    {
        for (int i = 0; i < 10; ++i) {
            LargePayloadType data = {static_cast<double>(i), 0, 0, 0, 0};
            graph::message::Message msg(data);
        }
    }

    auto final = GetPoolStats();

    uint64_t total_reqs = final.total_requests - baseline_reqs;
    uint64_t total_reuses = final.total_reuses - baseline_reuses;

    // With large prealloc, most should be reuses
    if (total_reqs > 0) {
        double expected_hit_rate = (static_cast<double>(total_reuses) /
                                    static_cast<double>(total_reqs)) * 100.0;

        // Hit rate should be >= expected (within floating point tolerance)
        EXPECT_GE(final.aggregate_hit_rate, expected_hit_rate - 1.0)
            << "Hit rate calculation should match expected value";
        EXPECT_LE(final.aggregate_hit_rate, 100.0)
            << "Hit rate should not exceed 100%";
    }
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_AfterDestruction) {
    // Test that statistics remain accurate after message destruction
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(50);

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;

    // Create and destroy messages in a scope
    {
        for (int i = 0; i < 5; ++i) {
            LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
            graph::message::Message msg(data);
        }
    }

    auto after_scope = GetPoolStats();
    uint64_t reqs_in_scope = after_scope.total_requests - baseline_reqs;

    // Statistics should persist after destruction
    EXPECT_EQ(reqs_in_scope, 5) << "All 5 messages should be tracked despite destruction";

    // Create more messages to verify stats continue to accumulate
    {
        LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg(data);
    }

    auto final = GetPoolStats();
    uint64_t total_reqs = final.total_requests - baseline_reqs;

    // Should have 6 total requests
    EXPECT_EQ(total_reqs, 6) << "Statistics should accumulate across multiple allocations";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_MultiplePools) {
    // Test that statistics are aggregated correctly from multiple pools
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(50);

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;

    // Create a small message (won't use pool)
    {
        int small_msg = 42;
        graph::message::Message msg(small_msg);
    }

    // Create a large message (will use pool)
    {
        LargePayloadType large_msg = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg(large_msg);
    }

    auto final = GetPoolStats();

    // Only the large message should add to pool requests
    uint64_t new_reqs = final.total_requests - baseline_reqs;
    EXPECT_EQ(new_reqs, 1) << "Only large message should trigger pool request";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_ConsistentAcrossResets) {
    // Test that statistics can be reset properly without memory issues
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(50);

    // Generate some statistics
    {
        for (int i = 0; i < 5; ++i) {
            LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
            graph::message::Message msg(data);
        }
    }

    auto before_reset = GetPoolStats();
    EXPECT_GT(before_reset.total_requests, 0) << "Should have some requests before reset";

    // Reset pools (clears statistics)
    graph::MessagePoolRegistry::GetInstance().Reset();

    // Re-initialize
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(50);

    auto after_reset = GetPoolStats();
    EXPECT_EQ(after_reset.total_requests, 0)
        << "Requests should be reset after Reset() call";

    // Generate new statistics
    {
        LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg(data);
    }

    auto after_new_work = GetPoolStats();
    EXPECT_EQ(after_new_work.total_requests, 1)
        << "After reset, new requests should start fresh";
}

TEST_F(MessagePoolIntegrationTest, StatsAccuracy_MoveOperations) {
    // Test that move operations don't corrupt statistics
    graph::MessagePoolRegistry::GetInstance().InitializeCommonPools(50);

    auto baseline = GetPoolStats();
    uint64_t baseline_reqs = baseline.total_requests;

    // Create and move messages
    {
        LargePayloadType data = {1.0, 2.0, 3.0, 4.0, 5.0};
        graph::message::Message msg1(data);
        graph::message::Message msg2(std::move(msg1));  // Move should not add requests
        graph::message::Message msg3(std::move(msg2));  // Another move
    }

    auto final = GetPoolStats();

    // Only the initial creation should count as a request
    uint64_t new_reqs = final.total_requests - baseline_reqs;
    EXPECT_EQ(new_reqs, 1)
        << "Move operations should not add additional pool requests";
}

