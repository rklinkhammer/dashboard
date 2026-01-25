#pragma once

#include <string>
#include <chrono>
#include <memory>
#include "graph/CapabilityBus.hpp"
#include "graph/GraphManager.hpp"

namespace graph
{
    struct GraphExecutorContext
    {

        std::shared_ptr<GraphManager> GetGraphManager() const
        {
            return graph_manager;
        }

        /// @brief Check if stop has been requested
        /// @return true if stop requested, false otherwise
        bool IsStopped() const
        {
            return is_stopped.load();
        }

        /// @brief Request application stop
        void SetStopped() const
        {
            is_stopped.store(true);
        }

        /// Capability bus for capability discovery and registration
        CapabilityBus capability_bus;
        std::shared_ptr<GraphManager> graph_manager;
        mutable std::atomic<bool> is_stopped{false};
    };

} // namespace graph