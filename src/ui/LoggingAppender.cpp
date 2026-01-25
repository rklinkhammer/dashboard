#include "ui/LoggingAppender.hpp"
#include <iostream>

LoggingAppender::LoggingAppender(LogCallback callback)
    : callback_(callback) {
    std::cerr << "[LoggingAppender] Created with callback\n";
}

void LoggingAppender::LogMessage(const std::string& level, const std::string& message) {
    try {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (callback_) {
            callback_(level, message);
        }
    } catch (const std::exception& e) {
        std::cerr << "[LoggingAppender] Error in LogMessage: " << e.what() << "\n";
    }
}
