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
 * @file completion_aggregator_node_plugin.cpp
 * @brief CompletionAggregatorNode as a dynamically-loadable plugin
 *
 * This file demonstrates how to expose CompletionAggregatorNode (completion signal aggregator)
 * as a dynamically-loadable plugin using the NodeFacade interface.
 *
 * Similar pattern to other CSV sensor plugins.
 *
 * Compilation (from workspace root):
 *   mkdir -p build/plugins
 *   cd build
 *   cmake ..
 *   make completion_aggregator_node
 *
 * This produces: build/plugins/libcompletion_aggregator_node.so
 */

#include <memory>
#include <string>
#include <span>
#include <log4cxx/logger.h>
#include "plugins/NodePluginTemplate.hpp"
#include "graph/CompletionAggregatorNode.hpp"


using namespace graph;
using graph::CompletionAggregatorNode;

// ============================================================================
// Policy specialization
// ============================================================================

struct CompletionAggregatorPolicy : PluginPolicy<CompletionAggregatorNode> {
    static constexpr const char* Description =
        "Aggregator node merging completion signals from 5 CSV sensors (plugin version)";

    static bool SetProperty(NodePluginInstance<CompletionAggregatorNode>* inst,
                            const char*, const char*) {
        LOG4CXX_TRACE(inst->logger, "No properties supported");
        return true;
    }
};

// ============================================================================
// Facade
// ============================================================================

using Glue = PluginGlue<CompletionAggregatorNode, CompletionAggregatorPolicy>;
static const NodeFacade completion_aggregator_node_facade = Glue::MakeFacade();

// ============================================================================
// C exports
// ============================================================================

extern "C" {

void* plugin_create_completion_aggregator_node() {
    try {
        auto node = std::make_shared<CompletionAggregatorNode>();
        return new NodePluginInstance<CompletionAggregatorNode>(
            node, "CompletionAggregatorNode", "plugin.CompletionAggregatorNode");
    } catch (...) {
        return nullptr;
    }
}

const char* plugin_get_info() {
    return "CompletionAggregatorNode|Sink node|1.0|"
           "plugin_create_completion_aggregator_node|"
#ifdef _LIBCPP_VERSION
           "libc++_v1";
#else
           "libstdc++_v1";
#endif
}

NodeFacade* plugin_get_facade() {
    return const_cast<NodeFacade*>(&completion_aggregator_node_facade);
}

}

