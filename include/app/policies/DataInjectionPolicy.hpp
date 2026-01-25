#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"



namespace app::policies {

static auto datainjection_logger = log4cxx::Logger::getLogger("app.policies.DataInjectionPolicy");

class DataInjectionPolicy : public graph::IExecutionPolicy {
public:
    DataInjectionPolicy() {
        LOG4CXX_TRACE(datainjection_logger, "DataInjectionPolicy initialized");
    }   

    virtual ~DataInjectionPolicy() = default;

    bool OnInit(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(datainjection_logger, "DataInjectionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(datainjection_logger, "DataInjectionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(datainjection_logger, "DataInjectionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(datainjection_logger, "DataInjectionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

}; // class DataInjectionPolicy
    
}// namespace app::policies