#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.CompletionPolicy");

class CompletionPolicy : public graph::policies::IExecutionPolicy {
public:
    CompletionPolicy() {
        LOG4CXX_TRACE(logger, "CompletionPolicy initialized");
    }   

    virtual ~CompletionPolicy() = default;

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CompletionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CompletionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CompletionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CompletionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class CompletionPolicy
    
}// namespace app::policies