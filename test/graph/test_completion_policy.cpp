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
#include "app/policies/CompletionPolicy.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <memory>
#include <chrono>
#include <thread>

// ============================================================================
// Layer 4, Task 4.6: CompletionPolicy Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - CompletionPolicy Tests
// ============================================================================

class CompletionPolicyTest : public ::testing::Test {
protected:
    std::unique_ptr<app::capabilities::GraphCapability> graph_cap_;

    void SetUp() override {
        graph_cap_ = std::make_unique<app::capabilities::GraphCapability>();
    }
};

// ============================================================================
// CompletionPolicy Tests
// ============================================================================

TEST_F(CompletionPolicyTest, OnInit_Returns_True) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    bool result = completion_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(CompletionPolicyTest, OnStart_Returns_True) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    completion_policy->OnInit(*graph_cap_);
    bool result = completion_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(CompletionPolicyTest, OnStop_DoesNotCrash) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    completion_policy->OnInit(*graph_cap_);
    completion_policy->OnStart(*graph_cap_);

    EXPECT_NO_THROW({
        completion_policy->OnStop(*graph_cap_);
    });
}

TEST_F(CompletionPolicyTest, OnJoin_DoesNotCrash) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    completion_policy->OnInit(*graph_cap_);
    completion_policy->OnStart(*graph_cap_);
    completion_policy->OnStop(*graph_cap_);

    EXPECT_NO_THROW({
        completion_policy->OnJoin(*graph_cap_);
    });
}

TEST_F(CompletionPolicyTest, FullLifecycle_InitStartStopJoin) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    EXPECT_TRUE(completion_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(completion_policy->OnStart(*graph_cap_));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    completion_policy->OnStop(*graph_cap_);
    completion_policy->OnJoin(*graph_cap_);

    // Should complete without exception
}

TEST_F(CompletionPolicyTest, CanBeConstructedMultipleTimes) {
    auto policy1 = std::make_unique<app::policies::CompletionPolicy>();
    auto policy2 = std::make_unique<app::policies::CompletionPolicy>();

    EXPECT_TRUE(policy1->OnInit(*graph_cap_));
    EXPECT_TRUE(policy2->OnInit(*graph_cap_));
}

TEST_F(CompletionPolicyTest, OnInit_CanBeCalledMultipleTimes) {
    auto completion_policy = std::make_unique<app::policies::CompletionPolicy>();
    EXPECT_TRUE(completion_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(completion_policy->OnInit(*graph_cap_));
}
