#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.CommandPolicy");

class CommandPolicy : public graph::policies::IExecutionPolicy {
public:
    CommandPolicy() {
        LOG4CXX_TRACE(logger, "CommandPolicy initialized");
    }   

    virtual ~CommandPolicy() = default;

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CommandPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CommandPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CommandPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CommandPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class CommandPolicy
    
}// namespace app::policies