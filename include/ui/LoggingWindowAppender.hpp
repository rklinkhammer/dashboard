#pragma once

#include <log4cxx/appenderskeleton.h>
#include <log4cxx/spi/loggingevent.h>
#include <memory>

// Forward declaration
class LoggingAppender;

/**
 * LoggingWindowAppender: Proper log4cxx appender for displaying messages in LoggingWindow
 * 
 * Inherits from log4cxx::AppenderSkeleton and implements the append() interface.
 * This allows it to be registered with log4cxx's root logger and receive all LOG4CXX_* calls.
 * 
 * Messages are routed to LoggingAppender's callback, which displays them in LoggingWindow.
 */
class LoggingWindowAppender : public log4cxx::AppenderSkeleton {
public:
    explicit LoggingWindowAppender(std::shared_ptr<LoggingAppender> logging_appender);
    ~LoggingWindowAppender() override = default;

    /**
     * Append a log event to the LoggingWindow via callback
     * 
     * @param event The log event from log4cxx
     * @param pool Memory pool for object allocation
     */
    void append(const log4cxx::spi::LoggingEventPtr& event,
                log4cxx::helpers::Pool& pool) override;

    /**
     * Close the appender (no resources to clean up)
     */
    void close() override;

    /**
     * This appender requires a layout to format messages
     */
    bool requiresLayout() const override { return true; }

private:
    std::shared_ptr<LoggingAppender> logging_appender_;
};
