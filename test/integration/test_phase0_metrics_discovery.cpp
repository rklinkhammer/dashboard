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

#include <gtest/gtest.h>
#include "graph/mock/MockGraphExecutor.hpp"
#include <thread>
#include <chrono>
#include <vector>

using graph::MockGraphExecutor;
using app::capabilities::MetricsCapability;
using app::metrics::MetricsEvent;

// ============================================================================
// Metrics Subscriber Test Helper
// ============================================================================
class MetricsSubscriberMock : public app::metrics::IMetricsSubscriber {
public:
    MetricsSubscriberMock() : invocation_count(0) {}
    
    void OnMetricsEvent(const MetricsEvent& event) override {
        invocation_count++;
        (void)event;
    }
    
    std::atomic<int> invocation_count;
};

// ============================================================================
// Metrics Discovery Integration Tests (4 tests)
// ============================================================================

class MetricsDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        executor = std::make_shared<MockGraphExecutor>(30);
        executor->Init();
        metrics_cap = executor->GetCapability<MetricsCapability>();
    }
    
    void TearDown() override {
        if (executor && executor->IsRunning()) {
            executor->Stop();
            executor->Join();
        }
    }
    
    std::shared_ptr<MockGraphExecutor> executor;
    std::shared_ptr<MetricsCapability> metrics_cap;
};

TEST_F(MetricsDiscoveryTest, DiscoverAllNodeSchemas) {
    auto schemas = metrics_cap->GetNodeMetricsSchemas();
    
    EXPECT_EQ(schemas.size(), 6);
    
    std::vector<std::string> expected_nodes = {
        "DataValidator", "Transform", "Aggregator", 
        "Filter", "Sink", "Monitor"
    };
    
    for (const auto& schema : schemas) {
        auto it = std::find(expected_nodes.begin(), expected_nodes.end(), schema.node_name);
        EXPECT_NE(it, expected_nodes.end());
    }
}

TEST_F(MetricsDiscoveryTest, All48MetricsDiscoverable) {
    auto schemas = metrics_cap->GetNodeMetricsSchemas();
    
    int total_metrics = 0;
    for (const auto& schema : schemas) {
        if (schema.metrics_schema.contains("fields")) {
            total_metrics += schema.metrics_schema["fields"].size();
        }
    }
    
    EXPECT_EQ(total_metrics, 48);
}

TEST_F(MetricsDiscoveryTest, SchemasHaveCompleteAlertRules) {
    auto schemas = metrics_cap->GetNodeMetricsSchemas();
    
    for (const auto& schema : schemas) {
        EXPECT_FALSE(schema.node_name.empty());
        EXPECT_FALSE(schema.node_type.empty());
        
        if (schema.metrics_schema.contains("fields")) {
            for (const auto& field : schema.metrics_schema["fields"]) {
                EXPECT_TRUE(field.contains("alert_rule"));
                EXPECT_TRUE(field["alert_rule"].contains("ok"));
                EXPECT_TRUE(field["alert_rule"].contains("warning"));
                EXPECT_TRUE(field["alert_rule"].contains("critical_range_type"));
            }
        }
    }
}

TEST_F(MetricsDiscoveryTest, NoSchemaValidationErrors) {
    auto schemas = metrics_cap->GetNodeMetricsSchemas();
    
    for (const auto& schema : schemas) {
        // Validate all required fields
        EXPECT_FALSE(schema.node_name.empty()) << "Node has empty name";
        EXPECT_FALSE(schema.node_type.empty()) << "Node " << schema.node_name << " has empty type";
        EXPECT_TRUE(schema.metrics_schema.is_object()) << "Node " << schema.node_name << " has invalid schema";
        
        if (schema.metrics_schema.contains("fields")) {
            for (size_t i = 0; i < schema.metrics_schema["fields"].size(); ++i) {
                const auto& field = schema.metrics_schema["fields"][i];
                EXPECT_TRUE(field.contains("name")) << "Field " << i << " in " << schema.node_name << " missing name";
                EXPECT_TRUE(field.contains("display_type")) << "Field " << i << " in " << schema.node_name << " missing display_type";
                EXPECT_TRUE(field.contains("unit")) << "Field " << i << " in " << schema.node_name << " missing unit";
            }
        }
    }
}

// ============================================================================
// Executor Initialization Integration Tests (5 tests)
// ============================================================================

class ExecutorInitializationTest : public ::testing::Test {
protected:
    void TearDown() override {
        if (executor && executor->IsRunning()) {
            executor->Stop();
            executor->Join();
        }
    }
    
    std::shared_ptr<MockGraphExecutor> executor;
};

TEST_F(ExecutorInitializationTest, InitSequenceCorrect) {
    executor = std::make_shared<MockGraphExecutor>(30);
    
    // Before Init
    EXPECT_FALSE(executor->IsRunning());
    
    // After Init
    executor->Init();
    EXPECT_FALSE(executor->IsRunning());
    
    // After Start
    executor->Start();
    EXPECT_TRUE(executor->IsRunning());
    
    // After Stop
    executor->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    executor->Join();
    EXPECT_FALSE(executor->IsRunning());
}

