#pragma once

#include "app/metrics/MetricsEvent.hpp"
#include "graph/ExecutionResult.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <random>
#include <mutex>
#include <vector>
#include <chrono>
#include <map>

namespace app::capabilities {

// Forward declaration
class MockMetricsPublisher;


} // namespace app::capabilities

namespace graph {
/**
 * @class MockGraphExecutor
 * @brief Test harness for dashboard development and validation
 *
 * MockGraphExecutor simulates a graph execution engine with synthetic metrics.
 * It provides:
 * - Configurable metrics publishing rate (1-199 Hz)
 * - 48 realistic metrics across 6 node types
 * - Thread-safe callback mechanism for metrics updates
 * - Lifecycle management (Init/Start/Stop/Join)
 *
 * Used during Phase 0-1 development for dashboard testing before integration
 * with the real GraphExecutor.
 */


class MockGraphExecutor {
public:
    /**
     * @brief Construct MockGraphExecutor with specified publishing rate
     * @param metrics_publish_rate_hz Publishing frequency in Hz (1-199, default 30)
     * @throws std::invalid_argument if rate is outside valid range
     */
    explicit MockGraphExecutor(int metrics_publish_rate_hz = 30);
    
    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~MockGraphExecutor();
    
    // Lifecycle management
    /**
     * @brief Initialize executor and metrics capability
     * @throws std::runtime_error if already initialized
     */
    InitializationResult Init();
    
    /**
     * @brief Start metrics publishing thread
     * @throws std::runtime_error if not initialized or already running
     */
    ExecutionResult Start();
    
    /**
     * @brief Stop metrics publishing thread
     */
    ExecutionResult Stop();
    
    /**
     * @brief Wait for metrics publisher thread to finish
     */
    ExecutionResult Join();
    
    // Status queries
    /**
     * @brief Check if executor is currently publishing metrics
     * @return true if running, false otherwise
     */
    bool IsRunning() const;
    
    // Metrics access
    /**
     * @brief Get total number of metrics events published
     * @return Event count
     */
    int GetMetricsPublishCount() const;
    
    /**
     * @brief Get the most recently published metrics event
     * @return Reference to last metrics event
     */
    const app::metrics::MetricsEvent& GetLastMetricsEvent() const;
    
    // Callback registration
    /**
     * @brief Register a callback to receive metrics events
     * @param callback Function invoked for each metrics event
     *
     * Called by dashboard during initialization to subscribe to metrics updates.
     * Callback is invoked from metrics publisher thread - must be thread-safe.
     */
    void RegisterMetricsCallback(
        std::function<void(const app::metrics::MetricsEvent&)> callback);
    
    /**
     * @brief Get number of registered callbacks
     * @return Callback count (for testing)
     */
    size_t GetCallbackCount() const;
    
    template<typename CapabilityT>
    std::shared_ptr<CapabilityT> GetCapability() const {

        return capability_bus.Get<CapabilityT>();
    }
    
    template<typename CapabilityT>
    bool HasCapability() const {
        return capability_bus.Has<CapabilityT>();
    }

private:
    // Internal publisher (handles actual metrics generation)
    std::unique_ptr<app::capabilities::MockMetricsPublisher> publisher_;
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap_;

    // Metrics state and synchronization
    mutable std::mutex state_lock_;
    std::atomic<bool> initialized_{false};
    graph::CapabilityBus capability_bus;

};

}  // namespace graph   