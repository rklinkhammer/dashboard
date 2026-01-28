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
#include <log4cxx/logger.h>
#include "graph/IExecutionPolicy.hpp"
#include "graph/GraphExecutorContext.hpp"

namespace app::policies
{

    static auto completion_logger = log4cxx::Logger::getLogger("app.policies.CompletionPolicy");

    class CompletionPolicy : public graph::IExecutionPolicy
    {
    public:
        CompletionPolicy()
        {
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy initialized");
        }

        virtual ~CompletionPolicy() = default;

        bool OnInit(graph::GraphExecutorContext &context) override
        {
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy OnInit called");
            // Initialize metrics system here if needed
            InitCompletionCallbacks(context);
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy OnInit completed");            
            return true;
        }

        bool OnStart(graph::GraphExecutorContext &context) override
        {
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy OnStart called");

            auto fn = [this, &context]()
            {
                LOG4CXX_TRACE(completion_logger, "CompletionPolicy::OnRun() - waiting for completion signal or timeout");
                std::unique_lock lock(completion_mutex_);
                if (!completion_signaled_)
                {
                    auto completed = completion_cv_.wait_for(lock, max_duration_);
                    LOG4CXX_TRACE(completion_logger, "CompletionPolicy::OnRun() - wait completed, signaled=" 
                        << (completed == std::cv_status::no_timeout));
                }
                else
                {
                    LOG4CXX_TRACE(completion_logger, "CompletionPolicy::OnRun() - completion already signaled");
                }
                context.SetStopped();
                LOG4CXX_TRACE(completion_logger, "CompletionPolicy::OnRun() - signaling shutdown");
            };
            completion_thread_ = std::thread(fn);
            return true;
        }

        void OnStop(graph::GraphExecutorContext &) override
        {
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy OnStop called");
            // Stop metrics collection and cleanup here if needed
        }

        void OnJoin(graph::GraphExecutorContext &) override
        {
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy OnJoin called");
            if (completion_thread_.joinable()) {
                completion_thread_.join();
            }   
            // Finalize metrics reporting here if needed
        }

        void SetMaxDuration(std::chrono::milliseconds duration)
        {
            max_duration_ = duration;
            LOG4CXX_TRACE(completion_logger, "CompletionPolicy max duration set to "
                                                 << max_duration_.count() << " ms");
        }

    private:
        bool InitCompletionCallbacks(graph::GraphExecutorContext &context);

        std::thread completion_thread_;
        std::condition_variable completion_cv_;
        std::mutex completion_mutex_;
        bool completion_signaled_ = false;
        std::chrono::milliseconds max_duration_;
        std::vector<std::shared_ptr<void>> completion_callbacks_;

    }; // class CompletionPolicy

} // namespace app::policies