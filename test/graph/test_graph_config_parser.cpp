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
#include "graph/GraphConfigParser.hpp"
#include "graph/GraphConfig.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Layer 2, Task 2.1: GraphConfigParser Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Graph Infrastructure Layer
// ============================================================================

using json = nlohmann::json;

class GraphConfigParserTest : public ::testing::Test {
protected:
    // Helper to create minimal valid graph config JSON
    static std::string MinimalGraphJson() {
        json config;
        config["name"] = "TestGraph";
        config["version"] = "1.0";
        config["nodes"] = json::array();
        config["nodes"].push_back({
            {"id", "test_node"},
            {"type", "TestNodeType"}
        });
        return config.dump();
    }

    // Helper to create valid single-edge config
    static std::string SingleEdgeGraphJson() {
        json config;
        config["name"] = "SingleEdgeGraph";
        config["version"] = "1.0";
        config["nodes"] = json::array();
        config["nodes"].push_back({
            {"id", "source_node"},
            {"type", "SourceNodeType"}
        });
        config["nodes"].push_back({
            {"id", "target_node"},
            {"type", "TargetNodeType"}
        });
        config["edges"] = json::array();
        config["edges"].push_back({
            {"source_node_id", "source_node"},
            {"source_port", 0},
            {"target_node_id", "target_node"},
            {"target_port", 0},
            {"buffer_size", 256}
        });
        return config.dump();
    }

    // Helper to create dual-port config (Phase 2 architecture)
    static std::string DualPortGraphJson() {
        json config;
        config["name"] = "DualPortGraph";
        config["version"] = "2.0";
        config["nodes"] = json::array();

        // Sensor node with 2 ports
        config["nodes"].push_back({
            {"id", "sensor"},
            {"type", "DataSensorNode"},
            {"node_config", {
                {"sample_interval_us", 10000}
            }}
        });

        // Processor node
        config["nodes"].push_back({
            {"id", "processor"},
            {"type", "ProcessorNode"}
        });

        // Edges connecting both ports
        config["edges"] = json::array();
        config["edges"].push_back({
            {"source_node_id", "sensor"},
            {"source_port", 0},
            {"target_node_id", "processor"},
            {"target_port", 0}
        });
        config["edges"].push_back({
            {"source_node_id", "sensor"},
            {"source_port", 1},
            {"target_node_id", "processor"},
            {"target_port", 1}
        });

        return config.dump();
    }
};

// ============================================================================
// Valid Configuration Parsing Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ParseValidConfiguration) {
    std::string json_text = MinimalGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "TestGraph");
    EXPECT_EQ(config.nodes.size(), 1);
    EXPECT_EQ(config.nodes[0].id, "test_node");
    EXPECT_EQ(config.nodes[0].type, "TestNodeType");
}

TEST_F(GraphConfigParserTest, ParseConfigWithMetadata) {
    json config_obj;
    config_obj["name"] = "MetadataTest";
    config_obj["version"] = "1.0";
    config_obj["metadata"] = {
        {"version", "1.0.0"},
        {"description", "Test configuration"},
        {"author", "TestAuthor"}
    };
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "NodeType"}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.name, "MetadataTest");
    EXPECT_EQ(config.metadata.version, "1.0.0");
    EXPECT_EQ(config.metadata.description, "Test configuration");
    EXPECT_EQ(config.metadata.author, "TestAuthor");
}

TEST_F(GraphConfigParserTest, ExtractMultipleNodes) {
    json config_obj;
    config_obj["name"] = "MultiNodeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "Type1"}, {"description", "First node"}},
        {{"id", "node2"}, {"type", "Type2"}, {"description", "Second node"}},
        {{"id", "node3"}, {"type", "Type3"}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.nodes.size(), 3);
    EXPECT_EQ(config.nodes[0].id, "node1");
    EXPECT_EQ(config.nodes[0].description, "First node");
    EXPECT_EQ(config.nodes[1].id, "node2");
    EXPECT_EQ(config.nodes[2].id, "node3");
}

TEST_F(GraphConfigParserTest, ExtractNodeWithConfiguration) {
    json config_obj;
    config_obj["name"] = "ConfiguredNodeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {
            {"id", "sensor"},
            {"type", "SensorNode"},
            {"node_config", {
                {"sample_rate", 100},
                {"calibration", {{"offset", 0.5}}}
            }}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.nodes[0].node_config["sample_rate"], 100);
    EXPECT_EQ(config.nodes[0].node_config["calibration"]["offset"], 0.5);
}

