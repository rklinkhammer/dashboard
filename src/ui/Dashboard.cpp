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
    std::istringstream iss(cmd);
    std::string command;
    iss >> command;
    
    if (command == "filter_metrics") {
        std::string pattern;
        if (iss >> pattern) {
            if (metrics_panel) {
                metrics_panel->SetFilterPattern(pattern);
                filter_pattern_ = pattern;
                filter_active_ = true;
                AddLog("Filter applied: " + pattern);
                UpdateStatusBarWithFilter();
            }
        } else {
            AddLog("Usage: filter_metrics <pattern>");
        }
    } else if (command == "clear_filter" || command == "clearfilter") {
        if (metrics_panel) {
            metrics_panel->ClearFilter();
            filter_pattern_.clear();
            filter_active_ = false;
            AddLog("Filter cleared");
            UpdateStatusBarWithFilter();
        }
    } else if (command == "toggle_sparklines") {
        if (metrics_panel) {
            metrics_panel->ToggleSparklines();
            std::string status = metrics_panel->AreSparklesEnabled() ? "enabled" : "disabled";
            AddLog("Sparklines " + status);
        } else {
            AddLog("No metrics panel available");
        }
    } else if (command == "status") {
        AddLog("Dashboard Status:");
        if (metrics_panel) {
            AddLog("  Tiles: " + std::to_string(metrics_panel->GetTileCount()));
            if (filter_active_) {
                AddLog("  Filter: " + filter_pattern_);
            } else {
                AddLog("  Filter: None");
            }
        }
    } else if (command == "help") {
        AddLog("Commands:");
        AddLog("  filter_metrics <pattern> - Show only metrics matching pattern");
        AddLog("  clear_filter - Show all metrics");
        AddLog("  list_metrics [node] - List all metrics (optionally filtered by node)");
        AddLog("  show_events - Display event type counts and statistics");
        AddLog("  show_history <node>::<metric> - Display metric history with statistics");
        AddLog("  set_metric <node>::<metric> <value> - Manually update a metric");
        AddLog("  clear_metrics - Reset all metrics");
        AddLog("  export_metrics [json|csv] - Export metrics (default: json)");
        AddLog("  set_confidence <node> <metric> <value> - Set confidence (0.0-1.0)");
        AddLog("  toggle_sparklines - Show/hide sparkline trends (Unicode by default)");
        AddLog("  status - Display dashboard state");
        AddLog("  help - Show this help");
    } else if (command == "set_confidence") {
        std::string node_name, metric_name, value_str;
        if (iss >> node_name >> metric_name >> value_str) {
            try {
                double conf_value = std::stod(value_str);
                
                // Clamp to 0.0-1.0 range
                if (conf_value < 0.0) conf_value = 0.0;
                if (conf_value > 1.0) conf_value = 1.0;
                
                // Find and update metric
                bool found = false;
                if (metrics_panel) {
                    for (auto& tile : metrics_panel->tiles) {
                        if (tile.name == node_name) {
                            for (auto& metric : tile.metrics) {
                                if (metric.name == metric_name) {
                                    metric.confidence = conf_value;
                                    // Update alert level based on new confidence
                                    if (conf_value < 0.2) {
                                        metric.alert_level = 2;  // critical
                                    } else if (conf_value < 0.5) {
                                        metric.alert_level = 1;  // warning
                                    } else {
                                        metric.alert_level = 0;  // ok
                                    }
                                    found = true;
                                    AddLog("Confidence updated: " + node_name + "::" + metric_name + 
                                           " = " + std::to_string((int)(conf_value * 100)) + "%");
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                if (!found) {
                    AddLog("Metric not found: " + node_name + "::" + metric_name);
                }
            } catch (...) {
                AddLog("Invalid confidence value: " + value_str + " (use 0.0-1.0)");
            }
        } else {
            AddLog("Usage: set_confidence <node> <metric> <value>");
            AddLog("  Example: set_confidence EstimationPipelineNode altitude_confidence 0.75");
        }
    } else if (command == "list_metrics" || command == "ls") {
        // List all metrics for current or specified node
        std::string node_filter;
        iss >> node_filter;  // Optional node filter
        
        bool found_any = false;
        if (metrics_panel) {
            for (const auto& tile : metrics_panel->tiles) {
                if (!node_filter.empty() && tile.name.find(node_filter) == std::string::npos) {
                    continue;  // Skip if filter doesn't match
                }
                
                AddLog(tile.name + " (" + std::to_string(tile.metrics.size()) + " metrics):");
                for (const auto& metric : tile.metrics) {
                    std::string type_str;
                    switch (metric.type) {
                        case MetricType::INT: type_str = "int"; break;
                        case MetricType::FLOAT: type_str = "float"; break;
                        case MetricType::BOOL: type_str = "bool"; break;
                        case MetricType::STRING: type_str = "string"; break;
                    }
                    AddLog("  " + metric.name + " (" + type_str + "): " + metric.GetFormattedValue());
                }
                found_any = true;
            }
        }
        if (!found_any) {
            AddLog("No metrics found" + (node_filter.empty() ? "" : " matching: " + node_filter));
        }
    } else if (command == "show_events") {
        // Show event type counts and statistics
        if (metrics_panel) {
            AddLog("Event Statistics:");
            int total_events = 0;
            for (const auto& tile : metrics_panel->tiles) {
                if (!tile.event_type_counts.empty()) {
                    AddLog(tile.name + ":");
                    for (const auto& [evt_type, count] : tile.event_type_counts) {
                        AddLog("  " + evt_type + ": " + std::to_string(count) + " updates");
                        total_events += count;
                    }
                }
            }
            AddLog("Total events: " + std::to_string(total_events));
        } else {
            AddLog("No metrics available");
        }
    } else if (command == "show_history") {
        // Display metric history with statistics
        std::string path_str;
        if (!(iss >> path_str)) {
            AddLog("Usage: show_history <node>::<metric>");
            AddLog("  Example: show_history EstimationPipelineNode::altitude_estimate_m");
        } else {
            // Parse node::metric path
            size_t delim = path_str.find("::");
            if (delim == std::string::npos) {
                AddLog("Error: Use format node::metric (e.g., EstimationPipelineNode::altitude_estimate_m)");
            } else {
                std::string node_name = path_str.substr(0, delim);
                std::string metric_name = path_str.substr(delim + 2);
                
                bool found = false;
                if (metrics_panel) {
                    for (auto& tile : metrics_panel->tiles) {
                        if (tile.name == node_name) {
                            for (auto& metric : tile.metrics) {
                                if (metric.name == metric_name) {
                                    found = true;
                                    
                                    // Display history information
                                    AddLog("History for " + node_name + "::" + metric_name);
                                    
                                    if (metric.GetHistorySize() == 0) {
                                        AddLog("  No history data yet");
                                    } else {
                                        AddLog("  Current value: " + metric.GetFormattedValue());
                                        AddLog("  History entries: " + std::to_string(metric.GetHistorySize()) + "/" + 
                                               std::to_string(Metric::HISTORY_SIZE));
                                        
                                        // Display statistics for numeric metrics
                                        if (metric.type == MetricType::INT || metric.type == MetricType::FLOAT) {
                                            double min_val = metric.GetHistoryMin();
                                            double max_val = metric.GetHistoryMax();
                                            double avg_val = metric.GetHistoryAvg();
                                            
                                            std::ostringstream oss;
                                            oss << std::fixed << std::setprecision(3);
                                            
                                            AddLog("  Min: " + oss.str() + std::to_string(min_val) + 
                                                   (metric.unit.empty() ? "" : " " + metric.unit));
                                            
                                            oss.str("");
                                            oss.clear();
                                            oss << std::fixed << std::setprecision(3) << max_val;
                                            AddLog("  Max: " + oss.str() + 
                                                   (metric.unit.empty() ? "" : " " + metric.unit));
                                            
                                            oss.str("");
                                            oss.clear();
                                            oss << std::fixed << std::setprecision(3) << avg_val;
                                            AddLog("  Avg: " + oss.str() + 
                                                   (metric.unit.empty() ? "" : " " + metric.unit));
                                            
                                            // Simple trend indicator
                                            if (metric.value_history.size() >= 2) {
                                                double first = metric.value_history.front();
                                                double last = metric.value_history.back();
                                                if (last > first * 1.01) {
                                                    AddLog("  Trend: /\\ Rising");
                                                } else if (last < first * 0.99) {
                                                    AddLog("  Trend: \\/ Falling");
                                                } else {
                                                    AddLog("  Trend: -- Stable");
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                if (!found) {
                    AddLog("Metric not found: " + node_name + "::" + metric_name);
                }
            }
        }
    } else if (command == "set_metric") {
        // Manual metric update: set_metric <node>::<metric> <value>
        std::string path_str, value_str;
        if (!(iss >> path_str >> value_str)) {
            AddLog("Usage: set_metric <node>::<metric> <value>");
            AddLog("  Example: set_metric EstimationPipelineNode::altitude_estimate_m 1550.5");
        } else {
            // Parse node::metric path
            size_t delim = path_str.find("::");
            if (delim == std::string::npos) {
                AddLog("Error: Use format node::metric (e.g., EstimationPipelineNode::altitude_estimate_m)");
            } else {
                std::string node_name = path_str.substr(0, delim);
                std::string metric_name = path_str.substr(delim + 2);
                
                bool found = false;
                if (metrics_panel) {
                    for (auto& tile : metrics_panel->tiles) {
                        if (tile.name == node_name) {
                            for (auto& metric : tile.metrics) {
                                if (metric.name == metric_name) {
                                    try {
                                        // Try to parse as different types
                                        if (value_str == "true" || value_str == "false") {
                                            metric.type = MetricType::BOOL;
                                            metric.value = (value_str == "true");
                                        } else if (value_str.find('.') != std::string::npos) {
                                            metric.type = MetricType::FLOAT;
                                            metric.value = std::stod(value_str);
                                        } else {
                                            metric.type = MetricType::INT;
                                            metric.value = std::stoi(value_str);
                                        }
                                        metric.last_update = std::chrono::system_clock::now();
                                        AddLog("Updated: " + node_name + "::" + metric_name + " = " + value_str);
                                        found = true;
                                    } catch (...) {
                                        AddLog("Error parsing value: " + value_str);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                if (!found) {
                    AddLog("Metric not found: " + node_name + "::" + metric_name);
                }
            }
        }
    } else if (command == "clear_metrics") {
        // Clear all metrics from all tiles
        if (metrics_panel) {
            int cleared_count = 0;
            for (auto& tile : metrics_panel->tiles) {
                cleared_count += tile.metrics.size();
                tile.metrics.clear();
                tile.event_type_counts.clear();
            }
            AddLog("Cleared " + std::to_string(cleared_count) + " metrics from " + 
                   std::to_string(metrics_panel->GetTileCount()) + " nodes");
        } else {
            AddLog("No metrics to clear");
        }
    } else if (command == "export_metrics") {
        // Export metrics to JSON or CSV format
        std::string format;
        if (!(iss >> format)) {
            format = "json";  // Default format
        }
        
        if (metrics_panel && !metrics_panel->tiles.empty()) {
            if (format == "json") {
                AddLog("{\"metrics\": [");
                bool first = true;
                for (const auto& tile : metrics_panel->tiles) {
                    for (const auto& metric : tile.metrics) {
                        if (!first) AddLog(",");
                        AddLog("  {\"node\": \"" + tile.name + "\", \"name\": \"" + metric.name + 
                               "\", \"value\": \"" + metric.GetFormattedValue() + "\"}");
                        first = false;
                    }
                }
                AddLog("]}");
                AddLog("Exported " + std::to_string(metrics_panel->GetTileCount()) + " nodes to JSON");
            } else if (format == "csv") {
                AddLog("node,metric,value,type,confidence");
                for (const auto& tile : metrics_panel->tiles) {
                    for (const auto& metric : tile.metrics) {
                        std::string type_str;
                        switch (metric.type) {
                            case MetricType::INT: type_str = "int"; break;
                            case MetricType::FLOAT: type_str = "float"; break;
                            case MetricType::BOOL: type_str = "bool"; break;
                            case MetricType::STRING: type_str = "string"; break;
                        }
                        int conf_percent = (int)(metric.confidence * 100);
                        AddLog(tile.name + "," + metric.name + "," + metric.GetFormattedValue() + 
                               "," + type_str + "," + std::to_string(conf_percent) + "%");
                    }
                }
                AddLog("Exported " + std::to_string(metrics_panel->GetTileCount()) + " nodes to CSV");
            } else {
                AddLog("Unknown format: " + format + " (use 'json' or 'csv')");
            }
        } else {
            AddLog("No metrics to export");
        }
    } else {
        AddLog("Unknown command: " + command + " (type 'help' for commands)");
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
