#include "graph/mock/MockGraphExecutor.hpp"
#include "app/metrics/MetricsEvent.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>
#include <random>

namespace app::capabilities {

// ============================================================================
// MockMetricsPublisher Implementation
// ============================================================================

class MockMetricsPublisher {
public:
    explicit MockMetricsPublisher(int metrics_publish_rate_hz, 
                                   std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap = nullptr);
    ~MockMetricsPublisher();
    
    void Start();
    void Stop();
    void Join();
    
    bool IsRunning() const;
    int GetMetricsPublishCount() const;
    const app::metrics::MetricsEvent& GetLastMetricsEvent() const;
    
    void RegisterMetricsCallback(
        std::function<void(const app::metrics::MetricsEvent&)> callback);
    size_t GetCallbackCount() const;
    
private:
    int metrics_publish_rate_hz_;
    std::atomic<bool> running_{false};
    std::thread metrics_publisher_thread_;
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap_;
    
    struct MockMetric {
        std::string node_name;
        std::string metric_name;
        double current_value = 0.0;
        double min_value = 0.0;
        double max_value = 100.0;
        int update_frequency_hz = 30;
    };
    
    std::vector<MockMetric> metrics_state_;
    mutable std::mutex metrics_lock_;
    
    app::metrics::MetricsEvent last_event_;
    std::atomic<int> publish_count_{0};
    
    std::mt19937 rng_;
    
    std::vector<std::function<void(const app::metrics::MetricsEvent&)>> callbacks_;
    mutable std::mutex callbacks_lock_;
    
