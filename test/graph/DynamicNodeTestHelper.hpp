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

#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "graph/NodeFactory.hpp"
#include "graph/NodeFacade.hpp"
#include "plugins/PluginRegistry.hpp"
#include "plugins/PluginLoader.hpp"

namespace graph::test {

/**
 * @class DynamicNodeTestHelper
 * @brief Utilities for dynamic node loading tests
 *
 * Provides factory utilities and validation helpers for Layer 5 dynamic
 * node loading tests. These tests verify that nodes work identically when
 * loaded dynamically via NodeFactory::CreateNode(string) instead of via
 * direct C++ instantiation.
 *
 * Design Pattern:
 * - Each Layer 5 node has TWO test files:
 *   1. test_flight_fsm_node.cpp (static) - Direct C++ instantiation
 *   2. test_flight_fsm_node_dynamic.cpp (dynamic) - Factory instantiation
 * - Both test suites run identical test cases
 * - This validates NodeFactory plugin system integration
 * - Ensures nodes behave identically via both paths
 *
 * Usage:
 * @code
 * class FlightFSMNodeDynamicTest : public ::testing::Test {
 * protected:
 *     void SetUp() override {
 *         auto factory = DynamicNodeTestHelper::CreateInitializedFactory();
 *         node_adapter_ = factory->CreateNode("FlightFSMNode");
 *         ASSERT_TRUE(node_adapter_.IsValid());
 *     }
 *
 *     graph::NodeFacadeAdapter node_adapter_;
 * };
 * @endcode
 *
 * @since Phase 1 - Layer 5 Dynamic Loading Tests
 */
class DynamicNodeTestHelper {
public:
    /**
     * Create a fully initialized NodeFactory with all Layer 5 nodes registered
     *
     * This factory is ready to create any Layer 5 node dynamically.
     * The factory is pre-initialized and doesn't require additional setup.
     *
     * @return Shared pointer to initialized NodeFactory
     *
     * @note The returned factory is thread-safe once created
     * @note All 12 Layer 5 node types should be available (if plugin system is loaded)
     * @note Currently relies on plugin system; static node registration is deferred
     *
     * Usage:
     * @code
     * auto factory = DynamicNodeTestHelper::CreateInitializedFactory();
     * auto node1 = factory->CreateNode("FlightFSMNode");
     * auto node2 = factory->CreateNode("StateVectorCaptureSinkNode");
     * @endcode
     */
    static std::shared_ptr<graph::NodeFactory> CreateInitializedFactory() {
        // Create plugin registry
        auto registry = std::make_shared<graph::PluginRegistry>();

        // Set up plugin directory and load plugins
#ifdef GDASHBOARD_PLUGIN_DIR
        try {
            graph::PluginLoader loader(GDASHBOARD_PLUGIN_DIR, registry);
            loader.LoadAllPlugins();
            GTEST_LOG_(INFO) << "Loaded " << loader.GetLoadedPluginCount() << " plugins from "
                             << GDASHBOARD_PLUGIN_DIR;
        } catch (const std::exception& e) {
            GTEST_LOG_(WARNING) << "Failed to load plugins: " << e.what()
                                << " (this is expected if plugins haven't been built)";
        }
#else
        GTEST_LOG_(WARNING) << "GDASHBOARD_PLUGIN_DIR not defined at compile time";
#endif

        // Create and initialize the factory with the populated registry
        auto factory = std::make_shared<graph::NodeFactory>(registry);

        try {
            factory->Initialize();
        } catch (const std::exception& e) {
            GTEST_LOG_(WARNING) << "Factory initialization failed: " << e.what();
        }

        return factory;
    }

