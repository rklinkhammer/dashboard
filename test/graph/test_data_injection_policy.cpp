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
#include "app/policies/DataInjectionPolicy.hpp"
#include "app/capabilities/DataInjectionCapability.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <memory>

// ============================================================================
// Layer 4, Task 4.3: DataInjectionPolicy Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - DataInjectionPolicy Tests
// ============================================================================

class DataInjectionPolicyTest : public ::testing::Test {
protected:
    std::unique_ptr<app::capabilities::GraphCapability> graph_cap_;

    void SetUp() override {
        graph_cap_ = std::make_unique<app::capabilities::GraphCapability>();
    }
};

// ============================================================================
// DataInjectionPolicy Tests
// ============================================================================

TEST_F(DataInjectionPolicyTest, OnInit_CreatesAndRegistersDataInjectionCapability) {
    auto& bus = graph_cap_->GetCapabilityBus();
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();

    // Before init, DataInjectionCapability should not exist in bus
    EXPECT_FALSE(bus.Has<app::capabilities::DataInjectionCapability>());

    // Call OnInit
    EXPECT_TRUE(injection_policy->OnInit(*graph_cap_));

    // After init, DataInjectionCapability should be registered in bus
    EXPECT_TRUE(bus.Has<app::capabilities::DataInjectionCapability>());

    auto injection_cap = bus.Get<app::capabilities::DataInjectionCapability>();
    EXPECT_NE(injection_cap, nullptr);
}

TEST_F(DataInjectionPolicyTest, OnInit_Returns_True) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    bool result = injection_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(DataInjectionPolicyTest, OnStart_Returns_True) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    injection_policy->OnInit(*graph_cap_);
    bool result = injection_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(DataInjectionPolicyTest, OnStop_DoesNotCrash) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    injection_policy->OnInit(*graph_cap_);
    injection_policy->OnStart(*graph_cap_);

    // OnStop should not throw
    EXPECT_NO_THROW({
        injection_policy->OnStop(*graph_cap_);
    });
}

TEST_F(DataInjectionPolicyTest, OnJoin_DoesNotCrash) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    injection_policy->OnInit(*graph_cap_);
    injection_policy->OnStart(*graph_cap_);
    injection_policy->OnStop(*graph_cap_);

    // OnJoin should not throw
    EXPECT_NO_THROW({
        injection_policy->OnJoin(*graph_cap_);
    });
}

TEST_F(DataInjectionPolicyTest, FullLifecycle_InitStartStopJoin) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();

    // Test complete lifecycle
    EXPECT_TRUE(injection_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(injection_policy->OnStart(*graph_cap_));
    injection_policy->OnStop(*graph_cap_);
    injection_policy->OnJoin(*graph_cap_);

    // Should complete without crash
    auto& bus = graph_cap_->GetCapabilityBus();
    EXPECT_TRUE(bus.Has<app::capabilities::DataInjectionCapability>());
}

TEST_F(DataInjectionPolicyTest, DataInjectionCapability_IsSharedPtr) {
    auto injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    injection_policy->OnInit(*graph_cap_);

    auto& bus = graph_cap_->GetCapabilityBus();
    auto cap1 = bus.Get<app::capabilities::DataInjectionCapability>();
    auto cap2 = bus.Get<app::capabilities::DataInjectionCapability>();

    ASSERT_NE(cap1, nullptr);
    ASSERT_NE(cap2, nullptr);
    EXPECT_EQ(cap1.get(), cap2.get());
}

TEST_F(DataInjectionPolicyTest, MultipleDataInjectionPolicies_EachRegistersOwnCapability) {
    auto policy1 = std::make_unique<app::policies::DataInjectionPolicy>();
    auto policy2 = std::make_unique<app::policies::DataInjectionPolicy>();

    auto& bus = graph_cap_->GetCapabilityBus();

    policy1->OnInit(*graph_cap_);
    auto cap1 = bus.Get<app::capabilities::DataInjectionCapability>();

    // Second policy overwrites
    policy2->OnInit(*graph_cap_);
    auto cap2 = bus.Get<app::capabilities::DataInjectionCapability>();

    EXPECT_NE(cap1, nullptr);
    EXPECT_NE(cap2, nullptr);
    // They are different instances due to overwrite
    EXPECT_NE(cap1.get(), cap2.get());
}