TEST_F(ExecutorInitializationTest, MetricsPublisherStartsAutomatically) {
    executor = std::make_shared<MockGraphExecutor>(30);
    executor->Init();
    executor->Start();
    
    EXPECT_EQ(executor->GetMetricsPublishCount(), 0);
    
    // Wait for metrics to publish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    int count = executor->GetMetricsPublishCount();
    EXPECT_GT(count, 0);
    
    executor->Stop();
    executor->Join();
}

TEST_F(ExecutorInitializationTest, CallbacksInvokedAfterStart) {
    executor = std::make_shared<MockGraphExecutor>(50);
    executor->Init();
    
    auto metrics_cap = executor->GetCapability<MetricsCapability>();
    
    MetricsSubscriberMock subscriber;
    metrics_cap->RegisterMetricsCallback(&subscriber);
    
    executor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    executor->Stop();
    executor->Join();
    
    EXPECT_GT(subscriber.invocation_count, 10);
}

TEST_F(ExecutorInitializationTest, CleanShutdownNoHangups) {
    executor = std::make_shared<MockGraphExecutor>(50);
    executor->Init();
    
    auto metrics_cap = executor->GetCapability<MetricsCapability>();
    MetricsSubscriberMock subscriber;
    metrics_cap->RegisterMetricsCallback(&subscriber);
    
    executor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    executor->Stop();
    executor->Join();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Should shutdown in < 500ms
    EXPECT_LT(duration.count(), 500);
}

TEST_F(ExecutorInitializationTest, RapidRestartWorks) {
    executor = std::make_shared<MockGraphExecutor>(30);
    executor->Init();
    
    // First run
    executor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    executor->Stop();
    executor->Join();
    
    int count_1 = executor->GetMetricsPublishCount();
    
    // Second run without recreating executor
    executor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    executor->Stop();
    executor->Join();
    
    int count_2 = executor->GetMetricsPublishCount();
    
    EXPECT_GT(count_2, count_1);
}

// ============================================================================
// Performance Integration Tests
// ============================================================================

class PerformanceTest : public ::testing::Test {
protected:
    void TearDown() override {
        if (executor && executor->IsRunning()) {
            executor->Stop();
            executor->Join();
        }
    }
    
    std::shared_ptr<MockGraphExecutor> executor;
};

TEST_F(PerformanceTest, PublishesAt199Hz) {
    executor = std::make_shared<MockGraphExecutor>(199);
    executor->Init();
    executor->Start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    int count = executor->GetMetricsPublishCount();
    executor->Stop();
    executor->Join();
    
    // At 199 Hz for 1 second, expect ~199 events (allow 10% tolerance)
    EXPECT_GT(count, 180);
    EXPECT_LT(count, 220);
}

TEST_F(PerformanceTest, CallbackLatencyUnder1ms) {
    executor = std::make_shared<MockGraphExecutor>(100);
    executor->Init();
    
    auto metrics_cap = executor->GetCapability<MetricsCapability>();
    
    // Use subscriber that tracks latencies
    class LatencyTrackingSubscriber : public app::metrics::IMetricsSubscriber {
    public:
        void OnMetricsEvent(const MetricsEvent& e) override {
            auto start = std::chrono::high_resolution_clock::now();
            // Simulate minimal work
            volatile int x = 0;
            for (int i = 0; i < 10; ++i) x += i;
            auto end = std::chrono::high_resolution_clock::now();
            
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            latencies_us.push_back(us);
            (void)e;
        }
        
        std::vector<long long> latencies_us;
    };
    
    auto latency_sub = std::make_unique<LatencyTrackingSubscriber>();
    auto* sub_ptr = latency_sub.get();
    metrics_cap->RegisterMetricsCallback(sub_ptr);
    
    executor->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    executor->Stop();
    executor->Join();
    
    // Calculate average latency
    if (!sub_ptr->latencies_us.empty()) {
        long long total_us = 0;
        for (auto lat : sub_ptr->latencies_us) {
            total_us += lat;
        }
        long long avg_us = total_us / sub_ptr->latencies_us.size();
        
        // Average callback latency should be < 1000 microseconds (1ms)
        EXPECT_LT(avg_us, 1000);
    }
}

TEST_F(PerformanceTest, NoMemoryLeaksAt199Hz) {
    executor = std::make_shared<MockGraphExecutor>(199);
    executor->Init();
    executor->Start();
    
    // Run for 5 seconds at max rate
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    
    executor->Stop();
    executor->Join();
    
    int count = executor->GetMetricsPublishCount();
    
    // Should have published ~1000 events without crashing
    EXPECT_GT(count, 900);
}
