#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "ui/Dashboard.hpp"

namespace app::policies
{

    static auto dashboard_logger = log4cxx::Logger::getLogger("app.policies.DashboardPolicy");

    class DashboardPolicy : public graph::IExecutionPolicy
    {
    public:
   
        virtual ~DashboardPolicy() = default;

        bool OnInit(graph::GraphExecutorContext &context) override
        {
            LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnInit called");
            auto capability = context.capability_bus.Get<app::capabilities::MetricsCapability>();
            dashboard_ = std::make_shared<Dashboard>(capability, WindowHeightConfig{});
            dashboard_->Initialize();
            // Initialize metrics system here if needed
            return true;
        }

        bool OnStart(graph::GraphExecutorContext &) override
        {
            LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnStart called");
            auto ui = [this]() {
                dashboard_->Run();
            };
            dashboard_thread_ = std::thread(ui);
            return true;
        }

        void OnStop(graph::GraphExecutorContext &) override
        {
            LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnStop called");
            // Stop metrics collection and cleanup here if needed
        }

        void OnJoin(graph::GraphExecutorContext &) override
        {
            LOG4CXX_TRACE(dashboard_logger, "DashboardPolicy OnJoin called");
            if (dashboard_thread_.joinable()) {
                dashboard_thread_.join();
            }
        }

    private:
        std::thread dashboard_thread_;
        std::shared_ptr<Dashboard> dashboard_;

    }; // class DashboardPolicy

} // namespace app::policies