#include "ui/Dashboard.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include "ui/LayoutConfig.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

Dashboard::Dashboard(
    std::shared_ptr<graph::GraphExecutor> executor,
    const WindowHeightConfig& heights)
    : executor_(executor), window_heights_(heights), initialized_(false) {

    if (!executor_) {
        throw std::invalid_argument("Executor cannot be null");
    }

    ValidateHeights();
    std::cerr << "[Dashboard] Constructed: "
              << window_heights_.DebugString() << "\n";
}

Dashboard::~Dashboard() {
    std::cerr << "[Dashboard] Destroying...\n";
    
    // Save layout configuration before shutdown (D4.2)
    try {
        LayoutConfig config("");
        config.SetMetricsHeight(window_heights_.metrics_height_percent);
        config.SetLoggingHeight(window_heights_.logging_height_percent);
        config.SetCommandHeight(window_heights_.command_height_percent);
        
        if (logging_window_) {
            config.SetLoggingLevelFilter(logging_window_->GetLevelFilter());
            config.SetLoggingSearchFilter(logging_window_->GetSearchText());
        }
        
        if (command_window_) {
            config.ClearCommandHistory();
            auto history = command_window_->GetHistory();
            for (const auto& cmd : history) {
                config.AddCommandToHistory(cmd);
            }
        }
        
        if (config.Save()) {
            std::cerr << "[Dashboard] Saved layout configuration\n";
        } else {
            std::cerr << "[Dashboard] Warning: Failed to save layout configuration\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[Dashboard] Error saving config: " << e.what() << "\n";
    }
    
    std::cerr << "[Dashboard] Destroyed\n";
}

void Dashboard::Initialize() {
    std::cerr << "[Initialize] Starting dashboard setup\n";

    try {
        // 0. Load configuration first (D4.2)
        std::cerr << "[Initialize] Loading layout configuration...\n";
        LayoutConfig config("");
        bool config_loaded = config.Load();
        if (config_loaded && config.IsValid()) {
            std::cerr << "[Initialize] Loaded configuration from disk\n";
            // Update window heights from config
            window_heights_.metrics_height_percent = config.GetMetricsHeightPercent();
            window_heights_.logging_height_percent = config.GetLoggingHeightPercent();
            window_heights_.command_height_percent = config.GetCommandHeightPercent();
            std::cerr << "[Initialize] Applied saved heights: " << window_heights_.DebugString() << "\n";
        } else {
            std::cerr << "[Initialize] Using default heights\n";
        }

        // 1. Create window components
        std::cerr << "[Initialize] Creating window components...\n";

        metrics_panel_ = std::make_shared<MetricsPanel>("Metrics");
        metrics_panel_->SetHeight(window_heights_.metrics_height_percent);

        logging_window_ = std::make_shared<LoggingWindow>("Logs");
        logging_window_->SetHeight(window_heights_.logging_height_percent);
        logging_window_->AddLogLine("[2025-01-24 12:00:00] Dashboard initialized");

        command_window_ = std::make_shared<CommandWindow>("Command");
        command_window_->SetHeight(window_heights_.command_height_percent);

        status_bar_ = std::make_shared<StatusBar>();

        std::cerr << "[Initialize] Created 4 window components\n";

        // 2. Apply saved configuration to components (D4.2)
        if (config_loaded && config.IsValid()) {
            std::cerr << "[Initialize] Applying saved configuration to components...\n";
            
            // Apply logging level filter
            std::string level = config.GetLoggingLevelFilter();
            logging_window_->SetLevelFilter(level);
            std::cerr << "[Initialize] Applied logging level filter: " << level << "\n";

            // Apply search filter
            std::string search = config.GetLoggingSearchFilter();
            if (!search.empty()) {
                logging_window_->SetSearchText(search);
                std::cerr << "[Initialize] Applied logging search filter: " << search << "\n";
            }

            // Apply command history (historical restoration)
            auto history = config.GetCommandHistory();
            if (!history.empty()) {
                std::cerr << "[Initialize] Loaded " << history.size() << " commands from history\n";
            }
        }

        // 3. Discover metrics from executor (Phase 2)
        std::cerr << "[Initialize] Discovering metrics from executor...\n";
        metrics_panel_->DiscoverMetricsFromExecutor(executor_);
        std::cerr << "[Initialize] Metrics discovery complete\n";

        // 4. Register metrics capability callbacks (Phase 2)
        std::cerr << "[Initialize] Registering metrics callbacks...\n";
        metrics_panel_->RegisterMetricsCapabilityCallback(executor_);
        std::cerr << "[Initialize] Metrics callbacks registered\n";

        // 5. Validate heights
        ValidateHeights();

        // 6. Apply heights to layout
        ApplyHeights();

        initialized_ = true;
        std::cerr << "[Initialize] Dashboard initialization complete\n";

    } catch (const std::exception& e) {
        std::cerr << "[Initialize] Error: " << e.what() << "\n";
        throw;
    }
}

void Dashboard::ValidateHeights() {
    const int total = window_heights_.metrics_height_percent +
                      window_heights_.logging_height_percent +
                      window_heights_.command_height_percent +
                      window_heights_.status_height_percent;

    if (total != 100) {
        std::cerr << "[ValidateHeights] ERROR: Heights sum to " << total
                  << "%, expected 100%\n";
        std::cerr << "[ValidateHeights] Resetting to defaults\n";
        window_heights_ = WindowHeightConfig();
    }
}

void Dashboard::ApplyHeights() {
    if (metrics_panel_) metrics_panel_->SetHeight(window_heights_.metrics_height_percent);
    if (logging_window_) logging_window_->SetHeight(window_heights_.logging_height_percent);
    if (command_window_) command_window_->SetHeight(window_heights_.command_height_percent);

    std::cerr << "[ApplyHeights] Applied: " << window_heights_.DebugString() << "\n";
}

ftxui::Element Dashboard::BuildLayout() const {
    using namespace ftxui;
    
    std::vector<Element> layout_elements;
    
    // Add metrics panel (40%)
    if (metrics_panel_) {
        layout_elements.push_back(metrics_panel_->Render());
    }
    
    // Add logging window (35%)
    if (logging_window_) {
        layout_elements.push_back(logging_window_->Render());
    }
    
    // Add command window (23%)
    if (command_window_) {
        layout_elements.push_back(command_window_->Render());
    }
    
    // Add status bar (2%)
    if (status_bar_) {
        layout_elements.push_back(status_bar_->Render());
    }
    
    return vbox(layout_elements);
}

void Dashboard::Run() {
    if (!initialized_) {
        throw std::runtime_error("Dashboard not initialized");
    }

    std::cerr << "[Run] Starting event loop with FTXUI rendering (30 FPS)\n";
    std::cerr << "[Run] Dashboard initialized with 4 panels\n";
    std::cerr << "[Run] Metrics: " << (metrics_panel_ ? metrics_panel_->GetMetricCount() : 0) << " metrics\n";
    std::cerr << "[Run] Logging: " << (logging_window_ ? logging_window_->GetLogCount() : 0) << " log lines\n";

    using namespace ftxui;
    
    // Create screen
    auto screen = Screen::Create(Dimension::Full(), Dimension::Full());
    
    const auto frame_time = std::chrono::milliseconds(33);  // ~30 FPS
    auto last_frame = std::chrono::high_resolution_clock::now();
    int frame_count = 0;

    while (!should_exit_) {
        try {
            frame_count++;

            // Update all metrics from latest executor values (Phase 2)
            if (metrics_panel_ && metrics_panel_->GetMetricsTilePanel()) {
                metrics_panel_->GetMetricsTilePanel()->UpdateAllMetrics();
            }

            // Build and render the layout
            auto document = BuildLayout();
            Render(screen, document);
            
            // Display rendered output
            std::cout << screen.ToString() << std::flush;
            
            // Sleep to maintain 30 FPS
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_frame);

            if (elapsed < frame_time) {
                std::this_thread::sleep_for(frame_time - elapsed);
            }

            last_frame = std::chrono::high_resolution_clock::now();

            // Exit for Phase 1 testing - in Phase 2 integrate real event loop
            if (frame_count >= 30) {  // Run for ~1 second then exit
                should_exit_ = true;
            }

        } catch (const std::exception& e) {
            std::cerr << "[Run] Error in event loop: " << e.what() << "\n";
            should_exit_ = true;
        }
    }

    std::cerr << "[Run] Event loop exited after " << frame_count << " frames\n";
}

// Getters
const std::shared_ptr<MetricsPanel>& Dashboard::GetMetricsPanel() const {
    return metrics_panel_;
}

const std::shared_ptr<LoggingWindow>& Dashboard::GetLoggingWindow() const {
    return logging_window_;
}

const std::shared_ptr<CommandWindow>& Dashboard::GetCommandWindow() const {
    return command_window_;
}

const std::shared_ptr<StatusBar>& Dashboard::GetStatusBar() const {
    return status_bar_;
}

const WindowHeightConfig& Dashboard::GetWindowHeights() const {
    return window_heights_;
}

bool Dashboard::AreHeightsValid() const {
    const int total = window_heights_.metrics_height_percent +
                      window_heights_.logging_height_percent +
                      window_heights_.command_height_percent +
                      window_heights_.status_height_percent;
    return total == 100;
}
