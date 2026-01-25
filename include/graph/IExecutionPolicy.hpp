
#pragma once


#include <memory>
#include "app/AppContext.hpp"


struct IExecutionPolicy {
    virtual ~IExecutionPolicy() = default;

    virtual bool PreInit(app::AppContext& context) { (void)context; return true; }
    virtual bool OnInit(app::AppContext& context) { (void)context; return true; }
    virtual bool OnStart(app::AppContext& context) { (void)context; return true; }
    virtual bool OnRun(app::AppContext& context) { (void)context; return true; }
    virtual void OnStop(app::AppContext& context) { (void)context; }
    virtual void OnJoin(app::AppContext& context) { (void)context; }
};

class ExecutionPolicyChain : public IExecutionPolicy {
public:
    explicit ExecutionPolicyChain(std::unique_ptr<IExecutionPolicy> policy,
                                  std::unique_ptr<ExecutionPolicyChain> next = nullptr)
        : policy_(std::move(policy)), next_(std::move(next)) {}

    bool PreInit(app::AppContext& ctx) override {
        if (!policy_->PreInit(ctx)) return false;
        return next_ ? next_->PreInit(ctx) : true;
    }   

    bool OnInit(app::AppContext& ctx) override {
        if (!policy_->OnInit(ctx)) return false;
        return next_ ? next_->OnInit(ctx) : true;
    }

    bool OnStart(app::AppContext& ctx) override {
        if (!policy_->OnStart(ctx)) return false;
        return next_ ? next_->OnStart(ctx) : true;
    }

    bool OnRun(app::AppContext& ctx) override {
        if (!policy_->OnRun(ctx)) return false;
        return next_ ? next_->OnRun(ctx) : true;
    }

    void OnStop(app::AppContext& ctx) override {
        policy_->OnStop(ctx);
        if (next_) next_->OnStop(ctx);
    }

    void OnJoin(app::AppContext& ctx) override {
        policy_->OnJoin(ctx);
        if (next_) next_->OnJoin(ctx);
    }

    /// @brief Append a policy to the end of the chain
    /// @param policy The execution policy to append
    void AppendPolicy(std::unique_ptr<IExecutionPolicy> policy) {
        if (!next_) {
            next_ = std::make_unique<ExecutionPolicyChain>(std::move(policy));
        } else {
            next_->AppendPolicy(std::move(policy));
        }
    }

    /// @brief Append a policy chain to the end of this chain
    /// @param next_chain The policy chain to append
    void AppendNext(std::unique_ptr<ExecutionPolicyChain> next_chain) {
        if (!next_) {
            next_ = std::move(next_chain);
        } else {
            next_->AppendNext(std::move(next_chain));
        }
    }

private:
    std::unique_ptr<IExecutionPolicy> policy_;
    std::unique_ptr<ExecutionPolicyChain> next_;
};