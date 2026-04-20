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
#include "graph/JsonDynamicGraphLoader.hpp"
#include "graph/GraphConfigParser.hpp"
#include "graph/NodeFactory.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// ============================================================================
// Layer 2, Task 2.2: JsonDynamicGraphLoader Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Graph Infrastructure Layer
// ============================================================================

using json = nlohmann::json;

// ============================================================================
// Mock NodeFactory for Testing
// ============================================================================

/**
 * Mock implementation of NodeFactory that returns controllable node adapters
 * and tracks creation calls for testing.
 */
class MockNodeFactory : public graph::NodeFactory {
public:
    MockNodeFactory() : graph::NodeFactory(nullptr) {
        created_nodes_.clear();
        creation_calls_.clear();
    }

    /**
     * Override CreateNode to return controllable mock nodes
     */
    graph::NodeFacadeAdapter CreateNode(const std::string& node_type_name) override {
        creation_calls_.push_back(node_type_name);

        if (should_fail_on_type_ == node_type_name) {
            throw std::runtime_error("Mock factory: node type not found - " + node_type_name);
        }

        // For testing, return a "dummy" adapter
        // In real tests, this would be replaced with actual node creation
        throw std::runtime_error("Mock factory not fully implemented - use TestNodeFacadeAdapter");
    }

    // Helper to control failure behavior
    void SetFailureType(const std::string& type_name) {
        should_fail_on_type_ = type_name;
    }

    // Tracking
    const std::vector<std::string>& GetCreationCalls() const {
        return creation_calls_;
    }

    void ClearTracking() {
        creation_calls_.clear();
        created_nodes_.clear();
    }

private:
    std::vector<std::string> creation_calls_;
    std::vector<std::shared_ptr<graph::NodeFacadeAdapter>> created_nodes_;
    std::string should_fail_on_type_;
};

// ============================================================================
// Test Fixture
// ============================================================================

class JsonDynamicGraphLoaderTest : public ::testing::Test {
protected:
    // Helper to create minimal valid graph JSON
    static std::string MinimalGraphJson() {
        json config;
        config["name"] = "TestGraph";
        config["version"] = "1.0";
        config["nodes"] = json::array({
            {{"id", "node1"}, {"type", "TestNodeType"}}
        });
        return config.dump();
    }

    // Helper to create single-node graph
    static std::string SingleNodeGraphJson() {
        json config;
        config["name"] = "SingleNodeGraph";
        config["version"] = "1.0";
        config["nodes"] = json::array({
            {
                {"id", "sensor"},
                {"type", "SensorNode"},
                {"node_config", {{"sample_rate", 100}}}
            }
        });
        return config.dump();
    }

    // Helper to create multi-node graph
    static std::string MultiNodeGraphJson() {
        json config;
        config["name"] = "MultiNodeGraph";
        config["version"] = "1.0";
        config["nodes"] = json::array({
            {{"id", "source"}, {"type", "SourceNode"}},
            {{"id", "processor"}, {"type", "ProcessorNode"}},
            {{"id", "sink"}, {"type", "SinkNode"}}
        });
        config["edges"] = json::array({
            {
                {"source_node_id", "source"},
                {"source_port", 0},
                {"target_node_id", "processor"},
                {"target_port", 0}
            },
            {
                {"source_node_id", "processor"},
                {"source_port", 0},
                {"target_node_id", "sink"},
                {"target_port", 0}
            }
        });
        return config.dump();
    }

    // Helper to create Phase 2 dual-port graph
    static std::string DualPortGraphJson() {
        json config;
        config["name"] = "DualPortGraph";
        config["version"] = "2.0";
        config["nodes"] = json::array();

        // Sensor with dual ports (data + notification)
        config["nodes"].push_back({
            {"id", "sensor_accel"},
            {"type", "AccelerometerSensor"},
            {"node_config", {{"sample_rate", 100}}}
        });

        // Processor node
        config["nodes"].push_back({
            {"id", "processor"},
            {"type", "DataProcessor"}
        });

        // Aggregator node
        config["nodes"].push_back({
            {"id", "aggregator"},
            {"type", "CompletionAggregator"}
        });

        // Edges: Port 0 = data, Port 1 = completion/notification
        config["edges"] = json::array({
            // Data path
            {
                {"source_node_id", "sensor_accel"},
                {"source_port", 0},
                {"target_node_id", "processor"},
                {"target_port", 0}
            },
            // Notification path
            {
                {"source_node_id", "sensor_accel"},
                {"source_port", 1},
                {"target_node_id", "aggregator"},
                {"target_port", 0}
            }
        });
        return config.dump();
    }

