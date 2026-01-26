#include "ui/Log4cxxAppender.hpp"
#include <iostream>

Log4cxxAppender::Log4cxxAppender(std::shared_ptr<LoggingAppender> appender)
    : appender_(appender) {
    std::cerr << "[Log4cxxAppender] Created\n";
}

void Log4cxxAppender::LogMessage(const std::string& level, const std::string& message) {
    if (appender_) {
        appender_->LogMessage(level, message);
    }
}

