#include "ui/LoggingWindowAppender.hpp"
#include "ui/LoggingAppender.hpp"
#include <log4cxx/level.h>
#include <log4cxx/helpers/pool.h>

LoggingWindowAppender::LoggingWindowAppender(std::shared_ptr<LoggingAppender> logging_appender)
    : logging_appender_(logging_appender) {}

void LoggingWindowAppender::append(const log4cxx::spi::LoggingEventPtr& event,
                                    log4cxx::helpers::Pool& pool) {
    if (!logging_appender_ || !event) {
        return;
    }

    auto layout = getLayout();
    if (!layout) {
        return;
    }

    try {
        // Format the message using the configured layout
        std::string message;
        layout->format(message, event, pool);

        // Extract the level from the event as a string
        std::string level = event->getLevel()->toString();
        
        // Route to LoggingAppender callback, which forwards to LoggingWindow
        logging_appender_->LogMessage(level, message);
        
    } catch (...) {
        // Silently ignore any errors - never throw from append()
    }
}

void LoggingWindowAppender::close() {
    // No resources to clean up
}
