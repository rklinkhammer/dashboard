// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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