#pragma once

#include <memory>
#include <chrono>
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "ui/Dashboard.hpp"
#include "ui/BuiltinCommands.hpp"

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
            auto metrics_capability = context.capability_bus.Get<app::capabilities::MetricsCapability>();
            auto graph_capability = context.capability_bus.Get<app::capabilities::GraphCapability>();
            auto command_registry = std::make_shared<CommandRegistry>();
            dashboard_ = std::make_shared<Dashboard>(graph_capability, metrics_capability, command_registry);
            commands::RegisterBuiltinCommands(command_registry, dashboard_.get());

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