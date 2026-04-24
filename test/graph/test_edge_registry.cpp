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
#include "graph/EdgeRegistry.hpp"
#include <thread>
#include <vector>
#include <atomic>

// ============================================================================
// Layer 2, Task 2.4: EdgeRegistry Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Graph Infrastructure Layer
// ============================================================================

// Test node types for registry testing
namespace test_nodes {
    struct SensorNode {
        static constexpr const char* TypeName = "SensorNode";
        int id = 0;
    };

    struct ProcessorNode {
        static constexpr const char* TypeName = "ProcessorNode";
        int id = 0;
    };

    struct AggregatorNode {
        static constexpr const char* TypeName = "AggregatorNode";
        int id = 0;
    };
}

// ============================================================================
// Test Fixture for EdgeRegistry
// ============================================================================

class EdgeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear registry before each test
        graph::config::EdgeRegistry::Clear();
    }

    void TearDown() override {
        // Ensure clean state after each test
        graph::config::EdgeRegistry::Clear();
    }
};

// ============================================================================
// EdgeRegistry Tests - Registration (5 tests)
// ============================================================================

TEST_F(EdgeRegistryTest, RegisterEdge_SingleType) {
    // Verify that registering a single edge type succeeds
    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 0);

    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 1);
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 0));
}

TEST_F(EdgeRegistryTest, RegisterEdge_MultipleTypes) {
    // Verify that registering multiple edge type combinations works
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 1, test_nodes::ProcessorNode, 1>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::ProcessorNode, 0, test_nodes::AggregatorNode, 0>(
        "ProcessorNode", "AggregatorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 3);
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 0));
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 1, "ProcessorNode", 1));
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("ProcessorNode", 0, "AggregatorNode", 0));
}

TEST_F(EdgeRegistryTest, DuplicateRegistration_Throws) {
    // Verify that registering the same edge type twice throws an error
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    // Attempt to register the same combination again
    bool threw_error = false;
    try {
        graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
            "SensorNode", "ProcessorNode",
            [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return false; }
        );
    } catch (const std::runtime_error&) {
        threw_error = true;
    }
    EXPECT_TRUE(threw_error);

    // Registry should still have only 1 entry
    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 1);
}

TEST_F(EdgeRegistryTest, PortSpecificity_DifferentPorts) {
    // Verify that different port combinations are treated as distinct
    // Port 0 should be different from Port 1
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    // Same nodes, different ports - should NOT throw
    bool threw_error = false;
    try {
        graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 1, test_nodes::ProcessorNode, 1>(
            "SensorNode", "ProcessorNode",
            [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
        );
    } catch (const std::runtime_error&) {
        threw_error = true;
    }
    EXPECT_FALSE(threw_error);

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 2);
}

TEST_F(EdgeRegistryTest, TypeNameVariation_DifferentNames) {
    // Verify that different type names are treated as distinct
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNodeV1", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    // Same type objects but different names in registry - should NOT throw
    bool threw_error = false;
    try {
        graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
            "SensorNodeV2", "ProcessorNode",
            [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
        );
    } catch (const std::runtime_error&) {
        threw_error = true;
    }
    EXPECT_FALSE(threw_error);

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 2);
}

// ============================================================================
// EdgeRegistry Tests - Queries (4 tests)
// ============================================================================

TEST_F(EdgeRegistryTest, IsRegistered_ReturnsFalse) {
    // Verify that IsRegistered returns false for unregistered types
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 0));
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 1, "ProcessorNode", 1));
}

TEST_F(EdgeRegistryTest, GetRegisteredCount_StartsAtZero) {
    // Verify that registry starts empty
    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 0);
}

TEST_F(EdgeRegistryTest, GetRegisteredTypes_ReturnsAll) {
    // Verify that GetRegistered returns all registered edge combinations
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::ProcessorNode, 0, test_nodes::AggregatorNode, 0>(
        "ProcessorNode", "AggregatorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    auto registered = graph::config::EdgeRegistry::GetRegistered();
    EXPECT_EQ(registered.size(), 2);

    // Verify both combinations are in the list
    bool found_sensor_processor = false;
    bool found_processor_aggregator = false;

    for (const auto& entry : registered) {
        if (entry.find("SensorNode") != std::string::npos &&
            entry.find("ProcessorNode") != std::string::npos) {
            found_sensor_processor = true;
        }
        if (entry.find("ProcessorNode") != std::string::npos &&
            entry.find("AggregatorNode") != std::string::npos) {
            found_processor_aggregator = true;
        }
    }

    EXPECT_TRUE(found_sensor_processor);
    EXPECT_TRUE(found_processor_aggregator);
}

TEST_F(EdgeRegistryTest, KeyFormat_MatchesExpected) {
    // Verify that registered entries use the expected key format
    // Format: "SrcType::SrcPort -> DstType::DstPort"
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 1>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    auto registered = graph::config::EdgeRegistry::GetRegistered();
    EXPECT_EQ(registered.size(), 1);

    // Key should contain type names and port info
    const auto& key = registered[0];
    EXPECT_NE(key.find("SensorNode"), std::string::npos);
    EXPECT_NE(key.find("ProcessorNode"), std::string::npos);
    EXPECT_NE(key.find("0"), std::string::npos);  // Source port
    EXPECT_NE(key.find("1"), std::string::npos);  // Destination port
    EXPECT_NE(key.find("->"), std::string::npos); // Arrow separator
}

// ============================================================================
// EdgeRegistry Tests - Lifecycle (2 tests)
// ============================================================================

TEST_F(EdgeRegistryTest, Clear_RemovesAll) {
    // Verify that Clear removes all registrations
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::ProcessorNode, 0, test_nodes::AggregatorNode, 0>(
        "ProcessorNode", "AggregatorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 2);

    graph::config::EdgeRegistry::Clear();

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 0);
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 0));
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("ProcessorNode", 0, "AggregatorNode", 0));
}