    // Helper to write JSON to temporary file
    static std::string WriteConfigToFile(const std::string& config_json) {
        std::string filepath = "/tmp/test_graph_config_" +
            std::to_string(std::rand()) + ".json";
        std::ofstream file(filepath);
        file << config_json;
        file.close();
        return filepath;
    }
};

// ============================================================================
// Configuration Parsing Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, ParseValidMinimalConfiguration) {
    std::string json_text = MinimalGraphJson();

    // Verify GraphConfigParser can parse it
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "TestGraph");
    EXPECT_EQ(config.nodes.size(), 1);
    EXPECT_EQ(config.nodes[0].id, "node1");
    EXPECT_EQ(config.nodes[0].type, "TestNodeType");
}

TEST_F(JsonDynamicGraphLoaderTest, ParseSingleNodeConfiguration) {
    std::string json_text = SingleNodeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "SingleNodeGraph");
    EXPECT_EQ(config.nodes.size(), 1);
    EXPECT_EQ(config.nodes[0].id, "sensor");
    EXPECT_EQ(config.nodes[0].type, "SensorNode");
    EXPECT_EQ(config.nodes[0].node_config["sample_rate"], 100);
}

TEST_F(JsonDynamicGraphLoaderTest, ParseMultiNodeConfiguration) {
    std::string json_text = MultiNodeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "MultiNodeGraph");
    EXPECT_EQ(config.nodes.size(), 3);
    EXPECT_EQ(config.edges.size(), 2);
}

TEST_F(JsonDynamicGraphLoaderTest, ParseDualPortArchitecture) {
    std::string json_text = DualPortGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "DualPortGraph");
    EXPECT_EQ(config.nodes.size(), 3);
    EXPECT_EQ(config.edges.size(), 2);

    // Verify dual-port edges
    EXPECT_EQ(config.edges[0].source_port, 0);  // Data port
    EXPECT_EQ(config.edges[1].source_port, 1);  // Notification port
}

// ============================================================================
// Edge Extraction Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, ExtractEdgesFromConfiguration) {
    std::string json_text = MultiNodeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.edges.size(), 2);

    // Verify first edge
    EXPECT_EQ(config.edges[0].source_node_id, "source");
    EXPECT_EQ(config.edges[0].source_port, 0);
    EXPECT_EQ(config.edges[0].target_node_id, "processor");
    EXPECT_EQ(config.edges[0].target_port, 0);

    // Verify second edge
    EXPECT_EQ(config.edges[1].source_node_id, "processor");
    EXPECT_EQ(config.edges[1].target_node_id, "sink");
}

TEST_F(JsonDynamicGraphLoaderTest, ValidateEdgeConnectivity) {
    std::string json_text = MultiNodeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);
    auto result = graph::config::GraphConfigParser::Validate(config);

    // All edges should point to existing nodes
    EXPECT_TRUE(result.valid);
}

TEST_F(JsonDynamicGraphLoaderTest, RejectEdgesWithMissingNodes) {
    json config_obj;
    config_obj["name"] = "InvalidEdgeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "Type1"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "node1"},
            {"source_port", 0},
            {"target_node_id", "nonexistent"},
            {"target_port", 0}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_FALSE(result.valid);
}

// ============================================================================
// Port Validation Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, ValidatePortSpecifications) {
    json config_obj;
    config_obj["name"] = "PortTest";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "src"}, {"type", "Source"}},
        {{"id", "dst"}, {"type", "Dest"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "src"},
            {"source_port", 0},
            {"target_node_id", "dst"},
            {"target_port", 0}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.edges[0].source_port, 0);
    EXPECT_EQ(config.edges[0].target_port, 0);
}

TEST_F(JsonDynamicGraphLoaderTest, HandleHighPortIndices) {
    json config_obj;
    config_obj["name"] = "HighPortTest";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "multi_port_src"}, {"type", "MultiPortSource"}},
        {{"id", "multi_port_dst"}, {"type", "MultiPortDest"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "multi_port_src"},
            {"source_port", 5},
            {"target_node_id", "multi_port_dst"},
            {"target_port", 3}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.edges[0].source_port, 5);
    EXPECT_EQ(config.edges[0].target_port, 3);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, RejectMalformedJSON) {
    std::string malformed_json = R"({ "name": "BadJSON" invalid )";

    EXPECT_THROW(
        graph::config::GraphConfigParser::Parse(malformed_json),
        std::runtime_error
    );
}

TEST_F(JsonDynamicGraphLoaderTest, RejectMissingNodesArray) {
    json config_obj;
    config_obj["name"] = "NoNodesGraph";
    config_obj["version"] = "1.0";
    // Intentionally missing "nodes" array

    EXPECT_THROW(
        graph::config::GraphConfigParser::Parse(config_obj.dump()),
        std::runtime_error
    );
}

