#pragma once

#include <string>
#include <map>
#include <cstdint>
#include "graph/ExecutionState.hpp"
namespace graph {

/**
 * @brief Result of graph initialization
 *
 * Contains success/failure information and per-node initialization details.
 */
struct InitializationResult {
    /// True if all nodes initialized successfully
    bool success = false;

    /// Human-readable status message
    std::string message;

    /// Number of nodes initialized
    uint32_t nodes_initialized = 0;

    /// Number of nodes that failed initialization
    uint32_t nodes_failed = 0;

    /// Per-node initialization times (milliseconds)
    std::map<std::string, uint32_t> node_init_times_ms;

    /// Total initialization time (milliseconds)
    uint32_t elapsed_time_ms = 0;

    /// Error details if initialization failed
    std::string error_details;
};

/**
 * @brief Result of execution operations (Start, Stop, etc.)
 *
 * Indicates whether the operation succeeded and provides status information.
 */
struct ExecutionResult {
    /// True if operation succeeded
    bool success = false;

    /// Human-readable status message
    std::string message;

    /// Current execution state after operation
    graph::ExecutionState current_state = graph::ExecutionState::ERROR;

    /// Time taken for operation (milliseconds)
    uint32_t elapsed_time_ms = 0;

    /// Error details if operation failed
    std::string error_details;
};

} // namespace graph
