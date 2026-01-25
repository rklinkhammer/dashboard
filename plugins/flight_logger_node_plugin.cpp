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
 * @file flight_logger_node_plugin.cpp
 * @brief FlightLoggerNode as a dynamically-loadable plugin
 *
 * This file demonstrates how to expose FlightLoggerNode (a sink node)
 * as a dynamically-loadable plugin using the NodeFacade interface.
 *
 * Similar pattern to sensor_node_plugin.cpp but for a data consumer.
 *
 * Compilation (from workspace root):
 *   mkdir -p build/plugins
 *   cd build
 *   cmake ..
 *   make flight_logger_node
 *
 * This produces: build/plugins/libflight_logger_node.so
 */

#include "plugins/NodePluginTemplate.hpp"
#include "avionics/nodes/FlightLoggerNode.hpp"

using namespace graph;
using avionics::FlightLoggerNode;

// ============================================================================
// Policy specialization
// ============================================================================

struct FlightLoggerPolicy : PluginPolicy<FlightLoggerNode> {
    static constexpr const char* Description =
        "Sink node that logs incoming message streams (plugin version)";

    static bool SetProperty(NodePluginInstance<FlightLoggerNode>* inst,
                            const char*, const char*) {
        LOG4CXX_TRACE(inst->logger, "No properties supported");
        return true;
    }
    
    static const char* GetProperty(NodePluginInstance<FlightLoggerNode>* inst,
                                  const char*) {
        LOG4CXX_TRACE(inst->logger, "No properties supported");
        return "";
    }
};

// ============================================================================
// Facade
// ============================================================================

using Glue = PluginGlue<FlightLoggerNode, FlightLoggerPolicy>;
static const graph::NodeFacade flight_logger_node_facade = Glue::MakeFacade();

// ============================================================================
// C exports
// ============================================================================

extern "C" {

void* plugin_create_flight_logger_node() {
    try {
        auto node = std::make_shared<FlightLoggerNode>();
        return new NodePluginInstance<FlightLoggerNode>(
            node, "FlightLoggerNode", "plugin.FlightLoggerNode");
    } catch (...) {
        return nullptr;
    }
}

const char* plugin_get_info() {
    return "FlightLoggerNode|Sink node consuming flight data|1.0|"
           "plugin_create_flight_logger_node|"
#ifdef _LIBCPP_VERSION
           "libc++_v1";
#else
           "libstdc++_v1";
#endif
}

graph::NodeFacade* plugin_get_facade() {
    return const_cast<graph::NodeFacade*>(&flight_logger_node_facade);
}

}

