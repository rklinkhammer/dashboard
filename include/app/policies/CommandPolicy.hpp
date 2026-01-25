#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"



namespace app::policies {

static auto command_logger = log4cxx::Logger::getLogger("app.policies.CommandPolicy");

class CommandPolicy : public graph::IExecutionPolicy {
public:
    CommandPolicy() {
        LOG4CXX_TRACE(command_logger, "CommandPolicy initialized");
    }   

    virtual ~CommandPolicy() = default;

    bool OnInit(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(command_logger, "CommandPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class CommandPolicy
    
}// namespace app::policies