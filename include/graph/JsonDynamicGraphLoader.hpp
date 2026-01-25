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
 * @file JsonDynamicGraphLoader.hpp
 * @brief Load JSON graph configurations as NodeFacadeAdapter collections
 *
 * JsonDynamicGraphLoader replaces the old JsonGraphLoader with a cleaner,
 * simpler design that works exclusively with NodeFacadeAdapter instances.
 *
 * Key features:
 * - Assumes ALL nodes are dynamically loaded from plugins
 * - No integration with INode or compile-time type system
 * - Returns raw NodeFacadeAdapter instances for manual graph assembly
 * - Proven interface used by rocket_telemetry.cpp
 *
 * Usage:
 * @code
 * auto factory = std::make_shared<NodeFactory>(registry);
 * auto nodes = JsonDynamicGraphLoader::LoadNodes("config.json", factory);
 * auto edges = JsonDynamicGraphLoader::LoadEdges("config.json");
 * 
 * // Initialize and start nodes...
 * for (auto& node : nodes) {
 *     node.Init();
 *     node.Start();
 * }
 * @endcode
 *
 * @see graph::NodeFacadeAdapter
 * @see graph::NodeFactory
 * @see graph::config::GraphConfig
 *
 * @author GitHub Copilot
 * @date 2026-01-04
 */

#pragma once

#include "GraphConfig.hpp"
#include "graph/NodeFacade.hpp"
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace graph {
    // Forward declaration
    class NodeFactory;
}

namespace graph::config {

/**
 * @class JsonDynamicGraphLoader
 * @brief Loads JSON graph configurations as collections of NodeFacadeAdapter instances
 *
 * Replaces the old JsonGraphLoader. This loader works exclusively with the
 * plugin system and returns NodeFacadeAdapter instances. All nodes in the
 * configuration must be available via the plugin system.
 *
 * Key differences from JsonGraphLoader:
 * - No INode integration (completely removed)
 * - No compile-time typed node support
 * - Simpler API (just load nodes and edges)
 * - Manual edge connection (not automatic)
 * - Proven pattern from rocket_telemetry.cpp
 */
class JsonDynamicGraphLoader {
public:
    /**
     * Load all nodes from JSON configuration
     *
     * Parses the JSON configuration file and creates NodeFacadeAdapter instances
     * for each node using the provided factory.
     *
     * @param filepath Path to JSON configuration file
     * @param factory NodeFactory for creating plugin nodes
     * @return Vector of NodeFacadeAdapter instances (movable)
     *
     * @throws std::runtime_error if:
     *   - File cannot be read
     *   - JSON is malformed
     *   - Configuration validation fails
     *   - Node type is not found in plugin system
     *   - Node creation fails
     *
     * @note Node names are set from config (uses id if name is empty)
     *
     * Example:
     * @code
     * auto nodes = JsonDynamicGraphLoader::LoadNodes("config.json", factory);
     * // Returns vector with one NodeFacadeAdapter per node in config
     * @endcode
     */
    static std::vector<std::shared_ptr<NodeFacadeAdapter>> LoadNodes(
        const std::string& filepath,
        std::shared_ptr<NodeFactory> factory);

    /**
     * Load edge specifications from JSON configuration
     *
     * Parses the JSON configuration file and extracts edge definitions.
     * Does not create actual connections - returns metadata only.
     * Caller must manually connect nodes using the returned EdgeConfig data.
     *
     * @param filepath Path to JSON configuration file
     * @return Vector of EdgeConfig (metadata, no actual connections)
     *
     * @throws std::runtime_error if:
     *   - File cannot be read
     *   - JSON is malformed
     *   - Configuration validation fails
     *
     * Example:
     * @code
     * auto edges = JsonDynamicGraphLoader::LoadEdges("config.json");
     * for (const auto& edge : edges) {
     *     // Connect nodes manually using edge metadata
     *     nodes[edge.source_node_id].Connect(
     *         nodes[edge.target_node_id],
     *         edge.source_port, edge.target_port);
     * }
     * @endcode
     */
    static std::vector<EdgeConfig> LoadEdges(
        const std::string& filepath);

    /**
     * Load and validate complete graph configuration
     *
     * Convenience method that loads both nodes and edges in one call.
     * Equivalent to calling LoadNodes() then LoadEdges().
     *
     * @param filepath Path to JSON configuration file
     * @param factory NodeFactory for creating plugin nodes
     * @return Pair of (nodes, edges)
     *
     * @throws std::runtime_error if loading fails (see LoadNodes/LoadEdges)
     *
     * Example:
     * @code
     * auto [nodes, edges] = JsonDynamicGraphLoader::LoadGraph(
     *     "config.json", factory);
     * @endcode
     */
    static std::pair<std::vector<std::shared_ptr<NodeFacadeAdapter>>, std::vector<EdgeConfig>>
    LoadGraph(
        const std::string& filepath,
        std::shared_ptr<NodeFactory> factory);

private:
    /**
     * Parse JSON file and return GraphConfig
     *
     * @param filepath Path to JSON file
     * @return Parsed GraphConfig
     * @throws std::runtime_error on parse failure
     */
    static GraphConfig ParseConfigFile(const std::string& filepath);

    /**
     * Apply node-specific configuration from JSON
     *
     * @param adapter NodeFacadeAdapter to configure
     * @param config_json JSON object with node configuration
     */
    static void ApplyNodeConfiguration(
        std::shared_ptr<NodeFacadeAdapter> adapter,
        const nlohmann::json& config_json);
};

}  // namespace graph::config

