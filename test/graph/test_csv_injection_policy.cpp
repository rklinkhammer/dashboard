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
#include "app/policies/CSVInjectionPolicy.hpp"
#include "app/policies/DataInjectionPolicy.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <memory>
#include <filesystem>

// ============================================================================
// Layer 4, Task 4.4: CSVInjectionPolicy Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - CSVInjectionPolicy Tests
// ============================================================================

class CSVInjectionPolicyTest : public ::testing::Test {
protected:
    std::unique_ptr<app::capabilities::GraphCapability> graph_cap_;

    void SetUp() override {
        graph_cap_ = std::make_unique<app::capabilities::GraphCapability>();
    }
};

// ============================================================================
// CSVInjectionPolicy Tests
// ============================================================================

TEST_F(CSVInjectionPolicyTest, OnInit_RequiresDataInjectionCapability) {
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    // Without DataInjectionCapability registered, OnInit should fail
    bool result = csv_policy->OnInit(*graph_cap_);
    EXPECT_FALSE(result);
}

TEST_F(CSVInjectionPolicyTest, OnInit_SucceedsWithDataInjectionCapability) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    // First set up DataInjectionPolicy
    EXPECT_TRUE(data_injection_policy->OnInit(*graph_cap_));

    // Now CSVInjectionPolicy should succeed
    bool result = csv_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(CSVInjectionPolicyTest, OnInit_RegistersCSVDataInjectionCapability) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    auto& bus = graph_cap_->GetCapabilityBus();

    data_injection_policy->OnInit(*graph_cap_);

    EXPECT_FALSE(bus.Has<app::capabilities::CSVDataInjectionCapability>());

    csv_policy->OnInit(*graph_cap_);

    EXPECT_TRUE(bus.Has<app::capabilities::CSVDataInjectionCapability>());
}

TEST_F(CSVInjectionPolicyTest, OnStart_Returns_True) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    data_injection_policy->OnInit(*graph_cap_);
    csv_policy->OnInit(*graph_cap_);

    bool result = csv_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);

    // Must stop and join thread before test ends
    csv_policy->OnStop(*graph_cap_);
    csv_policy->OnJoin(*graph_cap_);
}

TEST_F(CSVInjectionPolicyTest, OnStop_DoesNotCrash) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    data_injection_policy->OnInit(*graph_cap_);
    csv_policy->OnInit(*graph_cap_);
    csv_policy->OnStart(*graph_cap_);

    EXPECT_NO_THROW({
        csv_policy->OnStop(*graph_cap_);
    });

    // Must join thread before test ends
    csv_policy->OnJoin(*graph_cap_);
}

TEST_F(CSVInjectionPolicyTest, OnJoin_DoesNotCrash) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    data_injection_policy->OnInit(*graph_cap_);
    csv_policy->OnInit(*graph_cap_);
    csv_policy->OnStart(*graph_cap_);
    csv_policy->OnStop(*graph_cap_);

    EXPECT_NO_THROW({
        csv_policy->OnJoin(*graph_cap_);
    });
}

TEST_F(CSVInjectionPolicyTest, FullLifecycle_InitStartStopJoin) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    data_injection_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(csv_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(csv_policy->OnStart(*graph_cap_));

    // Give injection thread time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    csv_policy->OnStop(*graph_cap_);
    csv_policy->OnJoin(*graph_cap_);

    auto& bus = graph_cap_->GetCapabilityBus();
    EXPECT_TRUE(bus.Has<app::capabilities::CSVDataInjectionCapability>());
}

TEST_F(CSVInjectionPolicyTest, CSVDataInjectionCapability_IsSharedPtr) {
    auto data_injection_policy = std::make_unique<app::policies::DataInjectionPolicy>();
    auto csv_policy = std::make_unique<app::policies::CSVInjectionPolicy>();

    data_injection_policy->OnInit(*graph_cap_);
    csv_policy->OnInit(*graph_cap_);

    auto& bus = graph_cap_->GetCapabilityBus();
    auto cap1 = bus.Get<app::capabilities::CSVDataInjectionCapability>();
    auto cap2 = bus.Get<app::capabilities::CSVDataInjectionCapability>();

    ASSERT_NE(cap1, nullptr);
    ASSERT_NE(cap2, nullptr);
    EXPECT_EQ(cap1.get(), cap2.get());
}
