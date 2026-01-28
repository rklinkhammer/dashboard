#include "graph/ExecutionState.hpp"
#include <string>

namespace graph {
/**
 * @brief Get the string representation of an ExecutionState
 *
 * @param state The ExecutionState enum value
 * @return Corresponding string name
 */

std::string GetExecutionStateName(graph::ExecutionState state) {
  switch (state) {
    case graph::ExecutionState::INITIALIZED:
      return "INITIALIZED";
    case graph::ExecutionState::PAUSED:
      return "PAUSED";
    case graph::ExecutionState::RUNNING:
      return "RUNNING";
    case graph::ExecutionState::STEPPING:
      return "STEPPING";
    case graph::ExecutionState::STOPPING:
      return "STOPPING";
    case graph::ExecutionState::STOPPED:
      return "STOPPED";
    case graph::ExecutionState::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

}  // namespace graph`
