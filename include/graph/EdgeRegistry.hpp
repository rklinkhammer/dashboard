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
 * @file EdgeRegistry.hpp
 * @brief Type-aware edge creation registry
 *
 * Enables runtime creation of templated Edge<SrcNode, SrcPort, DstNode, DstPort>
 * by dispatching to compile-time registered creator functions.
 *
 * Key Pattern:
 * - At program startup: Register edge creators for each (SrcType, SrcPort, DstType, DstPort)
 * - At JSON load time: Lookup and dispatch based on (src_type_name, src_port, dst_type_name, dst_port)
 *
 * Note: The registry takes the GraphManager reference and node indices, not INode*,
 * because AddEdge is a template method that needs compile-time types.
 *
 * @author Copilot
 * @date 2026-01-04
 */

#pragma once

#include <map>
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

// GraphManager is declared in the graph namespace
namespace graph {
class GraphManager;
}

namespace graph::config {

// Use alias to make GraphManager refer to graph::GraphManager
using GraphManager = ::graph::GraphManager;

/**
 * @class EdgeRegistry
 * @brief Centralized registry for type-aware edge creation
 *
 * Solves the problem: How to create Edge<SrcType, SrcPort, DstType, DstPort>
 * when we only know type names and port numbers at runtime?
 *
 * Solution: Register creator lambdas at startup that capture template types,
 * then dispatch at runtime by (type_name, port_index).
 *
 * Usage:
 * @code
 * // At program startup
 * void RegisterEdges() {
 *   EdgeRegistry::Register<DataInjectionAccelerometerNode, 0, FlightFSMNode, 0>(
 *       [](GraphManager& g, size_t src_idx, size_t dst_idx) {
 *           return g.AddEdge<DataInjectionAccelerometerNode, 0, FlightFSMNode, 0>(
 *               std::dynamic_pointer_cast<DataInjectionAccelerometerNode>(g.GetNode(src_idx)),
 *               std::dynamic_pointer_cast<avionics::FlightFSMNode>(g.GetNode(dst_idx)));
 *       });
 * }
 *
 * // At JSON load time
 * EdgeRegistry::CreateEdge(
 *   graph, "DataInjectionAccelerometerNode", 0, "FlightFSMNode", 0, src_idx, dst_idx);
 * @endcode
 */
class EdgeRegistry {
public:
    /**
     * Creator function type: (GraphManager&, src_node_idx, dst_node_idx) -> bool
     * 
     * The creator lambda is responsible for:
     * 1. Getting the nodes by index from GraphManager
     * 2. Casting to the appropriate node types
     * 3. Calling GraphManager::AddEdge<SrcNode, Port, DstNode, Port>()
     * 4. Returning true if successful
     */
    using EdgeCreator = std::function<bool(GraphManager&, std::size_t, std::size_t)>;
    
    /**
     * Register a type-aware edge creator
     *
     * @tparam SrcNode Source node type
     * @tparam SrcPort Source port index
     * @tparam DstNode Destination node type
     * @tparam DstPort Destination port index
     * 
     * @param src_type_name Human-readable type name for source node
     * @param dst_type_name Human-readable type name for destination node
     * @param creator Lambda that instantiates Edge<SrcNode, SrcPort, DstNode, DstPort>
     *
     * Typically called once per edge type combination at program startup.
     *
     * Example:
     * @code
     * using CSVAccel = nodes::DataInjectionAccelerometerNode;
     * using FlightFSM = nodes::FlightFSMNode;
     * using Message = sensors::SensorData;
     * 
     * EdgeRegistry::Register<CSVAccel, 0, FlightFSM, 0>(
     *     "DataInjectionAccelerometerNode", "FlightFSMNode",
     *     [](GraphManager& g, size_t src_idx, size_t dst_idx) {
     *         auto src = std::dynamic_pointer_cast<CSVAccel>(g.GetNode(src_idx));
     *         auto dst = std::dynamic_pointer_cast<FlightFSM>(g.GetNode(dst_idx));
     *         if (!src || !dst) return false;
     *         g.AddEdge<CSVAccel, 0, FlightFSM, 0>(src, dst);
     *         return true;
     *     });
     * @endcode
     */
    template <typename SrcNode, std::size_t SrcPort, typename DstNode, std::size_t DstPort>
    static void Register(
        const std::string& src_type_name,
        const std::string& dst_type_name,
        EdgeCreator creator) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = MakeKey(src_type_name, SrcPort, dst_type_name, DstPort);
        
        if (GetRegistry().count(key) > 0) {
            throw std::runtime_error("Edge creator already registered for: " + key);
        }
        
        GetRegistry()[key] = creator;
    }
    
    /**
     * Create an edge using a registered creator
     *
     * @param graph GraphManager instance to add edge to
     * @param src_node_type Type name of source node
     * @param src_port_idx Source port index
     * @param dst_node_type Type name of destination node
     * @param dst_port_idx Destination port index
     * @param src_node_idx Index in graph.GetNodes()
     * @param dst_node_idx Index in graph.GetNodes()
     * @return true if edge was created successfully, false otherwise
     *
     * @throws std::runtime_error if no creator is registered for this combination
     */
    static bool CreateEdge(
        GraphManager& graph,
        const std::string& src_node_type,
        std::size_t src_port_idx,
        const std::string& dst_node_type,
        std::size_t dst_port_idx,
        std::size_t src_node_idx,
        std::size_t dst_node_idx);
    
    /**
     * Check if an edge creator is registered
     *
     * @param src_node_type Type name of source node
     * @param src_port_idx Source port index
     * @param dst_node_type Type name of destination node
     * @param dst_port_idx Destination port index
     * @return true if a creator is registered for this combination
     */
    static bool IsRegistered(
        const std::string& src_node_type,
        std::size_t src_port_idx,
        const std::string& dst_node_type,
        std::size_t dst_port_idx);
    
    /**
     * Clear all registered edge creators
     */
    static void Clear();
    
    /**
     * Get number of registered edge creators
     */
    static size_t GetRegisteredCount();
    
    /**
     * Get list of all registered edge type combinations (for debugging)
     */
    static std::vector<std::string> GetRegistered();
    
private:
    /// Singleton registry map
    static std::map<std::string, EdgeCreator>& GetRegistry();
    
    /// Generate unique key for an edge type combination
    static std::string MakeKey(
        const std::string& src_node_type,
        std::size_t src_port_idx,
        const std::string& dst_node_type,
        std::size_t dst_port_idx);
    
    /// Global mutex for thread-safe registry access
    static std::mutex mutex_;
};

}  // namespace graph::config

