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
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <string>

// ============================================================================
// Layer 6: End-to-End CSV Pipeline Integration Test
// Reference: config/graph_csv_pipeline_dual_port_integration.json
// ============================================================================

/**
 * @class CSVPipelineIntegrationTest
 * @brief Validates the complete 11-node, 14-edge CSV pipeline configuration
 */
class CSVPipelineIntegrationTest : public ::testing::Test {
protected:
    std::string config_file_ = "config/graph_csv_pipeline_dual_port_integration.json";
    nlohmann::json config_;

    void SetUp() override {
        if (ConfigFileExists()) {
            std::ifstream config_stream(config_file_);
            config_stream >> config_;
        }
    }

    bool ConfigFileExists() const {
        std::ifstream f(config_file_);
        return f.good();
    }
};

// ============================================================================
// Layer 6 Test Suite: Configuration Structure Validation
// ============================================================================

TEST_F(CSVPipelineIntegrationTest, ConfigFileExists) {
    EXPECT_TRUE(ConfigFileExists()) << "CSV pipeline config must exist at " << config_file_;
}

TEST_F(CSVPipelineIntegrationTest, ConfigFileIsValidJSON) {
    if (!ConfigFileExists()) GTEST_SKIP();

    std::ifstream config_stream(config_file_);
    EXPECT_NO_THROW({
        nlohmann::json test_config;
        config_stream >> test_config;
    }) << "Config file must contain valid JSON";
}

TEST_F(CSVPipelineIntegrationTest, ConfigHas11Nodes) {
    if (!ConfigFileExists()) GTEST_SKIP();

    ASSERT_TRUE(config_.contains("nodes"));
    EXPECT_EQ(config_["nodes"].size(), 11) << "Pipeline must have exactly 11 nodes";
}

TEST_F(CSVPipelineIntegrationTest, ConfigHas14Edges) {
    if (!ConfigFileExists()) GTEST_SKIP();

    ASSERT_TRUE(config_.contains("edges"));
    EXPECT_EQ(config_["edges"].size(), 14) << "Pipeline must have exactly 14 edges";
}

// ============================================================================
// Layer 6 Test Suite: Node Type Validation
// ============================================================================

TEST_F(CSVPipelineIntegrationTest, FiveCSVSensorsPresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    int csv_count = 0;
    for (const auto& node : config_["nodes"]) {
        std::string node_type = node["type"].get<std::string>();
        if (node_type.find("DataInjection") != std::string::npos) {
            csv_count++;
        }
    }
    EXPECT_EQ(csv_count, 5) << "Pipeline must have 5 CSV sensor nodes";
}

TEST_F(CSVPipelineIntegrationTest, FlightFSMNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "FlightFSMNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include FlightFSMNode";
}

TEST_F(CSVPipelineIntegrationTest, AltitudeFusionNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "AltitudeFusionNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include AltitudeFusionNode";
}

TEST_F(CSVPipelineIntegrationTest, EstimationPipelineNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "EstimationPipelineNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include EstimationPipelineNode";
}

TEST_F(CSVPipelineIntegrationTest, FlightMonitorNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "FlightMonitorNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include FlightMonitorNode";
}

TEST_F(CSVPipelineIntegrationTest, FeedbackTestSinkNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "FeedbackTestSinkNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include FeedbackTestSinkNode";
}

