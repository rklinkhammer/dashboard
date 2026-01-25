
#pragma once


#include <memory>


namespace graph {

    /**
 * @brief Enumeration of execution states for the interactive CLI
 *
 * Represents the complete state machine for graph execution in interactive mode:
 * - INITIALIZED: ExecutionController ready, graph prepared
 * - PAUSED: Running state, but temporarily paused by user
 * - RUNNING: Active execution (graph nodes processing)
 * - STEPPING: Stepping mode (one CSV row per step command)
 * - STOPPING: Graceful shutdown in progress
 * - STOPPED: Execution complete, waiting for termination
 * - ERROR: Fatal error occurred
 *
 * State Transitions:
 *   INITIALIZED → RUNNING (Start with continuous execution)
 *   INITIALIZED → STEPPING (Start with manual stepping)
 *   RUNNING → PAUSED (User pause() command)
 *   PAUSED → RUNNING (User resume() command)
 *   PAUSED → STEPPING (User step command while paused)
 *   RUNNING/PAUSED/STEPPING → STOPPING (User stop() command)
 *   STOPPING → STOPPED (Graph shutdown complete)
 *   Any → ERROR (Fatal error detected)
 *
 * Thread-Safety: ExecutionState is queried via thread-safe getter but not
 * directly mutable - transitions are managed by ExecutionController methods.
 */
enum class ExecutionState : uint8_t {
  INITIALIZED = 0,  ///< Ready to start, graph initialized
  PAUSED = 1,       ///< Running but temporarily paused
  RUNNING = 2,      ///< Active execution (continuous mode)
  STEPPING = 3,     ///< Stepping mode (manual row injection)
  STOPPING = 4,     ///< Graceful shutdown in progress
  STOPPED = 5,      ///< Execution stopped, nodes joined
  ERROR = 6,        ///< Fatal error occurred
  ANY = 99        ///< Wildcard for any state
};

/**
 * @brief Convert ExecutionState enum to human-readable string
 * @param state The execution state
 * @return String representation (e.g., "RUNNING", "PAUSED")
 *
 * Thread-Safety: Safe to call from any thread (stateless function)
 */
std::string GetExecutionStateName(ExecutionState state);

}  // namespace graph