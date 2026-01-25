#pragma once

/**
 * @file AppContext.hpp
 * @brief Application context for dashboard execution
 *
 * Provides shared state and resources across the application.
 */

#include <string>
#include <memory>

namespace app {

/**
 * @brief Application execution state enumeration
 */
enum class ExecutionState {
    UNINITIALIZED = 0,  ///< Not yet initialized
    INITIALIZED = 1,    ///< Initialized and ready
    RUNNING = 2,        ///< Currently executing
    PAUSED = 3,         ///< Paused execution
    STOPPED = 4,        ///< Stopped
    ERROR = 5           ///< Error state
};

/**
 * @brief Application context object
 *
 * Holds shared application state, configuration, and resources.
 * Used by GraphExecutor and MockGraphExecutor.
 */
class AppContext {
public:
    AppContext() = default;
    virtual ~AppContext() = default;

    // Delete copy operations
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    // Allow move operations
    AppContext(AppContext&&) = default;
    AppContext& operator=(AppContext&&) = default;

    /**
     * @brief Get application name
     */
    const std::string& GetAppName() const { return app_name_; }

    /**
     * @brief Set application name
     */
    void SetAppName(const std::string& name) { app_name_ = name; }

    /**
     * @brief Get application version
     */
    const std::string& GetAppVersion() const { return app_version_; }

    /**
     * @brief Set application version
     */
    void SetAppVersion(const std::string& version) { app_version_ = version; }

protected:
    std::string app_name_ = "Dashboard";
    std::string app_version_ = "0.3.0";
};

} // namespace app
