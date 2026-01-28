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

#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/capabilities/GraphCapability.hpp"



namespace app::policies {

static auto command_logger = log4cxx::Logger::getLogger("app.policies.CommandPolicy");

/**
 * @class CommandPolicy
 * @brief Execution policy for command injection and execution during graph runtime
 *
 * CommandPolicy provides hooks for the dashboard to inject commands (start, stop, pause)
 * into the graph execution during runtime. Receives execution phase callbacks and can
 * trigger command processing at appropriate points in the execution lifecycle.
 *
 * Key responsibilities:
 * 1. **Initialization**: Set up command injection infrastructure
 * 2. **Phase Transitions**: Detect and respond to state changes
 * 3. **Command Processing**: Execute injected commands from dashboard
 *
 * Lifecycle:
 * - OnInit(): Prepare command processing (optional)
 * - OnStart(): Begin accepting commands (optional)
 * - OnStop(): Finish processing pending commands
 * - OnJoin(): Clean up after execution
 *
 * Integration Points:
 * - Dashboard sends commands to GraphExecutor
 * - GraphExecutor routes via policies during execution
 * - CommandPolicy hooks key transition points
 * - Policies can influence graph execution (pausing, stepping, etc.)
 *
 * @see IExecutionPolicy, GraphExecutor
 */
class CommandPolicy : public graph::IExecutionPolicy {
public:
    /**
     * @brief Construct a command policy
     */
    CommandPolicy() {
        LOG4CXX_TRACE(command_logger, "CommandPolicy initialized");
    }   

    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~CommandPolicy() = default;

    /**
     * @brief Initialize command infrastructure during graph setup
     *
     * Called by GraphExecutor during Init() phase.
     * Can set up command queues or other infrastructure.
     *
     * @param context GraphExecutorContext with graph reference
     * @return True if initialization succeeded, false on error
     *
     * @see OnStart, OnStop
     */
    bool OnInit(app::capabilities::GraphCapability &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnInit called");
        // Initialize command infrastructure here if needed
        return true;
    }

    /**
     * @brief Start accepting commands from the dashboard
     *
     * Called by GraphExecutor during Start() phase.
     * Enables processing of dashboard commands during execution.
     *
     * @param context GraphExecutorContext for accessing shared resources
     * @return True if startup succeeded, false on error
     *
     * @see OnStop, OnJoin
     */
    bool OnStart(app::capabilities::GraphCapability &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnStart called");
        // Start accepting commands here if needed
        return true;
    }

    /**
     * @brief Stop accepting new commands and finalize pending ones
     *
     * Called by GraphExecutor during Stop() phase.
     * Flushes any pending commands and stops accepting new ones.
     *
     * @param context GraphExecutorContext for cleanup
     *
     * @see OnStart, OnJoin
     */
    void OnStop(app::capabilities::GraphCapability &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnStop called");
        // Stop command processing and cleanup here if needed
    }

    /**
     * @brief Finalize command processing after execution joins
     *
     * Called by GraphExecutor during Join() phase after all nodes complete.
     * Performs final cleanup and reporting of command execution.
     *
     * @param context GraphExecutorContext for final cleanup
     *
     * @see OnStop, OnStart
     */
    void OnJoin(app::capabilities::GraphCapability &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnJoin called");
        // Finalize command processing here if needed
    }   

}; // class CommandPolicy
    
}// namespace app::policies