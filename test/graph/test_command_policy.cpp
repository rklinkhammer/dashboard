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
#include "app/policies/CommandPolicy.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/CapabilityBus.hpp"
#include <memory>
#include <chrono>
#include <thread>

// ============================================================================
// Layer 4, Task 4.5: CommandPolicy Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - CommandPolicy Tests
// ============================================================================

class CommandPolicyTest : public ::testing::Test {
protected:
    std::unique_ptr<app::capabilities::GraphCapability> graph_cap_;

    void SetUp() override {
        graph_cap_ = std::make_unique<app::capabilities::GraphCapability>();
    }
};

// ============================================================================
// CommandPolicy Tests
// ============================================================================

TEST_F(CommandPolicyTest, OnInit_Returns_True) {
    auto command_policy = std::make_unique<app::policies::CommandPolicy>();
    bool result = command_policy->OnInit(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(CommandPolicyTest, OnStart_Returns_True) {
    auto command_policy = std::make_unique<app::policies::CommandPolicy>();
    command_policy->OnInit(*graph_cap_);
    bool result = command_policy->OnStart(*graph_cap_);
    EXPECT_TRUE(result);
}

TEST_F(CommandPolicyTest, OnStop_DoesNotCrash) {
    auto command_policy = std::make_unique<app::policies::CommandPolicy>();
    command_policy->OnInit(*graph_cap_);
    command_policy->OnStart(*graph_cap_);

    EXPECT_NO_THROW({
        command_policy->OnStop(*graph_cap_);
    });
}

TEST_F(CommandPolicyTest, OnJoin_DoesNotCrash) {
    auto command_policy = std::make_unique<app::policies::CommandPolicy>();
    command_policy->OnInit(*graph_cap_);
    command_policy->OnStart(*graph_cap_);
    command_policy->OnStop(*graph_cap_);

    EXPECT_NO_THROW({
        command_policy->OnJoin(*graph_cap_);
    });
}

TEST_F(CommandPolicyTest, FullLifecycle_InitStartStopJoin) {
    auto command_policy = std::make_unique<app::policies::CommandPolicy>();
    EXPECT_TRUE(command_policy->OnInit(*graph_cap_));
    EXPECT_TRUE(command_policy->OnStart(*graph_cap_));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    command_policy->OnStop(*graph_cap_);
    command_policy->OnJoin(*graph_cap_);

    // Should complete without exception
}
