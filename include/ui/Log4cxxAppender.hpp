#pragma once

#include "ui/LoggingAppender.hpp"
#include <string>
#include <memory>

/**
 * Log4cxxAppender: Simple bridge from log4cxx to LoggingAppender
 * 
 * This is a lightweight adapter that routes log messages to LoggingAppender's callback.
 * It doesn't need to be a full log4cxx::Appender - we can just invoke the callback directly
 * from dashboard_main where we configure log4cxx.
 */
class Log4cxxAppender {
public:
    explicit Log4cxxAppender(std::shared_ptr<LoggingAppender> appender);

    // Simple method to log a message
    void LogMessage(const std::string& level, const std::string& message);

private:
    std::shared_ptr<LoggingAppender> appender_;
};

using Log4cxxAppenderPtr = std::shared_ptr<Log4cxxAppender>;

