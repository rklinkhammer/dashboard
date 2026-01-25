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
 * @file feedback_test_sink_node_plugin.cpp
 * @brief FeedbackTestSinkNode as a dynamically-loadable plugin
 *
 * This file demonstrates how to expose FeedbackTestSinkNode (feedback message sink for testing)
 * as a dynamically-loadable plugin using the NodeFacade interface.
 *
 * Similar pattern to other sink node plugins.
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
#include <log4cxx/logger.h>
#include "plugins/NodePluginTemplate.hpp"
#include "avionics/nodes/FeedbackTestSinkNode.hpp"
#include "avionics/messages/AvionicsMessages.hpp"


using namespace graph;
using avionics::FeedbackTestSinkNode;
// ============================================================================
// Policy specialization
// ============================================================================

struct FeedbackTestSinkPolicy : PluginPolicy<FeedbackTestSinkNode> {
    static constexpr const char* Description =
        "Sink node receiving feedback messages from estimation pipeline (plugin version)";

    static bool SetProperty(NodePluginInstance<FeedbackTestSinkNode>* inst,
                            const char*, const char*) {
        LOG4CXX_TRACE(inst->logger, "No properties supported");
        return true;
    }
};

// ============================================================================
// Facade
// ============================================================================

using Glue = PluginGlue<FeedbackTestSinkNode, FeedbackTestSinkPolicy>;
static const NodeFacade feedback_test_sink_node_facade = Glue::MakeFacade();

// ============================================================================
// C exports
// ============================================================================

extern "C" {

void* plugin_create_feedback_test_sink_node() {
    try {
        auto node = std::make_shared<FeedbackTestSinkNode>();
        return new NodePluginInstance<FeedbackTestSinkNode>(
            node, "FeedbackTestSinkNode", "plugin.FeedbackTestSinkNode");
    } catch (...) {
        return nullptr;
    }
}

const char* plugin_get_info() {
    return "FeedbackTestSinkNode|Sink node receiving feedback messages from estimation pipeline|1.0|"
           "plugin_create_feedback_test_sink_node|"
#ifdef _LIBCPP_VERSION
           "libc++_v1";
#else
           "libstdc++_v1";
#endif
}

NodeFacade* plugin_get_facade() {
    return const_cast<NodeFacade*>(&feedback_test_sink_node_facade);
}

}

