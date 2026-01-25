// MIT License
//
// Copyright (c) 2025 graphlib contributors
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

/**
 * @file SensorDataRouter.hpp
 * @brief Type-based routing for polymorphic sensor data (Phase 3 Pattern)
 *
 * Implements a clean type-based dispatcher for sensor data variants, supporting
 * hardware device independence via per-port routing. Each router instance handles
 * dispatch for a single port (hardware device).
 *
 * The Type Router pattern enables:
 * - Type-safe handler registration for each sensor type
 * - Polymorphic dispatch of SensorPayload variants
 * - Hardware device independence (one router per port)
 * - Clear separation of routing logic from FSM
 *
 * @see SensorDataTypes.hpp for SensorPayload variant definition
 */

#pragma once

#include "sensor/SensorDataTypes.hpp"
#include <functional>
#include <map>
#include <typeinfo>
#include <stdexcept>
#include <memory>
#include <string>

namespace sensors {

    // ===================================================================================
    // SensorDataRouter - Type-based Dispatch for Sensor Payloads
    // -----------------------------------------------------------------------------------
    // Provides polymorphic routing of sensor data variants to registered handlers.
    // Hardware device independent through per-port router instances.
    // ===================================================================================

    /**
     * @brief Type-based dispatcher for SensorPayload variants
     *
     * Implements the Type Router pattern for polymorphic sensor data handling.
     * Each router instance handles dispatch for a single port (hardware device).
     *
     * **Type-Safe Handler Registration**:
     * @code
     *   router.RegisterHandler<AccelerometerData>([this](const auto& data) {
     *       fsm_.OnAcceleration(data.acceleration, data.timestamp);
     *   });
     * @endcode
     *
     * **Polymorphic Dispatch**:
     * @code
     *   sensors::SensorPayload payload(...);
     *   router.Dispatch(payload);  // Routes to appropriate type handler
     * @endcode
     *
     * **Benefits**:
     * - Centralized type-based dispatch with clean interface
     * - Hardware device independence (one router per port)
     * - Type-safe handler registration
     * - Easy to extend with new sensor types
     * - Clear separation of routing logic from FSM
     */
        class SensorDataRouter {
    public:
        // ====================================================================
        // Type Definitions
        // ====================================================================

        /**
         * @brief Handler function type for sensor data
         * @tparam T The sensor data type (e.g., AccelerometerData)
         *
         * Handler takes a const reference to typed sensor data.
         * Handlers are called when data of matching type is dispatched.
         */
        template<typename T>
        using Handler = std::function<void(const T&)>;

        // ====================================================================
        // Construction / Destruction / Lifecycle
        // ====================================================================

        SensorDataRouter() = default;
        ~SensorDataRouter() = default;

        // Delete copy to avoid handler registration duplication
        SensorDataRouter(const SensorDataRouter&) = delete;
        SensorDataRouter& operator=(const SensorDataRouter&) = delete;

        // Allow move semantics for flexible container usage
        SensorDataRouter(SensorDataRouter&&) = default;
        SensorDataRouter& operator=(SensorDataRouter&&) = default;

        // ====================================================================
        // Handler Registration
        // ====================================================================

        /**
         * @brief Register a handler for a specific sensor type
         * @tparam T The sensor data type (e.g., AccelerometerData)
         * @param handler Function to call when data of type T is dispatched
         *
         * Example:
         * @code
         *   router.RegisterHandler<AccelerometerData>([this](const auto& data) {
         *       fusion_.UpdateAccelerometer(data.acceleration, data.timestamp);
         *   });
         * @endcode
         */
        template<typename T>
        void RegisterHandler(Handler<T> handler) {
            handlers_[typeid(T).hash_code()] = [h = std::move(handler)](const SensorPayload& payload) {
                const auto* ptr = std::get_if<T>(&payload);
                if (ptr) {
                    h(*ptr);
                }
            };
        }

        // ====================================================================
        // Dispatch and Handler Management
        // ====================================================================

        /**
         * @brief Dispatch a sensor payload to registered handler
         * @param payload The sensor data variant to dispatch
         *
         * Looks up handler for the payload's actual type and calls it.
         * Silently ignores payloads with no registered handler (OK if intentional).
         *
         * Example:
         * @code
         *   sensors::SensorPayload payload(std::in_place_type<AccelerometerData>, ...);
         *   router.Dispatch(payload);  // Routes to AccelerometerData handler
         * @endcode
         */
        void Dispatch(const SensorPayload& payload) {
            // Try each known sensor type to find which variant is active
            if (TryDispatch<AccelerometerData>(payload)) return;
            if (TryDispatch<GyroscopeData>(payload)) return;
            if (TryDispatch<MagnetometerData>(payload)) return;
            if (TryDispatch<BarometricData>(payload)) return;
            if (TryDispatch<GPSPositionData>(payload)) return;

            // No handler matched (OK - might be intentional)
        }

        /**
         * @brief Clear all registered handlers
         *
         * Useful for resetting router state or cleaning up before destruction.
         */
        void ClearHandlers() {
            handlers_.clear();
        }

        /**
         * @brief Get count of registered handlers
         * @return Number of distinct sensor type handlers registered
         *
         * Useful for debugging and validation of router configuration.
         */
        int GetHandlerCount() const {
            return static_cast<int>(handlers_.size());
        }

        /**
         * @brief Check if handler is registered for a specific type
         * @tparam T The sensor data type to query
         * @return true if handler registered for type T, false otherwise
         */
        template<typename T>
        bool HasHandler() const {
            return handlers_.find(typeid(T).hash_code()) != handlers_.end();
        }

    private:
        // ====================================================================
        // Private Helper Methods
        // ====================================================================

        /**
         * @brief Try to dispatch payload to handler for type T
         * @tparam T The sensor data type to attempt dispatch for
         * @param payload The sensor payload variant
         * @return true if T matched and handler was called, false otherwise
         *
         * Safely extracts data of type T using std::get_if, calls handler if registered.
         * Returns true even if handler not registered (type matched but no handler).
         */
        template<typename T>
        bool TryDispatch(const SensorPayload& payload) {
            const auto* ptr = std::get_if<T>(&payload);
            if (!ptr) return false;

            // Find handler for this type
            auto type_hash = typeid(T).hash_code();
            auto it = handlers_.find(type_hash);
            if (it != handlers_.end()) {
                it->second(payload);
            }
            return true;  // Type matched, even if no handler registered
        }

        // ====================================================================
        // Private Data Members
        // ====================================================================

        /// @brief Map of type hash codes to dispatch functions
        std::map<size_t, std::function<void(const SensorPayload&)>> handlers_;
    };

}  // namespace sensors

