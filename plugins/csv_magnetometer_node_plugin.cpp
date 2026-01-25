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
 * @file csv_magnetometer_node_plugin.cpp
 * @brief DataInjectionMagnetometerNode as a dynamically-loadable plugin
 *
 * This file demonstrates how to expose DataInjectionMagnetometerNode (magnetometer data source)
 * as a dynamically-loadable plugin using the NodeFacade interface.
 *
 * Compilation (from workspace root):
 *   mkdir -p build/plugins
 *   cd build
 *   cmake ..
 *   make csv_magnetometer_node
 *
 * This produces: build/plugins/libcsv_magnetometer_node.so
 */

#include <memory>
#include <string>
#include <span>
#include <log4cxx/logger.h>
#include "plugins/NodePluginTemplate.hpp"
#include "sensor/DataInjectionMagnetometerNode.hpp"


using namespace graph;
using sensors::DataInjectionMagnetometerNode;
// ============================================================================
// Policy specialization
// ============================================================================

struct CSVMagnetometerPolicy : PluginPolicy<DataInjectionMagnetometerNode> {
    static constexpr const char* Description =
        "Source node streaming magnetometer data from CSV file (plugin version)";

    static bool SetProperty(NodePluginInstance<DataInjectionMagnetometerNode>* inst,
                            const char*, const char*) {
        LOG4CXX_TRACE(inst->logger, "No properties supported");
        return true;
    }
};

// ============================================================================
// Facade
// ============================================================================

using Glue = PluginGlue<DataInjectionMagnetometerNode, CSVMagnetometerPolicy>;
static const NodeFacade csv_magnetometer_node_facade = Glue::MakeFacade();

// ============================================================================
// C exports
// ============================================================================

extern "C" {

void* plugin_create_csv_magnetometer_node() {
    try {
        auto node = std::make_shared<DataInjectionMagnetometerNode>();
        return new NodePluginInstance<DataInjectionMagnetometerNode>(
            node, "DataInjectionMagnetometerNode", "plugin.DataInjectionMagnetometerNode");
    } catch (...) {
        return nullptr;
    }
}

const char* plugin_get_info() {
    return "DataInjectionMagnetometerNode|Source node streaming magnetometer data|1.0|"
           "plugin_create_csv_magnetometer_node|"
#ifdef _LIBCPP_VERSION
           "libc++_v1";
#else
           "libstdc++_v1";
#endif
}

NodeFacade* plugin_get_facade() {
    return const_cast<NodeFacade*>(&csv_magnetometer_node_facade);
}

}

