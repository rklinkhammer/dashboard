#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include "graph/GraphConfig.hpp"
#include "graph/GraphManager.hpp"
#include "graph/NodeFactory.hpp"
#include "plugins/PluginLoader.hpp"
#include "plugins/PluginRegistry.hpp"
#include "ui/Dashboard.hpp"

/**
 * @file MetricsCapability.hpp
 * @brief Capability interface for metrics discovery and subscription
 *
 * Provides methods for discovering node metrics schemas and registering
 * callbacks to receive metrics events published during graph execution.
 */

namespace app::capabilities {

class GraphCapability {
public:
    virtual ~GraphCapability() = default;
    
    /// @brief Set the GraphManager instance
    /// @param graph_manager Shared pointer to GraphManager
    void SetGraphManager(std::shared_ptr<graph::GraphManager> gm) {
        graph_manager = gm;
    }

    /// @brief Get the GraphManager instance
    /// @return Shared pointer to GraphManager
    std::shared_ptr<graph::GraphManager> GetGraphManager() const {
        return graph_manager;
    }

    void SetPluginRegistry(std::shared_ptr<graph::PluginRegistry> registry) {
        plugin_registry = registry;
    }
   
    std::shared_ptr<graph::PluginRegistry> GetPluginRegistry() const {
        return plugin_registry;
    }
    
    void SetPluginLoader(std::shared_ptr<graph::PluginLoader> loader) {
        plugin_loader = loader;
    }
    
    std::shared_ptr<graph::PluginLoader> GetPluginLoader() const {
        return plugin_loader;
    }
    
    void SetNodeFactory(std::shared_ptr<graph::NodeFactory> nf) {
        node_factory = nf;
    }
    
    std::shared_ptr<graph::NodeFactory> GetNodeFactory() const {
        return node_factory;
    }

    /// @brief Set the node names for the graph
    /// @param names Vector of node names
    void SetNodeNames(const std::vector<std::string>& names) {
        node_names = names;
    }

    /// @brief Get the node names from the graph
    /// @return Const reference to vector of node names
    const std::vector<std::string>& GetNodeNames() const {
        return node_names;
    }

    /// @brief Set the edge descriptions for the graph
    /// @param descriptions Vector of edge description strings in format "source:port → dest:port"
    void SetEdgeDescriptions(const std::vector<std::string>& descriptions) {
        edge_descriptions = descriptions;
    }

    /// @brief Get the edge descriptions from the graph
    /// @return Const reference to vector of edge descriptions
    const std::vector<std::string>& GetEdgeDescriptions() const {
        return edge_descriptions;
    }

    std::string GetJsonConfigPath() const {
        return json_config_path;
    }   
    
    void SetJsonConfigPath(const std::string& path) {
        json_config_path = path;
    }

    void SetWindowHeights(const WindowHeightConfig& h) {
        heights = h;
    }

    WindowHeightConfig GetWindowHeights() const {
        return heights;
    }

private:

    std::string json_config_path;
    //graph::GraphConfig graph_impl;
    std::shared_ptr<graph::PluginRegistry> plugin_registry  {nullptr};
    /// Plugin loader - CRITICAL: Must outlive all plugin function calls
    /// Keep this alive throughout entire application lifetime.
    /// Calling dlclose() on plugin_loader invalidates all plugin function pointers.
    /// Root cause of previous graphsim segfaults.
    std::shared_ptr<graph::PluginLoader> plugin_loader  {nullptr};
    std::shared_ptr<graph::NodeFactory> node_factory {nullptr};
    std::shared_ptr<graph::GraphManager> graph_manager {nullptr};
    std::vector<std::string> node_names;
    std::vector<std::string> edge_descriptions;
    WindowHeightConfig heights;



};

}  // namespace app::capabilities

