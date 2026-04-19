#pragma once

#include <string>
#include <memory>
#include <functional>
#include "core/ActiveQueue.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/CommandRegistry.hpp"

class Dashboard;
namespace app::capabilities {

class DashboardCapability {
public:
    DashboardCapability() = default;
    virtual ~DashboardCapability() = default;
 
    bool EnqueueCommand(const std::string& command) {
        return command_queue_.Enqueue(command);
    }

    bool DequeueCommand(std::string& command) {
        return command_queue_.Dequeue(command);
    }

    bool AddLog(const std::string& line) {
        // Placeholder: In a real implementation, this might log to a file or UI
        return log_queue_.Enqueue(line);
    }   

    void DisableLogQueue() {
        log_queue_.Disable();
    }

    bool DequeueLogQueue(std::string& line) {
        return log_queue_.Dequeue(line);
    }
    
    void DisableCommandQueue() {
        command_queue_.Disable();
    }   

    void SetDashboard(std::shared_ptr<Dashboard> dashboard) {
        dashboard_ = dashboard;
    }
    
    std::shared_ptr<Dashboard> GetDashboard() const {
        return dashboard_;
    }

private:
    core::ActiveQueue<std::string> command_queue_;
    core::ActiveQueue<std::string> log_queue_;
    std::shared_ptr<Dashboard> dashboard_;
};

}  // namespace app::capabilities
