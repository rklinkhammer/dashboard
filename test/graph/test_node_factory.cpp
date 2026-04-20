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
#include "graph/NodeFactory.hpp"
#include "graph/NodeFactoryRegistry.hpp"
#include <memory>
#include <algorithm>

// ============================================================================
// Layer 2, Task 2.3: NodeFactory & NodeFactoryRegistry Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Graph Infrastructure Layer
// ============================================================================

// ============================================================================
// Test Fixture for NodeFactoryRegistry
// ============================================================================

class NodeFactoryRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry = std::make_shared<graph::config::NodeFactoryRegistry>();
        call_counter = 0;
    }

    std::shared_ptr<graph::config::NodeFactoryRegistry> registry;
    static int call_counter;

public:
    /**
     * Helper: Create a factory that throws
     * Since NodeFacadeAdapter is non-copyable, we can only test by throwing
     */
    static graph::config::NodeFactoryRegistry::NodeFactoryFunction CreateThrowingFactory(
        const std::string& error_msg) {
        // Use a stateless lambda (no capture) that always throws
        // This avoids std::function conversion issues
        return []() -> graph::NodeFacadeAdapter {
            throw std::runtime_error("Factory not implemented for testing");
        };
    }
};

int NodeFactoryRegistryTest::call_counter = 0;

// ============================================================================
// NodeFactoryRegistry Tests
// ============================================================================

TEST_F(NodeFactoryRegistryTest, ConstructsSuccessfully) {
    EXPECT_EQ(registry->GetRegistrySize(), 0);
    EXPECT_TRUE(registry->GetAvailableTypes().empty());
}

TEST_F(NodeFactoryRegistryTest, RegisterSingleNodeType) {
    registry->Register("TestNode", CreateThrowingFactory("test"));

    EXPECT_EQ(registry->GetRegistrySize(), 1);
    EXPECT_TRUE(registry->IsAvailable("TestNode"));
}

TEST_F(NodeFactoryRegistryTest, RegisterMultipleNodeTypes) {
    registry->Register("Node1", CreateThrowingFactory("1"));
    registry->Register("Node2", CreateThrowingFactory("2"));
    registry->Register("Node3", CreateThrowingFactory("3"));

    EXPECT_EQ(registry->GetRegistrySize(), 3);
    EXPECT_TRUE(registry->IsAvailable("Node1"));
    EXPECT_TRUE(registry->IsAvailable("Node2"));
    EXPECT_TRUE(registry->IsAvailable("Node3"));
}

TEST_F(NodeFactoryRegistryTest, ReplaceExistingFactory) {
    registry->Register("Node", CreateThrowingFactory("first"));
    EXPECT_EQ(registry->GetRegistrySize(), 1);

    registry->Register("Node", CreateThrowingFactory("second"));
    EXPECT_EQ(registry->GetRegistrySize(), 1);  // Still 1, not 2
    EXPECT_TRUE(registry->IsAvailable("Node"));
}

TEST_F(NodeFactoryRegistryTest, GetAvailableTypes) {
    registry->Register("NodeA", CreateThrowingFactory("a"));
    registry->Register("NodeB", CreateThrowingFactory("b"));
    registry->Register("NodeC", CreateThrowingFactory("c"));

    auto types = registry->GetAvailableTypes();
    EXPECT_EQ(types.size(), 3);

    // Check that all registered types are in the list
    std::sort(types.begin(), types.end());
    EXPECT_EQ(types[0], "NodeA");
    EXPECT_EQ(types[1], "NodeB");
    EXPECT_EQ(types[2], "NodeC");
}

TEST_F(NodeFactoryRegistryTest, IsAvailableReturnsFalseForUnregistered) {
    EXPECT_FALSE(registry->IsAvailable("UnregisteredNode"));

    registry->Register("RegisteredNode", CreateThrowingFactory("registered"));

    EXPECT_TRUE(registry->IsAvailable("RegisteredNode"));
    EXPECT_FALSE(registry->IsAvailable("UnregisteredNode"));
}

TEST_F(NodeFactoryRegistryTest, CreateThrowsOnUnregisteredType) {
    registry->Register("Node1", CreateThrowingFactory("1"));

    EXPECT_THROW(
        registry->Create("UnregisteredNode"),
        std::runtime_error
    );
}

