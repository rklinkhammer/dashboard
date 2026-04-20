// MIT License
//
// Copyright (c) 2026 gdashboard contributors
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
#include "graph/IExecutionPolicy.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include <memory>
#include <vector>
#include <atomic>

// ============================================================================
// Layer 3, Task 3.2: ExecutionPolicyChain Unit Tests
// Reference: doc/UNIT_TEST_STRATEGY.md - Execution Policy Chain
// ============================================================================

namespace {

// ============================================================================
// Mock Policy Implementations
// ============================================================================

class InvocationTrackingPolicy : public graph::IExecutionPolicy {
public:
    explicit InvocationTrackingPolicy(const std::string& name,
                                       std::vector<std::string>* invocation_log)
        : name_(name), invocation_log_(invocation_log) {}

    bool OnInit(app::capabilities::GraphCapability& context) override {
        (void)context;
        if (invocation_log_) {
            invocation_log_->push_back(name_ + ".OnInit");
        }
        return should_succeed_init_;
    }

    bool OnStart(app::capabilities::GraphCapability& context) override {
        (void)context;
        if (invocation_log_) {
            invocation_log_->push_back(name_ + ".OnStart");
        }
        return should_succeed_start_;
    }

    bool OnRun(app::capabilities::GraphCapability& context) override {
        (void)context;
        if (invocation_log_) {
            invocation_log_->push_back(name_ + ".OnRun");
        }
        return should_succeed_run_;
    }

    void OnStop(app::capabilities::GraphCapability& context) override {
        (void)context;
        if (invocation_log_) {
            invocation_log_->push_back(name_ + ".OnStop");
        }
    }

    void OnJoin(app::capabilities::GraphCapability& context) override {
        (void)context;
        if (invocation_log_) {
            invocation_log_->push_back(name_ + ".OnJoin");
        }
    }

    // Test control
    void SetSucceed(bool init, bool start, bool run) {
        should_succeed_init_ = init;
        should_succeed_start_ = start;
        should_succeed_run_ = run;
    }

private:
    std::string name_;
    std::vector<std::string>* invocation_log_;
    bool should_succeed_init_ = true;
    bool should_succeed_start_ = true;
    bool should_succeed_run_ = true;
};

} // namespace

// ============================================================================
// Test Fixture for ExecutionPolicyChain
// ============================================================================

class ExecutionPolicyChainTest : public ::testing::Test {
protected:
    void SetUp() override {
        capability_ctx_ = std::make_unique<app::capabilities::GraphCapability>();
        invocation_order_.clear();
    }

    std::vector<std::string> invocation_order_;
    std::unique_ptr<app::capabilities::GraphCapability> capability_ctx_;

