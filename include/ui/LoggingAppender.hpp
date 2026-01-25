#pragma once

#include <string>
#include <memory>
#include <functional>
#include <mutex>

/**
 * Simple logging callback interface - doesn't require log4cxx dependencies
 * Can be extended later with proper log4cxx integration if needed
 */
class ILoggingCallback {
public:
    virtual ~ILoggingCallback() = default;
    virtual void OnLogMessage(const std::string& level, const std::string& message) = 0;
};

/**
 * LoggingAppender: Captures log messages via callback
 * Designed to be simple and decoupled from log4cxx internals
 */
class LoggingAppender {
public:
    using LogCallback = std::function<void(const std::string& level, const std::string& message)>;

    explicit LoggingAppender(LogCallback callback);
    ~LoggingAppender() = default;

    // Log a message
    void LogMessage(const std::string& level, const std::string& message);

    // Set the callback
    void SetCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_ = callback;
    }

    // Get callback
    LogCallback GetCallback() const {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        return callback_;
    }

protected:
    LogCallback callback_;
    mutable std::mutex callback_mutex_;
};

// Helper to create appender
using LoggingAppenderPtr = std::shared_ptr<LoggingAppender>;
inline LoggingAppenderPtr NewLoggingAppender(LoggingAppender::LogCallback callback) {
    return std::make_shared<LoggingAppender>(callback);
}