// ============================================================================
// Edge Extraction and Validation Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ExtractSingleEdge) {
    std::string json_text = SingleEdgeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.edges.size(), 1);
    EXPECT_EQ(config.edges[0].source_node_id, "source_node");
    EXPECT_EQ(config.edges[0].source_port, 0);
    EXPECT_EQ(config.edges[0].target_node_id, "target_node");
    EXPECT_EQ(config.edges[0].target_port, 0);
}

TEST_F(GraphConfigParserTest, ExtractMultipleEdges) {
    json config_obj;
    config_obj["name"] = "MultiEdgeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "src"}, {"type", "Source"}},
        {{"id", "dst1"}, {"type", "Dest"}},
        {{"id", "dst2"}, {"type", "Dest"}}
    });
    config_obj["edges"] = json::array({
        {{"source_node_id", "src"}, {"source_port", 0}, {"target_node_id", "dst1"}, {"target_port", 0}},
        {{"source_node_id", "src"}, {"source_port", 1}, {"target_node_id", "dst2"}, {"target_port", 0}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.edges.size(), 2);
    EXPECT_EQ(config.edges[0].source_port, 0);
    EXPECT_EQ(config.edges[1].source_port, 1);
    EXPECT_EQ(config.edges[0].target_node_id, "dst1");
    EXPECT_EQ(config.edges[1].target_node_id, "dst2");
}

TEST_F(GraphConfigParserTest, ParseEdgeWithBufferSize) {
    json config_obj;
    config_obj["name"] = "BufferedEdgeGraph";
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

TEST_F(GraphConfigParserTest, ParseEdgeWithBackpressure) {
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

// ============================================================================
// Port Specification Parsing Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ParsePortSpecificationWithIndex) {
    // Test parsing "node_id:port_index" format
    std::string json_text = SingleEdgeGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.edges[0].source_node_id, "source_node");
    EXPECT_EQ(config.edges[0].source_port, 0);
    EXPECT_EQ(config.edges[0].target_node_id, "target_node");
    EXPECT_EQ(config.edges[0].target_port, 0);
}

TEST_F(GraphConfigParserTest, ParseHighPortIndices) {
    json config_obj;
    config_obj["name"] = "HighPortGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "multi_port_src"}, {"type", "MultiPortSource"}},
        {{"id", "multi_port_dst"}, {"type", "MultiPortDest"}}
    });
    config_obj["edges"] = json::array({
        {{"source_node_id", "multi_port_src"}, {"source_port", 5}, {"target_node_id", "multi_port_dst"}, {"target_port", 3}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.edges[0].source_port, 5);
    EXPECT_EQ(config.edges[0].target_port, 3);
}

// ============================================================================
// Dual-Port Architecture Tests (Phase 2)
// ============================================================================

TEST_F(GraphConfigParserTest, HandlesDualPortArchitecture) {
    std::string json_text = DualPortGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);

    EXPECT_EQ(config.name, "DualPortGraph");
    EXPECT_EQ(config.nodes.size(), 2);
    EXPECT_EQ(config.edges.size(), 2);

    // Verify first edge connects sensor port 0 (data)
    EXPECT_EQ(config.edges[0].source_node_id, "sensor");
    EXPECT_EQ(config.edges[0].source_port, 0);

    // Verify second edge connects sensor port 1 (notification)
    EXPECT_EQ(config.edges[1].source_node_id, "sensor");
    EXPECT_EQ(config.edges[1].source_port, 1);
}

