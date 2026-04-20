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
#include <memory>

// Real node headers
#include "sensor/DataInjectionAccelerometerNode.hpp"
#include "sensor/DataInjectionGyroscopeNode.hpp"
#include "sensor/DataInjectionBarometricNode.hpp"
#include "sensor/DataInjectionGPSPositionNode.hpp"
#include "sensor/DataInjectionMagnetometerNode.hpp"
#include "avionics/nodes/FlightLoggerNode.hpp"

// Type system
#include "sensor/SensorDataTypes.hpp"
#include "graph/Message.hpp"
#include "graph/GraphManager.hpp"

// ============================================================================
// Layer 3, Task 3.3: Real Node Integration Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Real Node Integration
// ============================================================================

// ============================================================================
// Test Fixture for Real Node Integration
// ============================================================================

class RealNodeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        graph_manager_ = std::make_shared<graph::GraphManager>();
    }

    void TearDown() override {
        // Ensure graph is cleaned up
        if (graph_manager_) {
            graph_manager_->Clear();
        }
    }

    std::shared_ptr<graph::GraphManager> graph_manager_;
};

// ============================================================================
// Test Cases: Single Sensor Type Integration
// ============================================================================

TEST_F(RealNodeIntegrationTest, Accelerometer_NodeCreation) {
    // Verify AccelerometerNode can be created and added to graph
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    ASSERT_NE(accel_node, nullptr);
    EXPECT_EQ(graph_manager_->NodeCount(), 1);
}

TEST_F(RealNodeIntegrationTest, Accelerometer_With_Logger) {
    // Verify AccelerometerNode can be connected to FlightLoggerNode
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(accel_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);
}

TEST_F(RealNodeIntegrationTest, Gyroscope_With_Logger) {
    // Verify GyroscopeNode can be created and connected
    auto gyro_node = graph_manager_->AddNode<sensors::DataInjectionGyroscopeNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(gyro_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    graph_manager_->AddEdge<sensors::DataInjectionGyroscopeNode, 0, avionics::FlightLoggerNode, 0>(
        gyro_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);
}

TEST_F(RealNodeIntegrationTest, Barometric_With_Logger) {
    // Verify BarometricNode can be created and connected
    auto baro_node = graph_manager_->AddNode<sensors::DataInjectionBarometricNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(baro_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    graph_manager_->AddEdge<sensors::DataInjectionBarometricNode, 0, avionics::FlightLoggerNode, 0>(
        baro_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);
}

TEST_F(RealNodeIntegrationTest, GPS_With_Logger) {
    // Verify GPSNode can be created and connected
    auto gps_node = graph_manager_->AddNode<sensors::DataInjectionGPSPositionNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(gps_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    graph_manager_->AddEdge<sensors::DataInjectionGPSPositionNode, 0, avionics::FlightLoggerNode, 0>(
        gps_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);
}

TEST_F(RealNodeIntegrationTest, Magnetometer_With_Logger) {
    // Verify MagnetometerNode can be created and connected
    auto mag_node = graph_manager_->AddNode<sensors::DataInjectionMagnetometerNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(mag_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    graph_manager_->AddEdge<sensors::DataInjectionMagnetometerNode, 0, avionics::FlightLoggerNode, 0>(
        mag_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);
}

// ============================================================================
// Test Cases: Multiple Sensor Integration
// ============================================================================

TEST_F(RealNodeIntegrationTest, MultiSensor_All_To_Logger) {
    // Verify all 5 sensor types can be created and added to graph
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto gyro_node = graph_manager_->AddNode<sensors::DataInjectionGyroscopeNode>();
    auto mag_node = graph_manager_->AddNode<sensors::DataInjectionMagnetometerNode>();
    auto baro_node = graph_manager_->AddNode<sensors::DataInjectionBarometricNode>();
    auto gps_node = graph_manager_->AddNode<sensors::DataInjectionGPSPositionNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    ASSERT_NE(accel_node, nullptr);
    ASSERT_NE(gyro_node, nullptr);
    ASSERT_NE(mag_node, nullptr);
    ASSERT_NE(baro_node, nullptr);
    ASSERT_NE(gps_node, nullptr);
    ASSERT_NE(logger_node, nullptr);

    // Verify all nodes created
    EXPECT_EQ(graph_manager_->NodeCount(), 6);
}

TEST_F(RealNodeIntegrationTest, MultiSensor_Edge_Creation) {
    // Verify edges can be created from multiple sensors to logger
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto gyro_node = graph_manager_->AddNode<sensors::DataInjectionGyroscopeNode>();
    auto baro_node = graph_manager_->AddNode<sensors::DataInjectionBarometricNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    // Create edges
    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel_node, logger_node);
    graph_manager_->AddEdge<sensors::DataInjectionGyroscopeNode, 0, avionics::FlightLoggerNode, 0>(
        gyro_node, logger_node);
    graph_manager_->AddEdge<sensors::DataInjectionBarometricNode, 0, avionics::FlightLoggerNode, 0>(
        baro_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 4);
    EXPECT_EQ(graph_manager_->EdgeCount(), 3);
}

// ============================================================================
// Test Cases: Graph Lifecycle
// ============================================================================

TEST_F(RealNodeIntegrationTest, Graph_Init_With_Real_Nodes) {
    // Verify graph can be initialized with real nodes
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel_node, logger_node);

    // Initialize graph
    EXPECT_TRUE(graph_manager_->Init());
}

TEST_F(RealNodeIntegrationTest, Graph_Clear_After_Creation) {
    // Verify graph can be cleared after creating nodes
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel_node, logger_node);

    EXPECT_EQ(graph_manager_->NodeCount(), 2);
    EXPECT_EQ(graph_manager_->EdgeCount(), 1);

    // Clear graph
    graph_manager_->Clear();

    EXPECT_EQ(graph_manager_->NodeCount(), 0);
    EXPECT_EQ(graph_manager_->EdgeCount(), 0);
}

// ============================================================================
// Test Cases: Node Type Compatibility
// ============================================================================

TEST_F(RealNodeIntegrationTest, SensorPayload_Creation) {
    // Verify SensorPayload variant can hold all sensor types
    sensors::AccelerometerData accel(1.0f, 2.0f, 3.0f);
    sensors::SensorPayload payload1 = accel;
    ASSERT_TRUE(std::holds_alternative<sensors::AccelerometerData>(payload1));

    sensors::GyroscopeData gyro(0.1f, 0.2f, 0.3f);
    sensors::SensorPayload payload2 = gyro;
    ASSERT_TRUE(std::holds_alternative<sensors::GyroscopeData>(payload2));

    sensors::BarometricData baro(101325.0f, 288.15f);
    sensors::SensorPayload payload3 = baro;
    ASSERT_TRUE(std::holds_alternative<sensors::BarometricData>(payload3));
}

TEST_F(RealNodeIntegrationTest, Injection_Queue_Accessible) {
    // Verify DataInjectionNodes provide access to injection queue
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    ASSERT_NE(accel_node, nullptr);

    // Verify GetInjectionQueue() is accessible and returns queue
    auto& queue = accel_node->GetInjectionQueue();
    (void)queue;  // Just verify it's accessible

    EXPECT_EQ(graph_manager_->NodeCount(), 1);
}

// ============================================================================
// Test Cases: Multiple Pipelines
// ============================================================================

TEST_F(RealNodeIntegrationTest, Independent_Accel_Pipelines) {
    // Verify multiple independent accelerometer pipelines can coexist
    auto accel1 = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger1 = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    auto accel2 = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger2 = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    // Connect both pipelines
    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel1, logger1);
    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel2, logger2);

    EXPECT_EQ(graph_manager_->NodeCount(), 4);
    EXPECT_EQ(graph_manager_->EdgeCount(), 2);
}

