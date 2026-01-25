#pragma once

#include <string>

class StatusBar {
public:
    StatusBar();

    void SetStatus(const std::string& status) { status_ = status; }
    void SetNodeCount(int active, int total) {
        active_nodes_ = active;
        total_nodes_ = total;
    }
    void SetErrorCount(int errors) { error_count_ = errors; }

    std::string GetStatus() const { return status_; }
    int GetActiveNodes() const { return active_nodes_; }
    int GetTotalNodes() const { return total_nodes_; }
    int GetErrorCount() const { return error_count_; }

private:
    std::string status_ = "READY";
    int active_nodes_ = 0;
    int total_nodes_ = 0;
    int error_count_ = 0;
};
