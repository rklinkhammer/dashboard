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
#include "graph/GraphExecutorContext.hpp"
#include "graph/ExecutionState.hpp"
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
    explicit GraphExecutor(std::unique_ptr<ExecutionPolicyChain> policy_chain);

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

    // std::shared_ptr<app::AppContext> GetAppContext() const {
    //     return context_;
    // }
    
    template<typename CapabilityT>
    std::shared_ptr<CapabilityT> GetCapability() const {

        return executor_context_.capability_bus.Get<CapabilityT>();
    }
    
    template<typename CapabilityT>
    bool Has() const {
        return executor_context_.capability_bus.Has<CapabilityT>();
    }

    template<typename CapabilityT>
    void Register(std::shared_ptr<CapabilityT> capability) {
        executor_context_.capability_bus.Register<CapabilityT>(capability);
    }
    
    std::shared_ptr<graph::GraphManager> GetGraphManager() const {
        return graph_manager_;
    }

    void SetGraphManager(std::shared_ptr<graph::GraphManager> graph_manager) {
        graph_manager_ = graph_manager;
        executor_context_.graph_manager = graph_manager_;
    }

    void SetExecutionState(graph::ExecutionState state) {
        current_state_ = state;
    }

    graph::ExecutionState GetExecutionState() const {
        return current_state_;
    }   

    /// @brief Check if stop has been requested
    /// @return true if stop requested, false otherwise
    bool IsStopped() const {
        return is_stopped.load();
    }

    /// @brief Request application stop
    void SetStopped() const {
        is_stopped.store(true);
        executor_context_.SetStopped();
    }

private:

    int CountNodesinLifecycleState(graph::LifecycleState state) const;

    std::unique_ptr<ExecutionPolicyChain> policy_chain_;
    std::shared_ptr<graph::GraphManager> graph_manager_;
    GraphExecutorContext executor_context_;
    ExecutionState current_state_ = graph::ExecutionState::STOPPED;

    mutable std::atomic<bool> is_stopped{false};

};

}  // namespace graph

