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



namespace app::policies {

static auto csv_injection_logger_ = log4cxx::Logger::getLogger("app.policies.CSVInjectionPolicy");

class CSVInjectionPolicy : public graph::IExecutionPolicy {
public:
    CSVInjectionPolicy() {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy initialized");
    }   

    virtual ~CSVInjectionPolicy() = default;

    bool OnInit(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnInit called");
        // Initialize metrics system here if needed
        return true;
    }

    bool OnStart(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnStart called");
        // Start metrics collection here if needed
        return true;
    }

    void OnStop(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnStop called");
        // Stop metrics collection and cleanup here if needed
    }

    void OnJoin(graph::GraphExecutorContext &) override {
        LOG4CXX_TRACE(csv_injection_logger_, "CSVInjectionPolicy OnJoin called");
        // Finalize metrics reporting here if needed
    }   

    void SetCSVInputPaths(const std::vector<std::pair<std::string, std::string>>& paths) {
        csv_input_paths_ = paths;
        LOG4CXX_TRACE(csv_injection_logger_, "CSVDataInjectionPolicy::SetCSVInputPaths() - " 
                         << paths.size() << " paths set");
    }   

    private:

        std::vector<std::pair<std::string, std::string>> csv_input_paths_;

}; // class CSVInjectionPolicy
    
}// namespace app::policies