TEST_F(NodeFactoryRegistryTest, CreateThrowsWhenFactoryThrows) {
    registry->Register("FailingNode", CreateThrowingFactory("Factory error"));

    EXPECT_THROW(
        registry->Create("FailingNode"),
        std::runtime_error
    );
}

TEST_F(NodeFactoryRegistryTest, ClearRemovesAllFactories) {
    registry->Register("Node1", CreateThrowingFactory("1"));
    registry->Register("Node2", CreateThrowingFactory("2"));
    registry->Register("Node3", CreateThrowingFactory("3"));

    EXPECT_EQ(registry->GetRegistrySize(), 3);

    registry->Clear();

    EXPECT_EQ(registry->GetRegistrySize(), 0);
    EXPECT_FALSE(registry->IsAvailable("Node1"));
    EXPECT_FALSE(registry->IsAvailable("Node2"));
    EXPECT_FALSE(registry->IsAvailable("Node3"));
}

TEST_F(NodeFactoryRegistryTest, ClearAllowsReregistration) {
    registry->Register("Node", CreateThrowingFactory("1"));
    EXPECT_EQ(registry->GetRegistrySize(), 1);

    registry->Clear();
    EXPECT_EQ(registry->GetRegistrySize(), 0);

    registry->Register("Node", CreateThrowingFactory("2"));
    EXPECT_EQ(registry->GetRegistrySize(), 1);
    EXPECT_TRUE(registry->IsAvailable("Node"));
}

TEST_F(NodeFactoryRegistryTest, RegisterEmptyTypeNameThrows) {
    EXPECT_THROW(
        registry->Register("", CreateThrowingFactory("error")),
        std::invalid_argument
    );
}

TEST_F(NodeFactoryRegistryTest, LargeNumberOfRegistrations) {
    const int num_types = 100;

    for (int i = 0; i < num_types; ++i) {
        std::string type_name = "Node_" + std::to_string(i);
        registry->Register(type_name, CreateThrowingFactory(type_name));
    }

    EXPECT_EQ(registry->GetRegistrySize(), num_types);
    EXPECT_TRUE(registry->IsAvailable("Node_0"));
    EXPECT_TRUE(registry->IsAvailable("Node_50"));
    EXPECT_TRUE(registry->IsAvailable("Node_99"));
    EXPECT_FALSE(registry->IsAvailable("Node_100"));
}

TEST_F(NodeFactoryRegistryTest, RegistryPersistenceAcrossQueries) {
    registry->Register("Node", CreateThrowingFactory("test"));

    // Multiple queries should return consistent results
    EXPECT_TRUE(registry->IsAvailable("Node"));
    EXPECT_TRUE(registry->IsAvailable("Node"));
    EXPECT_EQ(registry->GetRegistrySize(), 1);
    EXPECT_EQ(registry->GetRegistrySize(), 1);

    auto types1 = registry->GetAvailableTypes();
    auto types2 = registry->GetAvailableTypes();
    EXPECT_EQ(types1.size(), types2.size());
}

// ============================================================================
// NodeFactory Tests
// ============================================================================

class NodeFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_shared<graph::NodeFactory>(nullptr);
    }

    std::shared_ptr<graph::NodeFactory> factory;
};

TEST_F(NodeFactoryTest, ConstructsSuccessfully) {
    EXPECT_NE(factory, nullptr);
    EXPECT_FALSE(factory->IsInitialized());
}

TEST_F(NodeFactoryTest, UnifiedRegistryIsAccessible) {
    auto registry = factory->GetUnifiedRegistry();
    EXPECT_NE(registry, nullptr);
    EXPECT_EQ(registry->GetRegistrySize(), 0);
}

TEST_F(NodeFactoryTest, InitializedStateIsFalse) {
    EXPECT_FALSE(factory->IsInitialized());
}

TEST_F(NodeFactoryTest, IsNodeTypeAvailableReturnsFalse) {
    // Before initialization, no types should be available
    EXPECT_FALSE(factory->IsNodeTypeAvailable("AnyNodeType"));
}

TEST_F(NodeFactoryTest, GetAvailableNodeTypesIsEmpty) {
    // Before initialization, should return empty list
    auto types = factory->GetAvailableNodeTypes();
    EXPECT_TRUE(types.empty());
}