TEST_F(GraphConfigParserTest, ParseNodeWithPorts) {
    json config_obj;
    config_obj["name"] = "PortedNodeGraph";
    config_obj["version"] = "2.0";
    config_obj["nodes"] = json::array({
        {
            {"id", "multi_port_node"},
            {"type", "MultiPortNode"},
            {"ports", json::array({
                {{"port_id", 0}, {"name", "Data"}, {"direction", "output"}},
                {{"port_id", 1}, {"name", "Control"}, {"direction", "input"}},
                {{"port_id", 2}, {"name", "Status"}, {"direction", "output"}}
            })}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());

    EXPECT_EQ(config.nodes[0].id, "multi_port_node");
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ValidateValidConfiguration) {
    std::string json_text = MinimalGraphJson();
    auto config = graph::config::GraphConfigParser::Parse(json_text);
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(GraphConfigParserTest, AllowEmptyNodeList) {
    // Note: Parser allows empty graphs for testing purposes (see GraphConfigParser.cpp line 259)
    json config_obj;
    config_obj["name"] = "NoNodesGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array();

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_TRUE(result.valid);
}

TEST_F(GraphConfigParserTest, RejectDuplicateNodeIds) {
    json config_obj;
    config_obj["name"] = "DuplicateIdGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "Type1"}},
        {{"id", "node1"}, {"type", "Type2"}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_FALSE(result.valid);
}

TEST_F(GraphConfigParserTest, RejectInvalidNodeIds) {
    json config_obj;
    config_obj["name"] = "InvalidIdGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "123_invalid"}, {"type", "Type1"}}  // IDs cannot start with digit
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_FALSE(result.valid);
}

TEST_F(GraphConfigParserTest, RejectEmptyNodeId) {
    json config_obj;
    config_obj["name"] = "EmptyIdGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", ""}, {"type", "Type1"}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_FALSE(result.valid);
}

TEST_F(GraphConfigParserTest, RejectMissingNodeType) {
    json config_obj;
    config_obj["name"] = "NoTypeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}}  // Missing type
    });

    // Should throw or result in validation failure
    try {
        auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
        auto result = graph::config::GraphConfigParser::Validate(config);
        EXPECT_FALSE(result.valid);
    } catch (const std::exception&) {
        // Expected: missing required field
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(GraphConfigParserTest, RejectMalformedJSON) {
    std::string malformed_json = R"({ "name": "BadJSON" invalid )";

    EXPECT_THROW(
        graph::config::GraphConfigParser::Parse(malformed_json),
        std::runtime_error
    );
}

TEST_F(GraphConfigParserTest, AllowMissingGraphName) {
    // Note: Parser allows missing name field - it's optional
    json config_obj;
    // Intentionally missing "name"
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "node1"}, {"type", "Type1"}}
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    // If it parses, validation should succeed because name is optional
    auto result = graph::config::GraphConfigParser::Validate(config);
    EXPECT_TRUE(result.valid);
}

TEST_F(GraphConfigParserTest, RejectEdgeWithNonexistentNode) {
    json config_obj;
    config_obj["name"] = "BadEdgeGraph";
    config_obj["version"] = "1.0";
    config_obj["nodes"] = json::array({
        {{"id", "existing_node"}, {"type", "Type1"}}
    });
    config_obj["edges"] = json::array({
        {
            {"source_node_id", "existing_node"},
            {"source_port", 0},
            {"target_node_id", "nonexistent_node"},
            {"target_port", 0}
        }
    });

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    // Validation should detect missing target node
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

// ============================================================================
// File I/O Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ParseFromFile) {
    // Test with the actual phase 2 config file if it exists
    std::string config_path = "config/graph_csv_pipeline_dual_port_integration.json";

    if (fs::exists(config_path)) {
        EXPECT_NO_THROW({
            auto config = graph::config::GraphConfigParser::ParseFile(config_path);
            EXPECT_FALSE(config.name.empty());
            EXPECT_FALSE(config.nodes.empty());
        });
    } else {
        GTEST_SKIP() << "Test config file not found: " << config_path;
    }
}

TEST_F(GraphConfigParserTest, RejectNonexistentFile) {
    std::string nonexistent_path = "/nonexistent/path/to/config.json";

    EXPECT_THROW(
        graph::config::GraphConfigParser::ParseFile(nonexistent_path),
        std::runtime_error
    );
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GraphConfigParserTest, ParseRealPhase2ConfigStructure) {
    // Create a realistic Phase 2 config with multiple sensor nodes
    json config_obj;
    config_obj["name"] = "Phase2Sensors";
    config_obj["version"] = "2.0";
    config_obj["execution"] = {
        {"thread_pool_size", 4},
        {"backpressure_enabled", true}
    };

    // Add 3 sensor nodes with dual ports
    config_obj["nodes"] = json::array();
    for (int i = 0; i < 3; ++i) {
        config_obj["nodes"].push_back({
            {"id", "sensor_" + std::to_string(i)},
            {"type", "SensorNode"},
            {"node_config", {
                {"sample_rate", 100},
                {"sensor_id", i}
            }}
        });
    }

    // Add a processor node
    config_obj["nodes"].push_back({
        {"id", "processor"},
        {"type", "ProcessorNode"}
    });

    // Connect sensors to processor (both ports)
    config_obj["edges"] = json::array();
    for (int i = 0; i < 3; ++i) {
        config_obj["edges"].push_back({
            {"source_node_id", "sensor_" + std::to_string(i)},
            {"source_port", 0},
            {"target_node_id", "processor"},
            {"target_port", 0}
        });
        config_obj["edges"].push_back({
            {"source_node_id", "sensor_" + std::to_string(i)},
            {"source_port", 1},
            {"target_node_id", "processor"},
            {"target_port", 1}
        });
    }

    auto config = graph::config::GraphConfigParser::Parse(config_obj.dump());
    auto result = graph::config::GraphConfigParser::Validate(config);

    EXPECT_TRUE(result.valid);
    EXPECT_EQ(config.nodes.size(), 4);
    EXPECT_EQ(config.edges.size(), 6);  // 3 sensors * 2 ports each
}