TEST_F(JsonDynamicGraphLoaderTest, RejectDuplicateNodeIds) {
    json config_obj;
    config_obj["name"] = "DuplicateGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "Type1"}},
        {{"id", "node1"}, {"type", "Type2"}}  // Duplicate ID
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_FALSE(result.valid);
}

// ============================================================================
// File I/O Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, ParseFromFile) {
    std::string config_json = MinimalGraphJson();
    std::string filepath = WriteConfigToFile(config_json);

    // Should be able to parse from file
    auto config = graph::config::GraphConfigParser::ParseFile(filepath);

    EXPECT_EQ(config.name, "TestGraph");
    EXPECT_EQ(config.nodes.size(), 1);

    // Cleanup
    std::remove(filepath.c_str());
}

TEST_F(JsonDynamicGraphLoaderTest, RejectNonexistentFile) {
    std::string nonexistent_path = "/nonexistent/path/to/config_" +
        std::to_string(std::rand()) + ".json";

    EXPECT_THROW(
        graph::config::GraphConfigParser::ParseFile(nonexistent_path),
        std::runtime_error
    );
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(JsonDynamicGraphLoaderTest, LoadCompleteMultiSensorPipeline) {
    // Realistic Phase 2 configuration with multiple sensors
    json config_obj;
    config_obj["name"] = "Phase2Pipeline";
    config_obj["version"] = "2.0";
    config_obj["execution"] = {
        {"thread_pool_size", 4},
        {"backpressure_enabled", true}
    };

    // Create 3 sensor nodes
    config_obj["nodes"] = json::array();
    for (int i = 0; i < 3; ++i) {
        config_obj["nodes"].push_back({
            {"id", "sensor_" + std::to_string(i)},
            {"type", "SensorNode"},
            {"node_config", {{"sensor_id", i}}}
        });
    }

    // Add processor and aggregator
    config_obj["nodes"].push_back({
        {"id", "processor"},
        {"type", "ProcessorNode"}
    });
    config_obj["nodes"].push_back({
        {"id", "aggregator"},
        {"type", "CompletionAggregator"}
    });

    // Connect sensors to processor (dual-port: data + notification)
    config_obj["edges"] = json::array();
    for (int i = 0; i < 3; ++i) {
        // Data path
        config_obj["edges"].push_back({
            {"source_node_id", "sensor_" + std::to_string(i)},
            {"source_port", 0},
            {"target_node_id", "processor"},
            {"target_port", 0}
        });
        // Notification path
        config_obj["edges"].push_back({
            {"source_node_id", "sensor_" + std::to_string(i)},
            {"source_port", 1},
            {"target_node_id", "aggregator"},
            {"target_port", 0}
        });
    }

    // Parse and validate
    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_TRUE(result.valid);
    EXPECT_EQ(config.nodes.size(), 5);    // 3 sensors + processor + aggregator
    EXPECT_EQ(config.edges.size(), 6);    // 3 * 2 dual-port connections
}

TEST_F(JsonDynamicGraphLoaderTest, LoadGraphWithNodeConfiguration) {
    // Graph with per-node configuration
    json config_obj;
    config_obj["name"] = "ConfiguredGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {
            {"id", "sensor"},
            {"type", "SensorNode"},
            {"node_config", {
                {"sample_rate", 500},
                {"calibration", {
                    {"offset", 0.5},
                    {"scale", 2.0}
                }}
            }}
        },
        {
            {"id", "processor"},
            {"type", "ProcessorNode"},
            {"node_config", {
                {"threshold", 10.5},
                {"mode", "fusion"}
            }}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    // Verify configurations were parsed
    EXPECT_EQ(config.nodes[0].node_config["sample_rate"], 500);
    EXPECT_EQ(config.nodes[0].node_config["calibration"]["offset"], 0.5);
    EXPECT_EQ(config.nodes[1].node_config["threshold"], 10.5);
}

TEST_F(JsonDynamicGraphLoaderTest, LoadGraphWithEdgeBufferSizes) {
    json config_obj;
    config_obj["name"] = "BufferedGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "src"}, {"type", "Source"}},
        {{"id", "dst"}, {"type", "Dest"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "src"},
            {"source_port", 0},
            {"target_node_id", "dst"},
            {"target_port", 0},
            {"buffer_size", 512}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.edges[0].buffer_size, 512);
}

TEST_F(JsonDynamicGraphLoaderTest, LoadGraphWithBackpressureSettings) {
    json config_obj;
    config_obj["name"] = "BackpressureGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "src"}, {"type", "Source"}},
        {{"id", "dst"}, {"type", "Dest"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "src"},
            {"source_port", 0},
            {"target_node_id", "dst"},
            {"target_port", 0},
            {"backpressure_enabled", false}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_FALSE(config.edges[0].backpressure_enabled);
}
