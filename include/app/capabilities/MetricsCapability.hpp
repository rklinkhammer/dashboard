#pragma once

#include "app/metrics/MetricsEvent.hpp"
#include "app/metrics/NodeMetricsSchema.hpp"
#include "app/metrics/IMetricsSubscriber.hpp"
#include "graph/CapabilityBus.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

/**
 * @file MetricsCapability.hpp
 * @brief Capability interface for metrics discovery and subscription
 *
 * Provides methods for discovering node metrics schemas and registering
 * callbacks to receive metrics events published during graph execution.
 */

namespace app::capabilities {

class MetricsCapability {
public:
    virtual ~MetricsCapability() = default;
    

    // Registration: Register callback for metrics updates
    // Called during dashboard Initialize()
    // Callback invoked by MetricsPublisher thread
    virtual void RegisterMetricsCallback(
        app::metrics::IMetricsSubscriber* callback) {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        subscribers_.push_back(callback);
    }

    virtual void UnregisterMetricsCallback(
        app::metrics::IMetricsSubscriber* callback) {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        subscribers_.erase(
            std::remove(subscribers_.begin(), subscribers_.end(), callback),
            subscribers_.end());
    }   

    // virtual bool IsPublishing() const = 0;
    virtual size_t GetCallbackCount() {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        return subscribers_.size();
    }   

    /**
     * @brief Set the metrics schemas (protected access for implementations)
     */
    virtual void SetNodeMetricsSchemas(std::vector<app::metrics::NodeMetricsSchema> schemas) {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        schemas_ = schemas;
    }

    std::vector<app::metrics::NodeMetricsSchema> GetNodeMetricsSchemas() {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        return schemas_;
    }
    
    /**
     * @brief Invoke all registered subscribers with a metrics event
     * Called by MetricsPublisher to notify subscribers
     */
    virtual void InvokeSubscribers(const app::metrics::MetricsEvent& event) {
        std::lock_guard<std::mutex> lock(callbacks_lock_);
        for (auto& subscriber : subscribers_) {
            if (subscriber) {
                subscriber->OnMetricsEvent(event);
            }
        }
    }
    
protected:

    std::mutex callbacks_lock_;
    std::vector<app::metrics::IMetricsSubscriber*> subscribers_;   
    std::vector<app::metrics::NodeMetricsSchema> schemas_;
};

}  // namespace app::capabilities
