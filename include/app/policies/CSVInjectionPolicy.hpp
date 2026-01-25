#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.CSVInjectionPolicy");

class CSVInjectionPolicy : public graph::policies::IExecutionPolicy {
public:
    CSVInjectionPolicy() {
        LOG4CXX_TRACE(logger, "CSVInjectionPolicy initialized");
    }   

    virtual ~CSVInjectionPolicy() = default;

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CSVInjectionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CSVInjectionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CSVInjectionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "CSVInjectionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class CSVInjectionPolicy
    
}// namespace app::policies