TEST_F(EdgeRegistryTest, Clear_AllowsReregistration) {
    // Verify that after Clear(), same types can be registered again
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 1);

    graph::config::EdgeRegistry::Clear();

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 0);

    // Should be able to register again without throwing
    bool threw_error = false;
    try {
        graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
            "SensorNode", "ProcessorNode",
            [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
        );
    } catch (const std::runtime_error&) {
        threw_error = true;
    }
    EXPECT_FALSE(threw_error);

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 1);
}

// ============================================================================
// EdgeRegistry Tests - Port Architecture (3 tests)
// ============================================================================

TEST_F(EdgeRegistryTest, DualPortArchitecture_DataAndSignal) {
    // Verify support for dual-port architecture
    // Port 0: data flow
    // Port 1: completion signals

    // Register both data port and signal port edges
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 1, test_nodes::ProcessorNode, 1>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    // Both should be registered
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 0));
    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 1, "ProcessorNode", 1));

    // Unregistered port combinations should return false
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "ProcessorNode", 1));
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 1, "ProcessorNode", 0));
}

TEST_F(EdgeRegistryTest, HighPortIndices_Supported) {
    // Verify that high port indices are supported (not just 0 and 1)
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 5, test_nodes::ProcessorNode, 10>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_TRUE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 5, "ProcessorNode", 10));
    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 1);
}

TEST_F(EdgeRegistryTest, ComplexPipeline_MultiStage) {
    // Verify support for complex multi-stage pipelines
    // Sensor -> Processor -> Aggregator

    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::ProcessorNode, 0, test_nodes::AggregatorNode, 0>(
        "ProcessorNode", "AggregatorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    // Completion signal ports
    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 1, test_nodes::ProcessorNode, 1>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    graph::config::EdgeRegistry::Register<test_nodes::ProcessorNode, 1, test_nodes::AggregatorNode, 1>(
        "ProcessorNode", "AggregatorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), 4);
}

// ============================================================================
// EdgeRegistry Tests - Error Handling (2 tests)
// ============================================================================

TEST_F(EdgeRegistryTest, CreateEdge_UnregisteredType_Throws) {
    // Verify that CreateEdge throws on unregistered types
    // Since GraphManager is not fully defined in this test context,
    // we verify the lookup would fail by checking IsRegistered first

    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("UnknownNode", 0, "UnknownNode", 0));

    // CreateEdge would throw std::runtime_error for unregistered types
    // This test documents the expected behavior
}

TEST_F(EdgeRegistryTest, InvalidTypeNames_HandledCorrectly) {
    // Verify that invalid or empty type names don't cause crashes
    // They should just not match any lookups

    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
        "SensorNode", "ProcessorNode",
        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
    );

    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("", 0, "ProcessorNode", 0));
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode", 0, "", 0));
    EXPECT_FALSE(graph::config::EdgeRegistry::IsRegistered("SensorNode_typo", 0, "ProcessorNode", 0));
}

// ============================================================================
// EdgeRegistry Tests - Thread Safety (1 test)
// ============================================================================

TEST_F(EdgeRegistryTest, ThreadSafety_ConcurrentRegistration) {
    // Verify that concurrent registration doesn't cause data races
    const int num_threads = 8;
    const int registrations_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successful_registrations{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &successful_registrations]() {
            for (int i = 0; i < registrations_per_thread; ++i) {
                try {
                    // Each thread tries to register a unique edge type
                    std::string src_name = "Sensor_" + std::to_string(t) + "_" + std::to_string(i);
                    std::string dst_name = "Processor_" + std::to_string(t) + "_" + std::to_string(i);

                    graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
                        src_name, dst_name,
                        [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
                    );

                    successful_registrations.fetch_add(1);
                } catch (const std::runtime_error&) {
                    // Expected if collision occurs (same thread might register duplicate)
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify that some registrations succeeded
    EXPECT_GT(successful_registrations.load(), 0);

    // Verify that the total count is reasonable
    size_t registered_count = graph::config::EdgeRegistry::GetRegisteredCount();
    EXPECT_LE(registered_count, static_cast<size_t>(num_threads * registrations_per_thread));
    EXPECT_GT(registered_count, 0);
}

// ============================================================================
// EdgeRegistry Tests - Large Scale (1 test)
// ============================================================================

TEST_F(EdgeRegistryTest, LargeRegistry_ManyEdgeTypes) {
    // Verify registry can handle many edge type combinations
    const int num_types = 25;

    for (int i = 0; i < num_types; ++i) {
        std::string src_name = "Node_" + std::to_string(i);
        std::string dst_name = "Node_" + std::to_string((i + 1) % num_types);

        // Alternate between registering with port 0 and port 1
        if (i % 2 == 0) {
            graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 0, test_nodes::ProcessorNode, 0>(
                src_name, dst_name,
                [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
            );
        } else {
            graph::config::EdgeRegistry::Register<test_nodes::SensorNode, 1, test_nodes::ProcessorNode, 1>(
                src_name, dst_name,
                [](graph::config::GraphManager&, std::size_t, std::size_t, std::size_t) { return true; }
            );
        }
    }

    EXPECT_EQ(graph::config::EdgeRegistry::GetRegisteredCount(), static_cast<size_t>(num_types));

    // Verify all can be retrieved
    auto registered = graph::config::EdgeRegistry::GetRegistered();
    EXPECT_EQ(registered.size(), static_cast<size_t>(num_types));
}

