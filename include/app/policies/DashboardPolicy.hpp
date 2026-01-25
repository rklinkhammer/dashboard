#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.DashboardPolicy");

class DashboardPolicy : public graph::policies::IExecutionPolicy {
public:
    DashboardPolicy() {
        LOG4CXX_TRACE(logger, "DashboardPolicy initialized");
    }   

    virtual ~DashboardPolicy() = default;

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DashboardPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DashboardPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DashboardPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "DashboardPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class DashboardPolicy
    
}// namespace app::policies