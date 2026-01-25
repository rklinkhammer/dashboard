#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"



namespace app::policies {

static auto csv_injection_logger_ = log4cxx::Logger::getLogger("app.policies.CSVInjectionPolicy");

class CSVInjectionPolicy : public graph::IExecutionPolicy {
public:
    CSVInjectionPolicy() {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy initialized");
    }   

    virtual ~CSVInjectionPolicy() = default;

    bool OnInit(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

    void SetCSVInputPaths(const std::vector<std::pair<std::string, std::string>>& paths) {
        csv_input_paths_ = paths;
        LOG4CXX_TRACE(csv_injection_logger_, "CSVDataInjectionPolicy::SetCSVInputPaths() - " 
                         << paths.size() << " paths set");
    }   

    private:

        std::vector<std::pair<std::string, std::string>> csv_input_paths_;

}; // class CSVInjectionPolicy
    
}// namespace app::policies