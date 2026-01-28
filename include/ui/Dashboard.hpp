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
#include "ui/CommandRegistry.hpp"
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
              std::shared_ptr<app::capabilities::MetricsCapability> metrics_cap,
              std::shared_ptr<CommandRegistry> registry) 
        : metrics_panel(nullptr), log_window(nullptr), 
          command_window(nullptr), status_bar(nullptr),
          metrics_win(nullptr), log_win(nullptr), 
          cmd_win(nullptr), status_win(nullptr),
          last_update_(std::chrono::steady_clock::now()),
          graph_capability_(graph_cap),
          metrics_capability_(metrics_cap),
          registry_(registry) {}
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

    void SetCommandRegistry(std::shared_ptr<CommandRegistry> registry) { registry_ = registry; }

    MetricsPanel* GetMetricsPanel() const { return metrics_panel; }
    LogWindow* GetLoggingWindow() const { return log_window; }
    CommandWindow* GetCommandWindow() const { return command_window; }
    StatusBar* GetStatusBar() const { return status_bar; }
    bool GetFilterActive() const { return filter_active_; }
    std::string GetFilterPattern() const { return filter_pattern_; }
    void ClearFilter() { filter_pattern_.clear(); filter_active_ = false; }
    void SetFilterPattern(const std::string& pattern) { filter_pattern_ = pattern; filter_active_ = true; }
    void UpdateStatusBarWithFilter();

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
    std::shared_ptr<CommandRegistry> registry_;

    // Filter state
    std::string filter_pattern_;
    bool filter_active_ = false;

    // Helper methods
    bool SetupTerminal();
    bool CreateWindows();
    bool CreatePanels();

};

#endif // GDASHBOARD_DASHBOARD_HPP