TEST_F(CSVPipelineIntegrationTest, CompletionAggregatorNodePresent) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& node : config_["nodes"]) {
        if (node["type"].get<std::string>() == "CompletionAggregatorNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Pipeline must include CompletionAggregatorNode";
}

// ============================================================================
// Layer 6 Test Suite: Dual-Port Architecture
// ============================================================================

TEST_F(CSVPipelineIntegrationTest, CSVSensorsHaveTwoPorts) {
    if (!ConfigFileExists()) GTEST_SKIP();

    for (const auto& node : config_["nodes"]) {
        std::string node_type = node["type"].get<std::string>();
        if (node_type.find("DataInjection") != std::string::npos) {
            int port_count = node["ports"].size();
            EXPECT_EQ(port_count, 2) << node_type << " must have 2 output ports";
        }
    }
}

TEST_F(CSVPipelineIntegrationTest, MainPipelineHas9Edges) {
    if (!ConfigFileExists()) GTEST_SKIP();

    int main_pipeline_count = 0;
    for (const auto& edge : config_["edges"]) {
        if (edge.contains("path") && edge["path"].get<std::string>() == "main_pipeline") {
            main_pipeline_count++;
        }
    }
    EXPECT_EQ(main_pipeline_count, 9);
}

TEST_F(CSVPipelineIntegrationTest, CompletionPathHas5Edges) {
    if (!ConfigFileExists()) GTEST_SKIP();

    int completion_count = 0;
    for (const auto& edge : config_["edges"]) {
        if (edge.contains("path") && edge["path"].get<std::string>() == "completion_path") {
            completion_count++;
        }
    }
    EXPECT_EQ(completion_count, 5);
}

// ============================================================================
// Layer 6 Test Suite: Pipeline Connectivity
// ============================================================================

TEST_F(CSVPipelineIntegrationTest, SensorConnectToFSM) {
    if (!ConfigFileExists()) GTEST_SKIP();

    int sensor_to_fsm = 0;
    for (const auto& edge : config_["edges"]) {
        std::string src = edge["source_node_id"].get<std::string>();
        std::string dst = edge["target_node_id"].get<std::string>();
        if (src.find("csv_") != std::string::npos && dst == "flight_fsm") {
            sensor_to_fsm++;
        }
    }
    EXPECT_EQ(sensor_to_fsm, 5) << "All 5 CSV sensors must connect to flight_fsm";
}

TEST_F(CSVPipelineIntegrationTest, FSMToAltitudeFusion) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& edge : config_["edges"]) {
        if (edge["source_node_id"].get<std::string>() == "flight_fsm" &&
            edge["target_node_id"].get<std::string>() == "altitude_fusion") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CSVPipelineIntegrationTest, AltitudeFusionToEstimation) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& edge : config_["edges"]) {
        if (edge["source_node_id"].get<std::string>() == "altitude_fusion" &&
            edge["target_node_id"].get<std::string>() == "estimation_pipeline") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CSVPipelineIntegrationTest, EstimationToMonitor) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& edge : config_["edges"]) {
        if (edge["source_node_id"].get<std::string>() == "estimation_pipeline" &&
            edge["target_node_id"].get<std::string>() == "flight_monitor") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CSVPipelineIntegrationTest, MonitorToFeedbackSink) {
    if (!ConfigFileExists()) GTEST_SKIP();

    bool found = false;
    for (const auto& edge : config_["edges"]) {
        if (edge["source_node_id"].get<std::string>() == "flight_monitor" &&
            edge["target_node_id"].get<std::string>() == "feedback_sink") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(CSVPipelineIntegrationTest, CompletionSignalsToAggregator) {
    if (!ConfigFileExists()) GTEST_SKIP();

    int completion_edges = 0;
    for (const auto& edge : config_["edges"]) {
        if (edge["target_node_id"].get<std::string>() == "completion_aggregator") {
            completion_edges++;
        }
    }
    EXPECT_EQ(completion_edges, 5);
}

// ============================================================================
// Layer 6 Test Suite: Configuration Parameters
// ============================================================================

TEST_F(CSVPipelineIntegrationTest, ExecutionConfigValid) {
    if (!ConfigFileExists()) GTEST_SKIP();

    ASSERT_TRUE(config_.contains("execution"));
    auto exec = config_["execution"];
    EXPECT_EQ(exec["thread_pool_size"], 4);
    EXPECT_TRUE(exec["deadlock_detection"]["enabled"]);
    EXPECT_EQ(exec["execution_timeout_s"], 10);
}

TEST_F(CSVPipelineIntegrationTest, TopologySummaryValid) {
    if (!ConfigFileExists()) GTEST_SKIP();

    ASSERT_TRUE(config_.contains("topology_summary"));
    auto summary = config_["topology_summary"];
    EXPECT_EQ(summary["total_nodes"], 11);
    EXPECT_EQ(summary["total_edges"], 14);
    EXPECT_EQ(summary["node_breakdown"]["csv_sensors"], 5);
    EXPECT_EQ(summary["node_breakdown"]["core_pipeline"], 5);
    EXPECT_EQ(summary["node_breakdown"]["completion_tracking"], 1);
}

