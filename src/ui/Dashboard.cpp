#include "ui/Dashboard.hpp"
#include <signal.h>
#include <locale.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>

bool Dashboard::Initialize() {
    if (!SetupTerminal()) return false;
    if (!CreateWindows()) return false;
    if (!CreatePanels()) return false;
    
    // Clear and refresh standard screen
    erase();
    refresh();
    
    // Initialize last_update for metric callback simulation
    last_update_ = std::chrono::steady_clock::now();
    
    if(metrics_capability_) {
        auto schemas = metrics_capability_->GetNodeMetricsSchemas();
        for(const auto& schema : schemas) {
            auto tile = NodeMetricsSchemaToTileWithMetrics(schema);
            AddMetricsTile(tile);
        }
        metrics_capability_->RegisterMetricsCallback(this);
    
    }
    status_bar->SetText("F1=Quit | Tab=Next Tab | Shift+Tab=Prev Tab | Arrows=Scroll");
    return true;
}

bool Dashboard::SetupTerminal() {
    // Set locale to UTF-8 for proper Unicode character support
    if(!setlocale(LC_ALL, "")) {       // Use system locale
        setlocale(LC_CTYPE, "en_US.UTF-8");  // Explicitly set UTF-8
    }
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    
    // Clear screen after initialization
    clear();
    refresh();

    getmaxyx(stdscr, term_h, term_w);
    
    // Minimum terminal size
    if (term_h < 10 || term_w < 40) {
        endwin();
        fprintf(stderr, "Terminal too small. Minimum: 10x40\n");
        return false;
    }

    return true;
}

bool Dashboard::CreateWindows() {
    // Calculate heights based on percentages
    // Metrics: 50%, Logs: 25%, Command: 20%, Status: 5%
    int metrics_h = std::max(3, term_h / 2);
    int log_h = std::max(3, term_h / 4);
    int cmd_h = std::max(3, (term_h * 20) / 100);
    int status_h = std::max(1, term_h - (metrics_h + log_h + cmd_h));

    metrics_win = newwin(metrics_h, term_w, 0, 0);
    log_win = newwin(log_h, term_w, metrics_h, 0);
    cmd_win = newwin(cmd_h, term_w, metrics_h + log_h, 0);
    status_win = newwin(status_h, term_w, metrics_h + log_h + cmd_h, 0);

    leaveok(metrics_win, TRUE);
    leaveok(log_win, TRUE);
    leaveok(status_win, TRUE);

    // Command window should control the cursor position
    leaveok(cmd_win, FALSE);

    return (metrics_win && log_win && cmd_win && status_win);
}

bool Dashboard::CreatePanels() {
    int metrics_h = term_h / 2;
    int log_h = term_h / 4;
    int cmd_h = (term_h * 20) / 100;

    metrics_panel = new MetricsPanel(metrics_win, term_w, metrics_h);
    log_window = new LogWindow(log_win, term_w, log_h);
    command_window = new CommandWindow(cmd_win, term_w, cmd_h);
    status_bar = new StatusBar(status_win, term_w, 1);

    return (metrics_panel && log_window && command_window && status_bar);
}

void Dashboard::AddMetricsTile(const NodeMetricsTile& tile) {
    if (metrics_panel) {
        metrics_panel->AddTile(tile);
    }
}

void Dashboard::AddLog(const std::string& line) {
    if (log_window) {
        log_window->AddLog(line);
    }
}

void Dashboard::OnMetricsEvent(const app::metrics::MetricsEvent& event) {
    // Find or create tile for this source
    NodeMetricsTile* tile = nullptr;
    if (metrics_panel) {
        for (auto& t : metrics_panel->tiles) {
            if (t.name == event.source) {
                tile = &t;
                break;
            }
        }
        
        if (!tile) {
            // Create new tile for this source
            NodeMetricsTile new_tile(event.source);
            metrics_panel->AddTile(new_tile);
            tile = &metrics_panel->tiles.back();
        }
    }
    
    if (!tile) return;
    
    // Record the event type
    tile->RecordEvent(event.event_type);
    
    // Update metrics from event data
    for (const auto& [key, value_str] : event.data) {
        // Find or create metric
        Metric* metric = nullptr;
        for (auto& m : tile->metrics) {
            if (m.name == key) {
                metric = &m;
                break;
            }
        }
        
        if (!metric) {
            // Create new metric
            Metric new_metric;
            new_metric.name = key;
            new_metric.event_type = event.event_type;
            tile->metrics.push_back(new_metric);
            metric = &tile->metrics.back();
        }
        
        // Parse and update value
        metric->last_update = event.timestamp;
        metric->event_type = event.event_type;
        
        try {
            // Try to parse as different types
            if (value_str == "true" || value_str == "false") {
                metric->type = MetricType::BOOL;
                metric->value = (value_str == "true");
                metric->AddToHistory((value_str == "true") ? 1.0 : 0.0);
            } else if (value_str.find('.') != std::string::npos) {
                metric->type = MetricType::FLOAT;
                double numeric_val = std::stod(value_str);
                metric->value = numeric_val;
                metric->AddToHistory(numeric_val);
                
                // Extract confidence if metric name contains "confidence"
                if (key.find("confidence") != std::string::npos) {
                    metric->confidence = numeric_val;
                    // Auto-set alert level based on confidence
                    if (metric->confidence < 0.2) {
                        metric->alert_level = 2;  // critical
                    } else if (metric->confidence < 0.5) {
                        metric->alert_level = 1;  // warning
                    } else {
                        metric->alert_level = 0;  // ok
                    }
                }
            } else {
                // Try int first, otherwise string
                try {
                    metric->type = MetricType::INT;
                    int int_val = std::stoi(value_str);
                    metric->value = int_val;
                    metric->AddToHistory(static_cast<double>(int_val));
                } catch (...) {
                    metric->type = MetricType::STRING;
                    metric->value = value_str;
                    // Try to parse as number for history
                    try {
                        metric->AddToHistory(std::stod(value_str));
                    } catch (...) {
                        // Skip history for non-numeric strings
                    }
                }
            }
        } catch (...) {
            // Fallback to string
            metric->type = MetricType::STRING;
            metric->value = value_str;
        }
    }
}

