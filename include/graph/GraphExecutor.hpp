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

#include "graph/GraphManager.hpp"
#include "graph/DefaultCapabilityBus.hpp"
#include "graph/CapabilityBus.hpp"
#include "graph/ExecutionResult.hpp"
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"
#include <memory>
#include <chrono>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <functional>

namespace graph {

class GraphExecutor {
public:
    /**
     * Construct executor with GraphManager
     *
     * @param graph Shared pointer to GraphManager
     *
     * Preconditions:
     * - graph must be a valid GraphManager pointer
     * - graph must not be nullptr
     *
     * Postconditions:
     * - Executor holds reference to graph
     * - Ready to call Execute()
     *
     * @throws std::invalid_argument if graph is nullptr
     */
    explicit GraphExecutor(std::shared_ptr<app::AppContext> context, 
                                         std::unique_ptr<ExecutionPolicyChain> policy_chain);

    /**
     * Virtual destructor for proper cleanup of derived classes
     */
    virtual ~GraphExecutor() = default;
    
    InitializationResult Init();
    ExecutionResult Start();
    ExecutionResult Run();
    ExecutionResult Stop();
    ExecutionResult Join();
    
    // ========== Query Methods (Lock-Free, Thread-Safe) ==========

    /**
     * @brief Check if execution is currently running
     *
     * @return True if in RUNNING or PAUSED state (graph executing)
     *
     * Thread-Safety: Lock-free atomic read, safe from any thread
     * Time Complexity: O(1)
     */
    bool IsRunning() const;

    /**
     * @brief Check if execution is paused
     *
     * @return True if in PAUSED state
     *
     * Thread-Safety: Lock-free atomic read, safe from any thread
     * Time Complexity: O(1)
     */
    bool IsPaused() const;

    /**
     * @brief Check if execution is stopped
     *
     * @return True if in STOPPED state
     *
     * Thread-Safety: Lock-free atomic read, safe from any thread
     * Time Complexity: O(1)
     */
    bool IsStopped() const;

    /**
     * @brief Check if in error state
     *
     * @return True if in ERROR state
     *
     * Thread-Safety: Lock-free atomic read, safe from any thread
     * Time Complexity: O(1)
     */
    bool IsInError() const;

    void DisplayResults(const ExecutionResult& results) const;
    

    ExecutionResult Execute();

    std::shared_ptr<app::AppContext> GetAppContext() const {
        return context_;
    }
    
    template<typename CapabilityT>
    std::shared_ptr<CapabilityT> Get() const {

        return capability_bus.Get<CapabilityT>();
    }
    
    template<typename CapabilityT>
    bool Has() const {
        return capability_bus.Has<CapabilityT>();
    }

private:

    int CountNodesinLifecycleState(graph::nodes::LifecycleState state) const;

    std::unique_ptr<ExecutionPolicyChain> policy_chain_;
    std::shared_ptr<app::AppContext> context_;
     /// Capability bus for capability discovery and registration
    graph::CapabilityBus capability_bus;

};

}  // namespace graph