    void MetricsPublisherLoop();
    void InitializeMockMetrics();
    app::metrics::MetricsEvent GenerateMetricsEvent();
    std::string GetCurrentTimestamp() const;
};

MockMetricsPublisher::MockMetricsPublisher(int metrics_publish_rate_hz,
                                             std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap)
    : metrics_publish_rate_hz_(metrics_publish_rate_hz),
      metrics_cap_(metrics_cap),
      rng_(std::random_device{}()) {
    if (metrics_publish_rate_hz < 1 || metrics_publish_rate_hz > 199) {
        throw std::invalid_argument("Metrics publish rate must be 1-199 Hz");
    }
}

MockMetricsPublisher::~MockMetricsPublisher() {
    if (running_) {
        Stop();
        Join();
    }
}

void MockMetricsPublisher::Start() {
    if (running_) {
        throw std::runtime_error("MockMetricsPublisher already running");
    }
    
    InitializeMockMetrics();
    running_ = true;
    metrics_publisher_thread_ = std::thread(
        &MockMetricsPublisher::MetricsPublisherLoop, this);
}

void MockMetricsPublisher::Stop() {
    running_ = false;
}

void MockMetricsPublisher::Join() {
    if (metrics_publisher_thread_.joinable()) {
        metrics_publisher_thread_.join();
    }
}

bool MockMetricsPublisher::IsRunning() const {
    return running_.load();
}

int MockMetricsPublisher::GetMetricsPublishCount() const {
    return publish_count_.load();
}

const app::metrics::MetricsEvent& MockMetricsPublisher::GetLastMetricsEvent() const {
    return last_event_;
}

void MockMetricsPublisher::RegisterMetricsCallback(
    std::function<void(const app::metrics::MetricsEvent&)> callback) {
    std::lock_guard<std::mutex> lock(callbacks_lock_);
    callbacks_.push_back(callback);
}

size_t MockMetricsPublisher::GetCallbackCount() const {
    std::lock_guard<std::mutex> lock(callbacks_lock_);
    return callbacks_.size();
}

void MockMetricsPublisher::InitializeMockMetrics() {
    // 6 nodes × 8 metrics = 48 total
    const std::vector<std::pair<std::string, std::string>> node_metrics[] = {
        // DataValidator: 8 metrics
        {
            {"DataValidator", "validation_errors"},
            {"DataValidator", "validation_rate"},
            {"DataValidator", "queue_depth"},
            {"DataValidator", "error_rate"},
            {"DataValidator", "processing_time"},
            {"DataValidator", "memory_usage"},
            {"DataValidator", "cache_hit_rate"},
            {"DataValidator", "uptime"}
        },
        // Transform: 8 metrics
        {
            {"Transform", "rows_processed"},
            {"Transform", "transform_rate"},
            {"Transform", "error_count"},
            {"Transform", "avg_latency"},
            {"Transform", "cpu_usage"},
            {"Transform", "memory_usage"},
            {"Transform", "queue_depth"},
            {"Transform", "success_rate"}
        },
        // Aggregator: 8 metrics
        {
            {"Aggregator", "aggregations_completed"},
            {"Aggregator", "aggregation_rate"},
            {"Aggregator", "group_count"},
            {"Aggregator", "result_size"},
            {"Aggregator", "cpu_usage"},
            {"Aggregator", "memory_usage"},
            {"Aggregator", "pending_groups"},
            {"Aggregator", "computation_time"}
        },
        // Filter: 8 metrics
        {
            {"Filter", "records_evaluated"},
            {"Filter", "filter_rate"},
            {"Filter", "records_passed"},
            {"Filter", "filter_latency"},
            {"Filter", "cpu_usage"},
            {"Filter", "memory_usage"},
            {"Filter", "pass_rate"},
            {"Filter", "error_count"}
        },
        // Sink: 8 metrics
        {
            {"Sink", "records_written"},
            {"Sink", "write_rate"},
            {"Sink", "write_errors"},
            {"Sink", "write_latency"},
            {"Sink", "storage_usage"},
            {"Sink", "buffer_fullness"},
            {"Sink", "flush_count"},
            {"Sink", "success_rate"}
        },
        // Monitor: 8 metrics
        {
            {"Monitor", "health_checks_passed"},
            {"Monitor", "check_rate"},
            {"Monitor", "alert_count"},
            {"Monitor", "response_time"},
            {"Monitor", "cpu_usage"},
            {"Monitor", "memory_usage"},
            {"Monitor", "active_alerts"},
            {"Monitor", "uptime"}
        }
    };
    
    // Initialize all 48 metrics with realistic ranges
    for (const auto& node_metrics : node_metrics) {
        for (const auto& [node, metric] : node_metrics) {
            MockMetric m;
            m.node_name = node;
            m.metric_name = metric;
            m.current_value = 50.0;
            m.min_value = 0.0;
            m.max_value = 100.0;
            m.update_frequency_hz = 30;
            metrics_state_.push_back(m);
        }
    }
}

void MockMetricsPublisher::MetricsPublisherLoop() {
    using namespace std::chrono;
    
    const auto frame_duration = milliseconds(1000 / metrics_publish_rate_hz_);
    auto next_publish = high_resolution_clock::now();
    
    while (running_) {
        auto now = high_resolution_clock::now();
        
        if (now >= next_publish) {
            app::metrics::MetricsEvent event = GenerateMetricsEvent();
            last_event_ = event;
            publish_count_.fetch_add(1, std::memory_order_relaxed);
            
            {
                std::lock_guard<std::mutex> lock(callbacks_lock_);
                for (const auto& callback : callbacks_) {
                    callback(event);
                }
            }
            
            // Also invoke subscribers registered on the MetricsCapability
            if (metrics_cap_) {
                metrics_cap_->InvokeSubscribers(event);
            }
            
            next_publish = now + frame_duration;
        }
        
        std::this_thread::sleep_for(microseconds(100));
    }
}

app::metrics::MetricsEvent MockMetricsPublisher::GenerateMetricsEvent() {
    app::metrics::MetricsEvent event;
    event.timestamp = std::chrono::system_clock::now();
    event.source = "MockGraphExecutor";
    event.event_type = "metrics_update";
    
    std::lock_guard<std::mutex> lock(metrics_lock_);
    
    std::uniform_int_distribution<> metric_dist(0, metrics_state_.size() - 1);
    std::uniform_real_distribution<> value_dist(-5.0, 5.0);
    
    // Update ~10 random metrics per event
    for (int i = 0; i < 10 && i < static_cast<int>(metrics_state_.size()); ++i) {
        int idx = metric_dist(rng_);
        metrics_state_[idx].current_value += value_dist(rng_);
        metrics_state_[idx].current_value = std::max(metrics_state_[idx].min_value,
                                                      std::min(metrics_state_[idx].max_value,
                                                               metrics_state_[idx].current_value));
    }
    
    // Populate event data
    for (const auto& metric : metrics_state_) {
        std::string key = metric.node_name + "/" + metric.metric_name;
        event.data[key] = std::to_string(metric.current_value);
    }
    
    return event;
}

std::string MockMetricsPublisher::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return ss.str();
}

}   // namespace app::capabilities

