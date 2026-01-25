#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.DataInjectionPolicy");

class DataInjectionPolicy : public graph::policies::IExecutionPolicy {
public:
    DataInjectionPolicy() {
        LOG4CXX_TRACE(logger, "DataInjectionPolicy initialized");
    }   

    virtual ~DataInjectionPolicy() = default;

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DataInjectionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DataInjectionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DataInjectionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DataInjectionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class DataInjectionPolicy
    
}// namespace app::policies