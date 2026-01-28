// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
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
 * @file JsonDynamicGraphLoader.cpp
 * @brief Implementation of JSON graph loader for NodeFacadeAdapter-based graphs
 */

#include "graph/JsonDynamicGraphLoader.hpp"
#include "graph/GraphConfigParser.hpp"
#include "graph/NodeFactory.hpp"
#include <stdexcept>
#include <sstream>
#include <log4cxx/logger.h>

namespace graph::config {

static log4cxx::LoggerPtr logger_ = 
    log4cxx::Logger::getLogger("graph.config.JsonDynamicGraphLoader");

// ============================================================================
// LoadNodes Implementation
// ============================================================================

std::vector<std::shared_ptr<NodeFacadeAdapter>> JsonDynamicGraphLoader::LoadNodes(
    const std::string& filepath,
    std::shared_ptr<NodeFactory> factory) {
    
    if (!factory) {
        throw std::runtime_error("NodeFactory is null");
    }
    
    LOG4CXX_TRACE(logger_, "Loading nodes from JSON: " << filepath);
    LOG4CXX_TRACE(logger_, "About to parse config file");
    
    // Parse the JSON configuration file
    GraphConfig config = ParseConfigFile(filepath);
    LOG4CXX_TRACE(logger_, "Config parsed, about to validate");
    
    // Validate the configuration
    ValidationResult validation = GraphConfigParser::Validate(config);
    LOG4CXX_TRACE(logger_, "Config validated");
    if (!validation.valid) {
        std::ostringstream oss;
        oss << "Configuration validation failed: ";
        for (const auto& error : validation.errors) {
            oss << error << "; ";
        }
        LOG4CXX_ERROR(logger_, oss.str());
        throw std::runtime_error(oss.str());
    }
    
    LOG4CXX_TRACE(logger_, "About to create node vector with " << config.nodes.size() << " nodes");
    std::vector<std::shared_ptr<NodeFacadeAdapter>> nodes;
    nodes.reserve(config.nodes.size());
    LOG4CXX_TRACE(logger_, "Node vector created and reserved");
    
    // Create each node
    LOG4CXX_TRACE(logger_, "Starting loop to create " << config.nodes.size() << " nodes");
    for (size_t i = 0; i < config.nodes.size(); ++i) {
        LOG4CXX_TRACE(logger_, "Loop iteration " << i);
        const auto& node_config = config.nodes[i];
        LOG4CXX_TRACE(logger_, "Got node_config for: " << node_config.id);
        LOG4CXX_TRACE(logger_, "Node type is: " << node_config.type);
        
        // Create the node via factory
        // Note: Using CreateDynamicNode directly to avoid unified factory lambda overhead
        // The unified factory (via CreateNode) is available for explicit use when needed
        try {
            LOG4CXX_TRACE(logger_, "About to create node: " << node_config.id);
            
            // Use plugin factory path directly - more reliable than unified registry
            std::shared_ptr<NodeFacadeAdapter> adapter = 
                std::make_shared<NodeFacadeAdapter>(factory->CreateDynamicNode(node_config.type));
            LOG4CXX_TRACE(logger_, "CreateDynamicNode completed for: " << node_config.id);
            
            // Set the node's name (use name if provided, otherwise use id)
            std::string node_name = node_config.name.empty() ? 
                node_config.id : node_config.name;
            adapter->SetName(node_name);
            
            LOG4CXX_TRACE(logger_, "Created node: " << node_name 
                          << " (type: " << node_config.type << ")");
            

            // Apply configuration from JSON if present
            if (!node_config.node_config.empty()) {
                ApplyNodeConfiguration(adapter, node_config.node_config);
            }
            
            // Add to vector
            nodes.push_back(adapter);
            LOG4CXX_TRACE(logger_, "Added node to vector, count: " << nodes.size());
        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "Failed to create node '" << node_config.id 
                << "' (type: " << node_config.type << "): " << e.what();
            LOG4CXX_ERROR(logger_, oss.str());
            throw std::runtime_error(oss.str());
        }
    }
    
    LOG4CXX_TRACE(logger_, "Successfully loaded " << nodes.size() << " nodes");
    return nodes;
}

// ============================================================================
// LoadEdges Implementation
// ============================================================================

std::vector<EdgeConfig> JsonDynamicGraphLoader::LoadEdges(
    const std::string& filepath) {
    
    LOG4CXX_TRACE(logger_, "Loading edges from JSON: " << filepath);
    
    // Parse the JSON configuration file
    GraphConfig config = ParseConfigFile(filepath);
    
    // Validate the configuration
    ValidationResult validation = GraphConfigParser::Validate(config);
    if (!validation.valid) {
        std::ostringstream oss;
        oss << "Configuration validation failed: ";
        for (const auto& error : validation.errors) {
            oss << error << "; ";
        }
        LOG4CXX_ERROR(logger_, oss.str());
        throw std::runtime_error(oss.str());
    }
    
    LOG4CXX_TRACE(logger_, "Successfully loaded " << config.edges.size() 
                 << " edge specifications");
    return config.edges;
}

// ============================================================================
// LoadGraph Implementation
// ============================================================================

std::pair<std::vector<std::shared_ptr<NodeFacadeAdapter>>, std::vector<EdgeConfig>>
JsonDynamicGraphLoader::LoadGraph(
    const std::string& filepath,
    std::shared_ptr<NodeFactory> factory) {
    
    LOG4CXX_TRACE(logger_, "Loading complete graph from JSON: " << filepath);
    
    auto nodes = LoadNodes(filepath, factory);
    auto edges = LoadEdges(filepath);
    
    LOG4CXX_TRACE(logger_, "Graph loaded: " << nodes.size() << " nodes, " 
                 << edges.size() << " edges");
    
    return std::make_pair(nodes, std::move(edges));
}

// ============================================================================
// Helper: ParseConfigFile Implementation
// ============================================================================

GraphConfig JsonDynamicGraphLoader::ParseConfigFile(
    const std::string& filepath) {
    
    LOG4CXX_TRACE(logger_, "Parsing JSON file: " << filepath);
    
    try {
        return GraphConfigParser::ParseFile(filepath);
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Failed to parse configuration file '" << filepath 
            << "': " << e.what();
        LOG4CXX_ERROR(logger_, oss.str());
        throw std::runtime_error(oss.str());
    }
}

void JsonDynamicGraphLoader::ApplyNodeConfiguration(
    std::shared_ptr<NodeFacadeAdapter> adapter,
    const nlohmann::json& config_json) {
    
    if (config_json.empty()) {
        LOG4CXX_TRACE(logger_, "Configuration is empty, skipping");
        return;
    }
    
    try {
        // Convert JSON object to string
        std::string config_str = config_json.dump();
        
        // Apply the configuration using NodeFacadeAdapter's API
        bool success = adapter->SetConfigurationJSON(config_str);
        
        if (success) {
            LOG4CXX_TRACE(logger_, "Node configuration applied successfully");
        } else {
            LOG4CXX_TRACE(logger_, "Node does not support configuration (SetConfigurationJSON returned false)");
        }
    } catch (const std::exception& e) {
        LOG4CXX_WARN(logger_, "Failed to apply node configuration: " << e.what());
    }
}

}  // namespace graph::config