namespace graph {

// ============================================================================
// MockGraphExecutor Implementation
// ============================================================================

MockGraphExecutor::MockGraphExecutor(int metrics_publish_rate_hz) {
    // Create the capability first
    metrics_cap_ = std::make_shared<app::capabilities::MetricsCapability>();
    capability_bus.Register<app::capabilities::MetricsCapability>(metrics_cap_);
    
    // Then create the publisher with a reference to it
    publisher_ = std::make_unique<app::capabilities::MockMetricsPublisher>(
        metrics_publish_rate_hz, metrics_cap_);
}

MockGraphExecutor::~MockGraphExecutor() {
    if (IsRunning()) {
        Stop();
        Join();
    }
}

InitializationResult MockGraphExecutor::Init() {
    InitializationResult results;
    std::lock_guard<std::mutex> lock(state_lock_);
    if (initialized_) {
        throw std::runtime_error("MockGraphExecutor already initialized");
    }
    
    // Populate MetricsCapability with schemas for all 6 nodes
    std::vector<app::metrics::NodeMetricsSchema> schemas;
    
    // Define the 6 nodes and their 8 metrics each
    const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> nodes = {
        {"DataValidator", "processor", {"validation_errors", "validation_rate", "queue_depth", "error_rate", "processing_time", "memory_usage", "cache_hit_rate", "uptime"}},
        {"Transform", "processor", {"rows_processed", "transform_rate", "error_count", "avg_latency", "cpu_usage", "memory_usage", "queue_depth", "success_rate"}},
        {"Aggregator", "processor", {"aggregations_completed", "aggregation_rate", "group_count", "result_size", "cpu_usage", "memory_usage", "pending_groups", "computation_time"}},
        {"Filter", "processor", {"records_evaluated", "filter_rate", "records_passed", "filter_latency", "cpu_usage", "memory_usage", "pass_rate", "error_count"}},
        {"Sink", "processor", {"records_written", "write_rate", "write_errors", "write_latency", "storage_usage", "buffer_fullness", "flush_count", "success_rate"}},
        {"Monitor", "analyzer", {"health_checks_passed", "check_rate", "alert_count", "response_time", "cpu_usage", "memory_usage", "active_alerts", "uptime"}}
    };
    
    for (const auto& [node_name, node_type, metrics] : nodes) {
        app::metrics::NodeMetricsSchema schema;
        schema.node_name = node_name;
        schema.node_type = node_type;
        
        // Build the metrics_schema JSON with all 8 metrics
        nlohmann::json fields_json = nlohmann::json::array();
        
        for (const auto& metric_name : metrics) {
            nlohmann::json field;
            field["name"] = metric_name;
            field["description"] = "Mock metric: " + metric_name;
            field["display_type"] = "line_chart";
            field["unit"] = "%";
            field["alert_rule"] = {
                {"ok", {0, 50}},
                {"warning", {50, 80}},
                {"critical_range_type", "above"}
            };
            fields_json.push_back(field);
        }
        
        schema.metrics_schema["fields"] = fields_json;
        schemas.push_back(schema);
    }
    
    // Set the schemas on the metrics capability
    metrics_cap_->SetNodeMetricsSchemas(schemas);
    
    initialized_ = true;
    return results;
}

ExecutionResult MockGraphExecutor::Start() {
    ExecutionResult results;
    if (!initialized_) {
        throw std::runtime_error("MockGraphExecutor not initialized. Call Init() first.");
    }
    if (IsRunning()) {
        throw std::runtime_error("MockGraphExecutor already running");
    }
    publisher_->Start();
    return results;
}

ExecutionResult MockGraphExecutor::Stop() {
    ExecutionResult results;
    publisher_->Stop();
    return results; 
}

ExecutionResult MockGraphExecutor::Join() {
    ExecutionResult results;
    publisher_->Join();
    return results;
}

bool MockGraphExecutor::IsRunning() const {
    return publisher_->IsRunning();
}

int MockGraphExecutor::GetMetricsPublishCount() const {
    return publisher_->GetMetricsPublishCount();
}

const app::metrics::MetricsEvent& MockGraphExecutor::GetLastMetricsEvent() const {
    return publisher_->GetLastMetricsEvent();
}

void MockGraphExecutor::RegisterMetricsCallback(
    std::function<void(const app::metrics::MetricsEvent&)> callback) {
    publisher_->RegisterMetricsCallback(callback);
}

size_t MockGraphExecutor::GetCallbackCount() const {
    return publisher_->GetCallbackCount();
}

}  // namespace graph