void Dashboard::HandleInput(int ch) {
    if (!command_window) return;
    if (ch == ERR) return;

    switch (ch) {
        case '\t':
            if (metrics_panel) metrics_panel->NextTab();
            break;
        case KEY_BTAB:
            if (metrics_panel) metrics_panel->PrevTab();
            break;
        case KEY_UP:
            if (metrics_panel) metrics_panel->ScrollUp();
            if (log_window) log_window->ScrollUp();
            break;
        case KEY_DOWN:
            if (metrics_panel) metrics_panel->ScrollDown();
            if (log_window) log_window->ScrollDown();
            break;
        case 10: // Enter - Execute command
            {
                std::string cmd = command_window->GetCommand();
                if (!cmd.empty()) {
                    AddLog("> " + cmd);
                    command_window->AddHistory(cmd);
                    ExecuteCommand(cmd);
                    command_window->ClearBuffer();
                }
            }
            break;
        case KEY_BACKSPACE:
        case 127:
            command_window->DeleteChar();
            break;
        default:
            if (isprint(ch)) {
                command_window->AddChar(ch);
            }
            break;
    }
}

void Dashboard::ExecuteCommand(const std::string& cmd) {
    // Split input into command and arguments
    std::istringstream iss(cmd);
    std::string command_name;
    iss >> command_name;
    
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
        
    if (command_name == "help") {
        if (!registry_) {
            AddLog("[ERROR] Command registry not initialized");
            return;
        }
        registry_->GenerateHelpText(this);
        return;
    }
    
    // Execute via registry if available
    if (!registry_) {
        AddLog("[ERROR] Command registry not initialized");
        return;
    }
    
    CommandResult result = registry_->ExecuteCommand(command_name, args);
    if (!result.success) {
        AddLog("[ERROR] " + result.message);
    }
}

void Dashboard::UpdateStatusBarWithFilter() {
    std::string status_text = "F1=Quit | Tab=Next Tab | Shift+Tab=Prev Tab | Arrows=Scroll";
    if (filter_active_) {
        status_text += " | Filter: " + filter_pattern_;
    }
    if (status_bar) {
        status_bar->SetText(status_text);
    }
}

void Dashboard::Render() {
    if (metrics_panel) metrics_panel->Render();
    if (log_window) log_window->Render();
    if (command_window) command_window->Render();
    if (status_bar) status_bar->Render();
}

void Dashboard::HandleResize() {
    endwin();
    refresh();
    clear();

    int new_h, new_w;
    getmaxyx(stdscr, new_h, new_w);
    term_h = new_h;
    term_w = new_w;

    int metrics_h = term_h / 2;
    int log_h = term_h / 4;
    int cmd_h = (term_h * 20) / 100;
    int status_h = std::max(1, term_h - (metrics_h + log_h + cmd_h));

    if (metrics_panel) metrics_panel->Resize(metrics_h, term_w, 0);
    if (log_window) log_window->Resize(log_h, term_w, metrics_h);
    if (command_window) command_window->Resize(cmd_h, term_w, metrics_h + log_h);
    if (status_bar) status_bar->Resize(status_h, term_w, metrics_h + log_h + cmd_h);

    Render();
}

void Dashboard::Cleanup() {
    delete metrics_panel;
    delete log_window;
    delete command_window;
    delete status_bar;

    if (metrics_win) delwin(metrics_win);
    if (log_win) delwin(log_win);
    if (cmd_win) delwin(cmd_win);
    if (status_win) delwin(status_win);

    endwin();
}

void Dashboard::Run() {
    int ch;
    while ((ch = getch()) != KEY_F(1)) { // F1 to quit
        
        HandleInput(ch);
        
        // Check for resize
        if (ch == KEY_RESIZE) {
            HandleResize();
        }

        Render();
    }
}
