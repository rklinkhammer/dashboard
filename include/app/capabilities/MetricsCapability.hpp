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
