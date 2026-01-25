// MIT License
//
// Copyright (c) 2025 graphlib contributors
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

/**
 * @file sensor/DataInjectionAccelerometerNode.hpp
 * @brief CSV-based accelerometer data producer node
 *
 * Implements a data producer node that reads accelerometer data from CSV
 * files and generates timestamped AccelerometerData messages. Demonstrates
 * the DataInjectionProducerWithNotification pattern for sensor simulation.
 *
 * @author GraphX Contributors
 * @date 2025
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <memory>

#include "graph/DataInjectionProducerWithNotification.hpp"
#include "graph/Message.hpp"
#include "sensor/SensorDataTypes.hpp"

namespace sensors {

/**
 * @class DataInjectionAccelerometerNode
 * @brief Accelerometer data producer from CSV file
 *
 * Reads enqueue (injected)  accelerometer data and produces timestamped
 * AccelerometerData messages at configurable sample intervals.
 * Supports initial sample skipping for data alignment.
 */
class DataInjectionBarometricNode : public graph::DataInjectionProducerWithNotification<
    DataInjectionBarometricNode,
    sensors::BarometricData,
    sensors::SensorPayload,
    sensors::SensorClassificationType::BAROMETRIC> {
public:
    /**
     * @brief Construct a DataInjectionBarometricNode
     * 
     * Uses default configuration:
     * - sample_interval: 10ms (100 Hz)
     * - sample_ignore: 0 (all samples)
     * 
     * Configuration can be changed via Configure() method.
     */
    DataInjectionBarometricNode()
        : graph::DataInjectionProducerWithNotification<
            DataInjectionBarometricNode,
            sensors::BarometricData,
            sensors::SensorPayload, 
            sensors::SensorClassificationType::BAROMETRIC>() {
        SetName("__unnamed__");
        
        // Initialize IDataInjectionSource members for discovery and data injection
        sensor_classification_type = GetSensorClassificationType();
    }

    virtual ~DataInjectionBarometricNode() = default;

    static constexpr char kDataPort[] = "Baro";
    static constexpr char kNotifyPort[]  = "Notify";
   
    using Ports = std::tuple<
            graph::PortSpec<0, ::graph::message::Message, graph::PortDirection::Output, kDataPort,
                graph::PayloadList<sensors::BarometricData>>,
            graph::PortSpec<1, ::graph::message::CompletionSignal, graph::PortDirection::Output, kNotifyPort,
                graph::PayloadList<>>
            >;
    };

} // namespace sensors

