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
#include "sensor/DataInjectionAccelerometerNode.hpp"
#include "sensor/DataInjectionGyroscopeNode.hpp"
#include "sensor/DataInjectionMagnetometerNode.hpp"
#include "sensor/DataInjectionBarometricNode.hpp"
#include "sensor/DataInjectionGPSPositionNode.hpp"
#include <memory>

// ============================================================================
// Layer 5, Task 5.1: DataInjectionNode Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - DataInjectionNode Tests
// ============================================================================

// ============================================================================
// DataInjectionAccelerometerNode Tests
// ============================================================================

class DataInjectionAccelerometerNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<sensors::DataInjectionAccelerometerNode> node_;

    void SetUp() override {
        node_ = std::make_unique<sensors::DataInjectionAccelerometerNode>();
        node_->SetName("accel_node");
    }
};

TEST_F(DataInjectionAccelerometerNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "accel_node");
}

TEST_F(DataInjectionAccelerometerNodeTest, HasDualPorts) {
    // DataInjectionAccelerometerNode has 2 ports:
    // Port 0: Data output (AccelerometerData)
    // Port 1: Notification output (CompletionSignal)
    EXPECT_STREQ(node_->kDataPort, "Accel");
    EXPECT_STREQ(node_->kNotifyPort, "Notify");
}

TEST_F(DataInjectionAccelerometerNodeTest, PortMetadata) {
    // Verify port specs are correctly defined
    using PortsTuple = sensors::DataInjectionAccelerometerNode::Ports;
    constexpr size_t num_ports = std::tuple_size_v<PortsTuple>;
    EXPECT_EQ(num_ports, 2);  // Port 0 (data) + Port 1 (notify)
}

TEST_F(DataInjectionAccelerometerNodeTest, IsDataInjectionSource) {
    // Node should implement IDataInjectionSource interface
    auto* injection_source = dynamic_cast<graph::datasources::IDataInjectionSource*>(node_.get());
    EXPECT_NE(injection_source, nullptr);
}

TEST_F(DataInjectionAccelerometerNodeTest, IsConfigurable) {
    // Node should implement IConfigurable interface
    auto* configurable = dynamic_cast<graph::IConfigurable*>(node_.get());
    EXPECT_NE(configurable, nullptr);
}

// ============================================================================
// DataInjectionGyroscopeNode Tests
// ============================================================================

class DataInjectionGyroscopeNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<sensors::DataInjectionGyroscopeNode> node_;

    void SetUp() override {
        node_ = std::make_unique<sensors::DataInjectionGyroscopeNode>();
        node_->SetName("gyro_node");
    }
};

TEST_F(DataInjectionGyroscopeNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "gyro_node");
}

TEST_F(DataInjectionGyroscopeNodeTest, HasDualPorts) {
    EXPECT_STREQ(node_->kDataPort, "Gyro");
    EXPECT_STREQ(node_->kNotifyPort, "Notify");
}

TEST_F(DataInjectionGyroscopeNodeTest, PortMetadata) {
    using PortsTuple = sensors::DataInjectionGyroscopeNode::Ports;
    constexpr size_t num_ports = std::tuple_size_v<PortsTuple>;
    EXPECT_EQ(num_ports, 2);
}

TEST_F(DataInjectionGyroscopeNodeTest, IsDataInjectionSource) {
    auto* injection_source = dynamic_cast<graph::datasources::IDataInjectionSource*>(node_.get());
    EXPECT_NE(injection_source, nullptr);
}

// ============================================================================
// DataInjectionMagnetometerNode Tests
// ============================================================================

class DataInjectionMagnetometerNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<sensors::DataInjectionMagnetometerNode> node_;

    void SetUp() override {
        node_ = std::make_unique<sensors::DataInjectionMagnetometerNode>();
        node_->SetName("mag_node");
    }
};

TEST_F(DataInjectionMagnetometerNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "mag_node");
}

TEST_F(DataInjectionMagnetometerNodeTest, HasDualPorts) {
    EXPECT_STREQ(node_->kDataPort, "Mag");
    EXPECT_STREQ(node_->kNotifyPort, "Notify");
}

TEST_F(DataInjectionMagnetometerNodeTest, PortMetadata) {
    using PortsTuple = sensors::DataInjectionMagnetometerNode::Ports;
    constexpr size_t num_ports = std::tuple_size_v<PortsTuple>;
    EXPECT_EQ(num_ports, 2);
}