TEST_F(RealNodeIntegrationTest, Mixed_Sensor_Pipelines) {
    // Verify mixed sensor type pipelines can coexist
    auto accel = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger1 = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    auto gyro = graph_manager_->AddNode<sensors::DataInjectionGyroscopeNode>();
    auto logger2 = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    // Connect pipelines
    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel, logger1);
    graph_manager_->AddEdge<sensors::DataInjectionGyroscopeNode, 0, avionics::FlightLoggerNode, 0>(
        gyro, logger2);

    EXPECT_EQ(graph_manager_->NodeCount(), 4);
    EXPECT_EQ(graph_manager_->EdgeCount(), 2);
}

// ============================================================================
// Test Cases: Error Handling
// ============================================================================

TEST_F(RealNodeIntegrationTest, Duplicate_Node_Creation) {
    // Verify multiple nodes of same type can be created
    auto accel1 = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto accel2 = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto accel3 = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();

    ASSERT_NE(accel1, nullptr);
    ASSERT_NE(accel2, nullptr);
    ASSERT_NE(accel3, nullptr);

    EXPECT_EQ(graph_manager_->NodeCount(), 3);
}

TEST_F(RealNodeIntegrationTest, Graph_Lifecycle_With_Real_Nodes) {
    // Verify graph can be initialized with real nodes (without Start to avoid hanging on no data)
    auto accel_node = graph_manager_->AddNode<sensors::DataInjectionAccelerometerNode>();
    auto logger_node = graph_manager_->AddNode<avionics::FlightLoggerNode>();

    graph_manager_->AddEdge<sensors::DataInjectionAccelerometerNode, 0, avionics::FlightLoggerNode, 0>(
        accel_node, logger_node);

    // Init
    EXPECT_TRUE(graph_manager_->Init());

    // Note: Skip Start/Stop/Join to avoid waiting on data injection
    // Real data-driven lifecycle tested in Layer 4 with policies

    // Clear for next test
    graph_manager_->Clear();
    EXPECT_EQ(graph_manager_->NodeCount(), 0);
}