TEST_F(NodeFactoryTest, CanRegisterViaUnifiedRegistry) {
    auto registry = factory->GetUnifiedRegistry();

    // Register a node type manually via unified registry
    registry->Register("TestNode", NodeFactoryRegistryTest::CreateThrowingFactory("test"));

    EXPECT_EQ(registry->GetRegistrySize(), 1);
    EXPECT_TRUE(registry->IsAvailable("TestNode"));
}

TEST_F(NodeFactoryTest, CreateNodeWithTemplateParameter) {
    // Simple test node for template creation
    struct SimpleNode {
        SimpleNode() = default;
    };

    auto node = factory->CreateNode<SimpleNode>();
    EXPECT_NE(node, nullptr);
}

TEST_F(NodeFactoryTest, CreateNodeWithConstructorArguments) {
    // Test node that takes constructor parameters
    struct ConfigurableNode {
        int value;
        std::string name;

        ConfigurableNode(int v, const std::string& n)
            : value(v), name(n) {}
    };

    auto node = factory->CreateNode<ConfigurableNode>(42, "test");
    EXPECT_NE(node, nullptr);
    EXPECT_EQ(node->value, 42);
    EXPECT_EQ(node->name, "test");
}

TEST_F(NodeFactoryTest, CreateNodeReturnsNullptrOnException) {
    // Test node that throws during construction
    struct FailingNode {
        FailingNode() {
            throw std::runtime_error("Construction failed");
        }
    };

    auto node = factory->CreateNode<FailingNode>();
    EXPECT_EQ(node, nullptr);
}

TEST_F(NodeFactoryTest, PluginRegistryIsOptional) {
    // Factory should work without plugin registry
    auto factory_without_plugins = std::make_shared<graph::NodeFactory>(nullptr);
    EXPECT_EQ(factory_without_plugins->GetPluginRegistry(), nullptr);
}

TEST_F(NodeFactoryTest, CanSetPluginRegistry) {
    // Test that plugin registry can be set after construction
    EXPECT_EQ(factory->GetPluginRegistry(), nullptr);

    // SetPluginRegistry exists but we can't easily test it without real plugin registry
    // Just verify the method exists and doesn't crash
    factory->SetPluginRegistry(nullptr);
    EXPECT_EQ(factory->GetPluginRegistry(), nullptr);
}

// ============================================================================
// Integration Tests
// ============================================================================

// Test node with static counter (defined at file scope)
struct CountedNode {
    static int instance_count;
    CountedNode() { instance_count++; }
};
int CountedNode::instance_count = 0;

TEST_F(NodeFactoryTest, UnifiedRegistryWorkflow) {
    // Simulate the unified registry workflow
    auto registry = factory->GetUnifiedRegistry();

    // Register multiple node types
    registry->Register("SensorNode", NodeFactoryRegistryTest::CreateThrowingFactory("sensor"));
    registry->Register("ProcessorNode", NodeFactoryRegistryTest::CreateThrowingFactory("processor"));
    registry->Register("AggregatorNode", NodeFactoryRegistryTest::CreateThrowingFactory("aggregator"));

    EXPECT_EQ(registry->GetRegistrySize(), 3);

    auto types = registry->GetAvailableTypes();
    EXPECT_EQ(types.size(), 3);

    // Verify each type is available
    for (const auto& type : types) {
        EXPECT_TRUE(registry->IsAvailable(type));
    }
}

TEST_F(NodeFactoryTest, TemplateCreationWithMultipleInstances) {
    CountedNode::instance_count = 0;

    auto node1 = factory->CreateNode<CountedNode>();
    EXPECT_NE(node1, nullptr);
    EXPECT_EQ(CountedNode::instance_count, 1);

    auto node2 = factory->CreateNode<CountedNode>();
    EXPECT_NE(node2, nullptr);
    EXPECT_EQ(CountedNode::instance_count, 2);

    auto node3 = factory->CreateNode<CountedNode>();
    EXPECT_NE(node3, nullptr);
    EXPECT_EQ(CountedNode::instance_count, 3);
}

TEST_F(NodeFactoryTest, TemplateCreationWithSharedPtr) {
    auto node1 = factory->CreateNode<CountedNode>();
    auto node2 = factory->CreateNode<CountedNode>();

    EXPECT_NE(node1, nullptr);
    EXPECT_NE(node2, nullptr);
    EXPECT_NE(node1, node2);  // Different instances
}

