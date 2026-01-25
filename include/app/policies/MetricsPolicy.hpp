#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "app/AppContext.hpp"
#include "app/capabilities/MetricsCapability.hpp"



namespace app::policies {

static auto logger = log4cxx::Logger::getLogger("app.policies.MetricsPolicy");

class MetricsPolicy : public graph::policies::IExecutionPolicy {
public:
    MetricsPolicy() {
        LOG4CXX_TRACE(logger, "MetricsPolicy initialized");
    }   

    virtual ~MetricsPolicy() = default;

    bool PreInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "MetricsPolicy OnConfig called");
        // Configure metrics system here if needed
        return true;
    }

    bool OnInit(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "MetricsPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "MetricsPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "MetricsPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(app::AppContext& context) override {
        LOG4CXX_TRACE(logger, "MetricsPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

private:
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_capability_;

}; // class MetricsPolicy

    
}// namespace app::policies