    // Helper: Create chain of policies with given names
    std::unique_ptr<graph::ExecutionPolicyChain> CreatePolicyChain(
        const std::vector<std::string>& policy_names) {
        if (policy_names.empty()) {
            return nullptr;
        }

        auto first_policy = std::make_unique<InvocationTrackingPolicy>(
            policy_names[0], &invocation_order_);

        auto chain =
            std::make_unique<graph::ExecutionPolicyChain>(std::move(first_policy));

        for (size_t i = 1; i < policy_names.size(); ++i) {
            auto policy = std::make_unique<InvocationTrackingPolicy>(
                policy_names[i], &invocation_order_);
            auto next_chain =
                std::make_unique<graph::ExecutionPolicyChain>(std::move(policy));
            chain->AppendNext(std::move(next_chain));
        }

        return chain;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

TEST_F(ExecutionPolicyChainTest, InvokePoliciesInOrder) {
    // Create chain with 3 policies
    auto chain = CreatePolicyChain({"Policy1", "Policy2", "Policy3"});

    // Execute full lifecycle
    EXPECT_TRUE(chain->OnInit(*capability_ctx_));
    EXPECT_TRUE(chain->OnStart(*capability_ctx_));
    EXPECT_TRUE(chain->OnRun(*capability_ctx_));
    chain->OnStop(*capability_ctx_);
    chain->OnJoin(*capability_ctx_);

    // Verify order: each policy's OnXxx called in sequence
    std::vector<std::string> expected = {
        "Policy1.OnInit", "Policy2.OnInit", "Policy3.OnInit",
        "Policy1.OnStart", "Policy2.OnStart", "Policy3.OnStart",
        "Policy1.OnRun", "Policy2.OnRun", "Policy3.OnRun",
        "Policy1.OnStop", "Policy2.OnStop", "Policy3.OnStop",
        "Policy1.OnJoin", "Policy2.OnJoin", "Policy3.OnJoin"};

    EXPECT_EQ(invocation_order_, expected);
}

TEST_F(ExecutionPolicyChainTest, ChainMultiplePolicies) {
    // Create chain with 5 policies
    auto chain = CreatePolicyChain(
        {"P1", "P2", "P3", "P4", "P5"});

    // Execute full lifecycle
    EXPECT_TRUE(chain->OnInit(*capability_ctx_));
    EXPECT_TRUE(chain->OnStart(*capability_ctx_));
    EXPECT_TRUE(chain->OnRun(*capability_ctx_));
    chain->OnStop(*capability_ctx_);
    chain->OnJoin(*capability_ctx_);

    // Verify all 5 policies invoked in each phase
    int init_count = 0, start_count = 0, run_count = 0, stop_count = 0, join_count = 0;

    for (const auto& call : invocation_order_) {
        if (call.find(".OnInit") != std::string::npos) ++init_count;
        if (call.find(".OnStart") != std::string::npos) ++start_count;
        if (call.find(".OnRun") != std::string::npos) ++run_count;
        if (call.find(".OnStop") != std::string::npos) ++stop_count;
        if (call.find(".OnJoin") != std::string::npos) ++join_count;
    }

    EXPECT_EQ(init_count, 5);
    EXPECT_EQ(start_count, 5);
    EXPECT_EQ(run_count, 5);
    EXPECT_EQ(stop_count, 5);
    EXPECT_EQ(join_count, 5);
}

TEST_F(ExecutionPolicyChainTest, HandlePolicyErrors) {
    // Create chain with 3 policies
    auto chain = CreatePolicyChain({"Policy1", "Policy2", "Policy3"});

    // OnInit succeeds for all
    EXPECT_TRUE(chain->OnInit(*capability_ctx_));
    EXPECT_EQ(invocation_order_.size(), 3); // All 3 OnInit called

    // Verify chain continues if all policies succeed
    invocation_order_.clear();
    EXPECT_TRUE(chain->OnStart(*capability_ctx_));
    EXPECT_EQ(invocation_order_.size(), 3); // All 3 OnStart called

    // Verify OnStop/OnJoin are still called
    invocation_order_.clear();
    chain->OnStop(*capability_ctx_);
    chain->OnJoin(*capability_ctx_);

    // All 3 should have OnStop/OnJoin called
    int stop_count = 0, join_count = 0;
    for (const auto& call : invocation_order_) {
        if (call.find(".OnStop") != std::string::npos) ++stop_count;
        if (call.find(".OnJoin") != std::string::npos) ++join_count;
    }
    EXPECT_EQ(stop_count, 3);
    EXPECT_EQ(join_count, 3);
}

TEST_F(ExecutionPolicyChainTest, StateManagement) {
    // Create chain with 2 policies
    auto chain = CreatePolicyChain({"StatePolicy1", "StatePolicy2"});

    // Verify context state is consistent through lifecycle
    EXPECT_FALSE(capability_ctx_->IsStopped());

    // OnInit phase
    EXPECT_TRUE(chain->OnInit(*capability_ctx_));
    EXPECT_FALSE(capability_ctx_->IsStopped());

    // OnStart phase
    EXPECT_TRUE(chain->OnStart(*capability_ctx_));
    EXPECT_FALSE(capability_ctx_->IsStopped());

    // OnRun phase
    EXPECT_TRUE(chain->OnRun(*capability_ctx_));
    EXPECT_FALSE(capability_ctx_->IsStopped());

    // OnStop phase - set stopped flag
    capability_ctx_->SetStopped();
    chain->OnStop(*capability_ctx_);
    EXPECT_TRUE(capability_ctx_->IsStopped());

    // OnJoin phase - still stopped
    chain->OnJoin(*capability_ctx_);
    EXPECT_TRUE(capability_ctx_->IsStopped());

    // Verify all lifecycle methods were called
    EXPECT_EQ(invocation_order_.size(), 10); // 5 phases × 2 policies
}

TEST_F(ExecutionPolicyChainTest, ContextPassedToAllPolicies) {
    // Verify that the same context object is passed to all policies
    auto chain = CreatePolicyChain({"Policy1", "Policy2"});

    // Set some state on context
    capability_ctx_->SetCliMode(true);
    capability_ctx_->SetNodeNames({"Node1", "Node2", "Node3"});

    // Execute lifecycle - policies can access the context
    EXPECT_TRUE(chain->OnInit(*capability_ctx_));
    EXPECT_TRUE(chain->OnStart(*capability_ctx_));

    // Verify state was preserved
    EXPECT_TRUE(capability_ctx_->IsCliMode());
    EXPECT_EQ(capability_ctx_->GetNodeNames().size(), 3);
}