TEST_F(DataInjectionMagnetometerNodeTest, IsDataInjectionSource) {
    auto* injection_source = dynamic_cast<graph::datasources::IDataInjectionSource*>(node_.get());
    EXPECT_NE(injection_source, nullptr);
}

// ============================================================================
// DataInjectionBarometricNode Tests
// ============================================================================

class DataInjectionBarometricNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<sensors::DataInjectionBarometricNode> node_;

    void SetUp() override {
        node_ = std::make_unique<sensors::DataInjectionBarometricNode>();
        node_->SetName("baro_node");
    }
};

TEST_F(DataInjectionBarometricNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "baro_node");
}

TEST_F(DataInjectionBarometricNodeTest, HasDualPorts) {
    EXPECT_STREQ(node_->kDataPort, "Baro");
    EXPECT_STREQ(node_->kNotifyPort, "Notify");
}

TEST_F(DataInjectionBarometricNodeTest, PortMetadata) {
    using PortsTuple = sensors::DataInjectionBarometricNode::Ports;
    constexpr size_t num_ports = std::tuple_size_v<PortsTuple>;
    EXPECT_EQ(num_ports, 2);
}

TEST_F(DataInjectionBarometricNodeTest, IsDataInjectionSource) {
    auto* injection_source = dynamic_cast<graph::datasources::IDataInjectionSource*>(node_.get());
    EXPECT_NE(injection_source, nullptr);
}

// ============================================================================
// DataInjectionGPSPositionNode Tests
// ============================================================================

class DataInjectionGPSPositionNodeTest : public ::testing::Test {
protected:
    std::unique_ptr<sensors::DataInjectionGPSPositionNode> node_;

    void SetUp() override {
        node_ = std::make_unique<sensors::DataInjectionGPSPositionNode>();
        node_->SetName("gps_node");
    }
};

TEST_F(DataInjectionGPSPositionNodeTest, NodeCreation) {
    ASSERT_NE(node_, nullptr);
    EXPECT_EQ(node_->GetName(), "gps_node");
}

TEST_F(DataInjectionGPSPositionNodeTest, HasDualPorts) {
    EXPECT_STREQ(node_->kDataPort, "GPS");
    EXPECT_STREQ(node_->kNotifyPort, "Notify");
}

TEST_F(DataInjectionGPSPositionNodeTest, PortMetadata) {
    using PortsTuple = sensors::DataInjectionGPSPositionNode::Ports;
    constexpr size_t num_ports = std::tuple_size_v<PortsTuple>;
    EXPECT_EQ(num_ports, 2);
}

TEST_F(DataInjectionGPSPositionNodeTest, IsDataInjectionSource) {
    auto* injection_source = dynamic_cast<graph::datasources::IDataInjectionSource*>(node_.get());
    EXPECT_NE(injection_source, nullptr);
}

// ============================================================================
// Cross-Cutting Tests: All Sensor Types
// ============================================================================

