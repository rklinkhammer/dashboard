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

#include "graph/EdgeRegistry.hpp"
#include "graph/GraphManager.hpp"

#include <sstream>

namespace graph::config {

// Initialize static members
std::mutex EdgeRegistry::mutex_;

// Get singleton registry
std::map<std::string, EdgeRegistry::EdgeCreator>& EdgeRegistry::GetRegistry() {
    static std::map<std::string, EdgeRegistry::EdgeCreator> registry;
    return registry;
}

// Generate unique key for edge type combination
std::string EdgeRegistry::MakeKey(
    const std::string& src_node_type,
    std::size_t src_port_idx,
    const std::string& dst_node_type,
    std::size_t dst_port_idx) {
    std::ostringstream oss;
    oss << src_node_type << "::" << src_port_idx
        << " -> " << dst_node_type << "::" << dst_port_idx;
    return oss.str();
}

// Check if an edge creator is registered
bool EdgeRegistry::IsRegistered(
    const std::string& src_node_type,
    std::size_t src_port_idx,
    const std::string& dst_node_type,
    std::size_t dst_port_idx) {
    
    std::lock_guard<std::mutex> lock(EdgeRegistry::mutex_);
    
    std::string key = EdgeRegistry::MakeKey(src_node_type, src_port_idx, dst_node_type, dst_port_idx);
    return EdgeRegistry::GetRegistry().count(key) > 0;
}

// Create an edge using registered creator
bool EdgeRegistry::CreateEdge(
    GraphManager& graph,
    const std::string& src_node_type,
    std::size_t src_port_idx,
    const std::string& dst_node_type,
    std::size_t dst_port_idx,
    std::size_t src_node_idx,
    std::size_t dst_node_idx) {
    
    std::lock_guard<std::mutex> lock(EdgeRegistry::mutex_);
    
    std::string key = EdgeRegistry::MakeKey(src_node_type, src_port_idx, dst_node_type, dst_port_idx);
    
    auto it = EdgeRegistry::GetRegistry().find(key);
    if (it == EdgeRegistry::GetRegistry().end()) {
        std::ostringstream oss;
        oss << "No edge creator registered for: " << key
            << " (registered types: ";
        
        bool first = true;
        for (const auto& registered_key : EdgeRegistry::GetRegistry()) {
            if (!first) oss << ", ";
            oss << registered_key.first;
            first = false;
        }
        oss << ")";
        
        throw std::runtime_error(oss.str());
    }
    
    // Call the registered creator with node indices
    return it->second(graph, src_node_idx, dst_node_idx);
}

// Clear all registered edge creators
void EdgeRegistry::Clear() {
    std::lock_guard<std::mutex> lock(EdgeRegistry::mutex_);
    EdgeRegistry::GetRegistry().clear();
}

// Get number of registered edge creators
size_t EdgeRegistry::GetRegisteredCount() {
    std::lock_guard<std::mutex> lock(EdgeRegistry::mutex_);
    return EdgeRegistry::GetRegistry().size();
}

// Get list of all registered edge type combinations
std::vector<std::string> EdgeRegistry::GetRegistered() {
    std::lock_guard<std::mutex> lock(EdgeRegistry::mutex_);
    
    std::vector<std::string> result;
    for (const auto& pair : EdgeRegistry::GetRegistry()) {
        result.push_back(pair.first);
    }
    return result;
}

}  // namespace graph::config
