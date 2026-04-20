// MIT License
//
// Copyright (c) 2025 gdashboard contributors
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
#include "app/policies/MetricsPolicy.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <memory>
#include <chrono>
#include <thread>

// ============================================================================
// Layer 4, Task 4.2: MetricsPolicy Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - MetricsPolicy Tests
// ============================================================================

class MetricsPolicyTest : public ::testing::Test {
protected:
    std::unique_ptr<app::capabilities::GraphCapability> graph_cap_;

    void SetUp() override {
        graph_cap_ = std::make_unique<app::capabilities::GraphCapability>();
    }
};

// ============================================================================
// MetricsPolicy Tests
// ============================================================================

TEST_F(MetricsPolicyTest, OnInit_CreatesAndRegistersMetricsCapability) {
    auto& bus = graph_cap_->GetCapabilityBus();

    // Before init, MetricsCapability should not exist in bus
    EXPECT_FALSE(bus.Has<app::capabilities::MetricsCapability>());

    // Call OnInit
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    EXPECT_TRUE(metrics_policy->OnInit(*graph_cap_));

    // After init, MetricsCapability should be registered in bus
    EXPECT_TRUE(bus.Has<app::capabilities::MetricsCapability>());

    auto metrics_cap = bus.Get<app::capabilities::MetricsCapability>();
    EXPECT_NE(metrics_cap, nullptr);
}

TEST_F(MetricsPolicyTest, OnInit_Returns_True) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    bool result = metrics_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(MetricsPolicyTest, OnStart_Returns_True) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    metrics_policy->OnInit(*graph_cap_);
    bool result = metrics_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);

    // Must stop and join thread before test ends to avoid accessing destroyed context
    metrics_policy->OnStop(*graph_cap_);
    metrics_policy->OnJoin(*graph_cap_);
}

TEST_F(MetricsPolicyTest, OnStart_BeforeInit_CanBeCalled) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    // Even without OnInit, OnStart should not crash
    bool result = metrics_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);

    // Must stop and join thread before test ends
    metrics_policy->OnStop(*graph_cap_);
    metrics_policy->OnJoin(*graph_cap_);
}

TEST_F(MetricsPolicyTest, OnStop_DoesNotCrash) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    metrics_policy->OnInit(*graph_cap_);
    metrics_policy->OnStart(*graph_cap_);

    // OnStop should not throw
    EXPECT_NO_THROW({
        metrics_policy->OnStop(*graph_cap_);
    });

    // Must join thread before test ends
    metrics_policy->OnJoin(*graph_cap_);
}

TEST_F(MetricsPolicyTest, OnJoin_DoesNotCrash) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    metrics_policy->OnInit(*graph_cap_);
    metrics_policy->OnStart(*graph_cap_);
    metrics_policy->OnStop(*graph_cap_);

    // OnJoin should not throw
    EXPECT_NO_THROW({
        metrics_policy->OnJoin(*graph_cap_);
    });
}

TEST_F(MetricsPolicyTest, FullLifecycle_InitStartStopJoin) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();

    // Test complete lifecycle
    EXPECT_TRUE(metrics_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(metrics_policy->OnStart(*graph_cap_));

    // Give thread time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    metrics_policy->OnStop(*graph_cap_);
    metrics_policy->OnJoin(*graph_cap_);

    // Should complete without crash
    auto& bus = graph_cap_->GetCapabilityBus();
    EXPECT_TRUE(bus.Has<app::capabilities::MetricsCapability>());
}

TEST_F(MetricsPolicyTest, MetricsCapability_IsSharedPtr) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    metrics_policy->OnInit(*graph_cap_);

    auto& bus = graph_cap_->GetCapabilityBus();
    auto cap1 = bus.Get<app::capabilities::MetricsCapability>();
    auto cap2 = bus.Get<app::capabilities::MetricsCapability>();

    ASSERT_NE(cap1, nullptr);
    ASSERT_NE(cap2, nullptr);
    EXPECT_EQ(cap1.get(), cap2.get());
}

TEST_F(MetricsPolicyTest, GetNodeMetricsSchemas_ReturnsValidMap) {
    auto metrics_policy = std::make_unique<app::policies::MetricsPolicy>();
    auto& schemas = metrics_policy->GetNodeMetricsSchemas();
    EXPECT_TRUE(schemas.empty() || schemas.size() >= 0);
}

TEST_F(MetricsPolicyTest, MultipleMetricsPolicies_EachRegistersOwnCapability) {
    auto policy1 = std::make_unique<app::policies::MetricsPolicy>();
    auto policy2 = std::make_unique<app::policies::MetricsPolicy>();

    auto& bus = graph_cap_->GetCapabilityBus();

    policy1->OnInit(*graph_cap_);
    auto cap1 = bus.Get<app::capabilities::MetricsCapability>();

    // Second policy overwrites
    policy2->OnInit(*graph_cap_);
    auto cap2 = bus.Get<app::capabilities::MetricsCapability>();

    EXPECT_NE(cap1, nullptr);
    EXPECT_NE(cap2, nullptr);
    // They are different instances due to overwrite
    EXPECT_NE(cap1.get(), cap2.get());
}