class DataInjectionNodesCommonTest : public ::testing::Test {
};

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesImplementIDataInjectionSource) {
    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(accel.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(gyro.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(mag.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(baro.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(gps.get()), nullptr);
}

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesAreConfigurable) {
    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(accel.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(gyro.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(mag.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(baro.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::IConfigurable*>(gps.get()), nullptr);
}

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesHaveDualPortOutput) {
    // Each node should have:
    // - Port 0: Data output
    // - Port 1: Notification/Completion signal output

    auto test_node = [](const auto& node_name) {
        SCOPED_TRACE(node_name);
        // All nodes have kDataPort and kNotifyPort defined
        EXPECT_STRNE(node_name->kDataPort, "");
        EXPECT_STRNE(node_name->kNotifyPort, "");
        // Verify port names are different
        EXPECT_STRNE(node_name->kDataPort, node_name->kNotifyPort);
    };

    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    test_node(accel);
    test_node(gyro);
    test_node(mag);
    test_node(baro);
    test_node(gps);
}

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesHaveCorrectClassification) {
    // Each node should have its own sensor classification type
    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    // Verify they can be cast to IDataInjectionSource and have correct classification
    auto accel_src = dynamic_cast<graph::datasources::IDataInjectionSource*>(accel.get());
    auto gyro_src = dynamic_cast<graph::datasources::IDataInjectionSource*>(gyro.get());
    auto mag_src = dynamic_cast<graph::datasources::IDataInjectionSource*>(mag.get());
    auto baro_src = dynamic_cast<graph::datasources::IDataInjectionSource*>(baro.get());
    auto gps_src = dynamic_cast<graph::datasources::IDataInjectionSource*>(gps.get());

    ASSERT_NE(accel_src, nullptr);
    ASSERT_NE(gyro_src, nullptr);
    ASSERT_NE(mag_src, nullptr);
    ASSERT_NE(baro_src, nullptr);
    ASSERT_NE(gps_src, nullptr);

    // Each should have a valid classification type
    EXPECT_EQ(accel_src->sensor_classification_type, sensors::SensorClassificationType::ACCELEROMETER);
    EXPECT_EQ(gyro_src->sensor_classification_type, sensors::SensorClassificationType::GYROSCOPE);
    EXPECT_EQ(mag_src->sensor_classification_type, sensors::SensorClassificationType::MAGNETOMETER);
    EXPECT_EQ(baro_src->sensor_classification_type, sensors::SensorClassificationType::BAROMETRIC);
    EXPECT_EQ(gps_src->sensor_classification_type, sensors::SensorClassificationType::GPS_POSITION);
}

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesCanBeNamed) {
    auto test_naming = [](auto& node, const std::string& name) {
        SCOPED_TRACE(name);
        node->SetName(name);
        EXPECT_EQ(node->GetName(), name);
    };

    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    test_naming(accel, "csv_accel");
    test_naming(gyro, "csv_gyro");
    test_naming(mag, "csv_mag");
    test_naming(baro, "csv_baro");
    test_naming(gps, "csv_gps");
}

TEST_F(DataInjectionNodesCommonTest, AllFiveSensorTypesHaveUniquePortNames) {
    // Verify each sensor has unique port names (no confusion between sensors)
    auto accel = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto gyro = std::make_unique<sensors::DataInjectionGyroscopeNode>();
    auto mag = std::make_unique<sensors::DataInjectionMagnetometerNode>();
    auto baro = std::make_unique<sensors::DataInjectionBarometricNode>();
    auto gps = std::make_unique<sensors::DataInjectionGPSPositionNode>();

    // All should have "Notify" for completion signal port
    EXPECT_STREQ(accel->kNotifyPort, "Notify");
    EXPECT_STREQ(gyro->kNotifyPort, "Notify");
    EXPECT_STREQ(mag->kNotifyPort, "Notify");
    EXPECT_STREQ(baro->kNotifyPort, "Notify");
    EXPECT_STREQ(gps->kNotifyPort, "Notify");

    // Each should have unique data port name
    EXPECT_STRNE(accel->kDataPort, gyro->kDataPort);
    EXPECT_STRNE(accel->kDataPort, mag->kDataPort);
    EXPECT_STRNE(accel->kDataPort, baro->kDataPort);
    EXPECT_STRNE(accel->kDataPort, gps->kDataPort);
    EXPECT_STRNE(gyro->kDataPort, mag->kDataPort);
    EXPECT_STRNE(gyro->kDataPort, baro->kDataPort);
    EXPECT_STRNE(gyro->kDataPort, gps->kDataPort);
    EXPECT_STRNE(mag->kDataPort, baro->kDataPort);
    EXPECT_STRNE(mag->kDataPort, gps->kDataPort);
    EXPECT_STRNE(baro->kDataPort, gps->kDataPort);
}

TEST_F(DataInjectionNodesCommonTest, MultipleInstancesOfSameSensorTypeCanCoexist) {
    // Verify we can create multiple instances without conflicts
    auto accel1 = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto accel2 = std::make_unique<sensors::DataInjectionAccelerometerNode>();
    auto accel3 = std::make_unique<sensors::DataInjectionAccelerometerNode>();

    accel1->SetName("accel_primary");
    accel2->SetName("accel_secondary");
    accel3->SetName("accel_tertiary");

    EXPECT_EQ(accel1->GetName(), "accel_primary");
    EXPECT_EQ(accel2->GetName(), "accel_secondary");
    EXPECT_EQ(accel3->GetName(), "accel_tertiary");

    // All should maintain their interfaces
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(accel1.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(accel2.get()), nullptr);
    EXPECT_NE(dynamic_cast<graph::datasources::IDataInjectionSource*>(accel3.get()), nullptr);
}
