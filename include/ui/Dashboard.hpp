#ifndef GDASHBOARD_DASHBOARD_HPP
#define GDASHBOARD_DASHBOARD_HPP

#include <ncurses.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>

#include "ui/MetricsPanel.hpp"
#include "ui/LogWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include "app/metrics/MetricsEvent.hpp"
#include "app/metrics/IMetricsSubscriber.hpp"

// Using declarations for production types

// ============================================================================
// Dashboard Manager - Coordinates all panels and layout
// ============================================================================
class Dashboard : public app::metrics::IMetricsSubscriber {

public:
    Dashboard(std::shared_ptr<app::capabilities::GraphCapability> graph_cap,
              std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap) 
        : metrics_panel(nullptr), log_window(nullptr), 
          command_window(nullptr), status_bar(nullptr),
          metrics_win(nullptr), log_win(nullptr), 
          cmd_win(nullptr), status_win(nullptr),
          last_update_(std::chrono::steady_clock::now()),
          graph_capability_(graph_cap),
          metrics_capability_(metrics_cap) {}
    ~Dashboard() {
        Cleanup();
    }

    bool Initialize();
    
    void OnMetricsEvent(const app::metrics::MetricsEvent& event);
    void HandleInput(int ch);
    void ExecuteCommand(const std::string& cmd);
    void Render();
    void HandleResize();
    void Cleanup();
    void Run();

    void AddMetricsTile(const NodeMetricsTile& tile);
    void AddLog(const std::string& line);

private:
    MetricsPanel* metrics_panel;
    LogWindow* log_window;
    CommandWindow* command_window;
    StatusBar* status_bar;

    WINDOW* metrics_win;
    WINDOW* log_win;
    WINDOW* cmd_win;
    WINDOW* status_win;

    int term_h = 0;
    int term_w = 0;
    
    // Real-time update mechanism
    std::chrono::steady_clock::time_point last_update_;
    
    std::shared_ptr<app::capabilities::GraphCapability> graph_capability_;
    std::shared_ptr<app::capabilities::MetricsCapability> metrics_capability_;
    
    // Filter state
    std::string filter_pattern_;
    bool filter_active_ = false;

    // Helper methods
    bool SetupTerminal();
    bool CreateWindows();
    bool CreatePanels();
    void UpdateStatusBarWithFilter();

};

#endif // GDASHBOARD_DASHBOARD_HPP
