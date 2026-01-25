#include <memory>
#include <chrono>
#include <thread>
#include <log4cxx/logger.h>
#include "core/ActiveQueue.hpp"
#include "app/metrics/MetricsEvent.hpp"
#include "graph/IMetricsCallback.hpp"
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"
#include "app/capabilities/MetricsCapability.hpp"



namespace app::policies {

static auto metrics_logger = log4cxx::Logger::getLogger("app.policies.MetricsPolicy");

struct MetricsCapabilityCallback : public graph::IMetricsCallback {
    void PublishAsync(const app::metrics::MetricsEvent& event) noexcept override{
        if (on_publish_async_) {
            on_publish_async_(event);
        }
    }
    std::function<void(const app::metrics::MetricsEvent&)> on_publish_async_;
};


class MetricsPolicy : public graph::IExecutionPolicy {
public:
    MetricsPolicy() {
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy initialized");
    }   

    virtual ~MetricsPolicy() = default;

    bool OnInit(graph::GraphExecutorContext& context) override {
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy OnInit called");
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy::OnInit() - creating MetricsCapability");
        metrics_capability_ = std::make_shared<app::capabilities::MetricsCapability>();
        context.capability_bus.Register<app::capabilities::MetricsCapability>(metrics_capability_);
        InitMetricsSources(context);
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy::OnInit() - MetricsCapability registered in CapabilityBus");
        return true;
    }

    bool OnStart(graph::GraphExecutorContext& context) override
    {
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy::OnStart()");
        bool ret = true;
        auto fn = [this, &context]()
        {
            while (!context.IsStopped())
            {
                app::metrics::MetricsEvent event;
                if (metrics_event_queue_.Dequeue(event))
                {
                    LOG4CXX_TRACE(metrics_logger, "MetricsPolicy: Publishing metrics event from Node '" 
                        << event.source << "' "
                        << "of type '" << event.event_type << "' with " << event.data.size() << " fields:");
                    for (const auto &[field, value] : event.data)
                    {
                        LOG4CXX_TRACE(metrics_logger, "  Field: " << field << " = " << value);
                    }
                    metrics_capability_->InvokeSubscribers(event);
                }
                else
                {
                    context.SetStopped();
                }
            }
        };
        metrics_thread_ = std::thread(fn);
        return ret;
    }

    void OnStop(graph::GraphExecutorContext &) override
    {
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy OnStop called");
        metrics_event_queue_.Disable();
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(metrics_logger, "MetricsPolicy OnJoin called");
        if (metrics_thread_.joinable()) {
            metrics_thread_.join();
        }
    }   

    std::unordered_map<std::string, app::metrics::NodeMetricsSchema>& GetNodeMetricsSchemas() {
        return node_metrics_schemas_;
    }

    void AddNodeMetrics( const std::string& node_name,
                        std::shared_ptr<MetricsCapabilityCallback> callback,
                        const app::metrics::NodeMetricsSchema& schema) {
        metrics_node_callbacks_[node_name] = callback;
        node_metrics_schemas_[node_name] = schema;
    }


private:
    void InitMetricsSources(graph::GraphExecutorContext& context);

    std::thread metrics_thread_;
    core::ActiveQueue<app::metrics::MetricsEvent> metrics_event_queue_;
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_capability_;
    std::unordered_map<std::string, std::shared_ptr<MetricsCapabilityCallback>> metrics_node_callbacks_;
    std::unordered_map<std::string, app::metrics::NodeMetricsSchema> node_metrics_schemas_;

}; // class MetricsPolicy

    
}// namespace app::policies