    /**
     * Create a node dynamically by type name
     *
     * Helper convenience method that creates a factory and instantiates
     * a node in one call.
     *
     * @param node_type_name Name of the node type (e.g., "FlightFSMNode")
     * @return NodeFacadeAdapter wrapping the created node
     *
     * @throws std::runtime_error if node type is not registered
     *
     * Usage:
     * @code
     * auto node = DynamicNodeTestHelper::CreateNodeByType("FlightFSMNode");
     * EXPECT_TRUE(node.IsValid());
     * @endcode
     */
    static graph::NodeFacadeAdapter CreateNodeByType(const std::string& node_type_name) {
        auto factory = CreateInitializedFactory();
        return factory->CreateNode(node_type_name);
    }

    /**
     * Verify that all Layer 5 node types are registered
     *
     * Helper for tests that need to check registration completeness.
     * Returns a vector of all node types that failed to register.
     *
     * @return Empty vector if all nodes registered, otherwise list of missing types
     *
     * Usage:
     * @code
     * auto missing = DynamicNodeTestHelper::GetMissingNodeTypes();
     * EXPECT_TRUE(missing.empty()) << "Missing nodes: " << ...;
     * @endcode
     */
    static std::vector<std::string> GetMissingNodeTypes() {
        auto factory = CreateInitializedFactory();
        std::vector<std::string> all_types = {
            // Layer 5 Data Injection Nodes (5)
            "DataInjectionAccelerometerNode",
            "DataInjectionGyroscopeNode",
            "DataInjectionMagnetometerNode",
            "DataInjectionBarometricNode",
            "DataInjectionGPSPositionNode",

            // Layer 5 Core Processing Nodes (5)
            "FlightFSMNode",
            "AltitudeFusionNode",
            "EstimationPipelineNode",
            "FlightMonitorNode",

            // Layer 5 Sink Nodes (2)
            "FeedbackTestSinkNode",
            "StateVectorCaptureSinkNode",

            // Layer 6 Aggregation Node (1)
            "CompletionAggregatorNode",
        };

        std::vector<std::string> missing;
        for (const auto& type : all_types) {
            if (!factory->IsNodeTypeAvailable(type)) {
                missing.push_back(type);
            }
        }
        return missing;
    }

    /**
     * Get list of all Layer 5 node type names
     *
     * @return Vector of 12 expected Layer 5 node type names
     */
    static std::vector<std::string> GetAllExpectedNodeTypes() {
        return {
            // Layer 5 Data Injection Nodes (5)
            "DataInjectionAccelerometerNode",
            "DataInjectionGyroscopeNode",
            "DataInjectionMagnetometerNode",
            "DataInjectionBarometricNode",
            "DataInjectionGPSPositionNode",

            // Layer 5 Core Processing Nodes (5)
            "FlightFSMNode",
            "AltitudeFusionNode",
            "EstimationPipelineNode",
            "FlightMonitorNode",

            // Layer 5 Sink Nodes (2)
            "FeedbackTestSinkNode",
            "StateVectorCaptureSinkNode",

            // Layer 6 Aggregation Node (1)
            "CompletionAggregatorNode",
        };
    }

    /**
     * Helper for printing node type information in test diagnostics
     *
     * @param node_type_name Name of the node type
     * @return String describing the node type category
     */
    static std::string GetNodeCategory(const std::string& node_type_name) {
        if (node_type_name.find("DataInjection") != std::string::npos) {
            return "DataInjectionNode";
        } else if (node_type_name == "FlightFSMNode") {
            return "CoreProcessing (5-input fusion)";
        } else if (node_type_name == "AltitudeFusionNode" ||
                   node_type_name == "EstimationPipelineNode" ||
                   node_type_name == "FlightMonitorNode") {
            return "CoreProcessing (Interior transformation)";
        } else if (node_type_name == "FeedbackTestSinkNode" ||
                   node_type_name == "StateVectorCaptureSinkNode") {
            return "SinkNode (Terminal)";
        } else if (node_type_name == "CompletionAggregatorNode") {
            return "AggregationNode";
        }
        return "Unknown";
    }
};

}  // namespace graph::test
