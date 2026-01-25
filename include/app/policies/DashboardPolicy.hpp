#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"



namespace app::policies {

static auto dashboard_logger = log4cxx::Logger::getLogger("app.policies.DashboardPolicy");

class DashboardPolicy : public graph::IExecutionPolicy {
public:
    DashboardPolicy() {
        LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy initialized");
    }   

    virtual ~DashboardPolicy() = default;

    bool OnInit(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class DashboardPolicy
    
}// namespace app::policies