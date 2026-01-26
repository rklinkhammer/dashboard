#include "ui/Dashboard.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/StatusBar.hpp"
#include "ui/LayoutConfig.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/component/event.hpp>

static log4cxx::LoggerPtr dashboard_logger = log4cxx::Logger::getLogger("dashboard.Dashboard");

Dashboard::Dashboard(
    std::shared_ptr<app::capabilities::MetricsCapability> capability,
    const WindowHeightConfig& heights)
    : capability_(capability), window_heights_(heights), initialized_(false) {

    if (!capability_) {
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
    LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize() called");  

    try {
        // 0. Load configuration first (D4.2)
        LOG4CXX_TRACE(dashboard_logger, "Loading layout configuration...");
        LayoutConfig config("");
        bool config_loaded = config.Load();
        if (config_loaded && config.IsValid()) {
            LOG4CXX_TRACE(dashboard_logger, "Loaded configuration from disk");
            // Update window heights from config
            window_heights_.metrics_height_percent = config.GetMetricsHeightPercent();
            window_heights_.logging_height_percent = config.GetLoggingHeightPercent();
            window_heights_.command_height_percent = config.GetCommandHeightPercent();
            LOG4CXX_TRACE(dashboard_logger, "Applied saved heights: " << window_heights_.DebugString());
        } else {
            LOG4CXX_TRACE(dashboard_logger, "Using default heights");
        }

        // 1. Create window components
        LOG4CXX_TRACE(dashboard_logger, "Creating window components...");

        metrics_panel_ = std::make_shared<MetricsPanel>("Metrics");
        metrics_panel_->SetHeight(window_heights_.metrics_height_percent);

        logging_window_ = std::make_shared<LoggingWindow>("Logs");
        logging_window_->SetHeight(window_heights_.logging_height_percent);
        logging_window_->AddLogLine("[2025-01-24 12:00:00] Dashboard initialized");

        command_window_ = std::make_shared<CommandWindow>("Command");
        command_window_->SetHeight(window_heights_.command_height_percent);

        status_bar_ = std::make_shared<StatusBar>();

        LOG4CXX_TRACE(dashboard_logger, "Created 4 window components");

        // 2. Apply saved configuration to components (D4.2)
        if (config_loaded && config.IsValid()) {
            LOG4CXX_TRACE(dashboard_logger, "Applying saved configuration to components...");
            
            // Apply logging level filter
            std::string level = config.GetLoggingLevelFilter();
            logging_window_->SetLevelFilter(level);
            LOG4CXX_TRACE(dashboard_logger, "Applied logging level filter: " << level);

            // Apply search filter
            std::string search = config.GetLoggingSearchFilter();
            if (!search.empty()) {
                logging_window_->SetSearchText(search);
                LOG4CXX_TRACE(dashboard_logger, "Applied logging search filter: " << search);
            }

            // Apply command history (historical restoration)
            auto history = config.GetCommandHistory();
            if (!history.empty()) {
                LOG4CXX_TRACE(dashboard_logger, "Loaded " << history.size() << " commands from history");
            }
        }

        // 3. Discover metrics from executor (Phase 2)
        LOG4CXX_TRACE(dashboard_logger, "Initializing metrics discovery from executor");
        metrics_panel_->DiscoverMetricsFromExecutor(capability_);
        LOG4CXX_TRACE(dashboard_logger, "Metrics discovery complete");

        // 4. Register metrics capability callbacks (Phase 2)
        LOG4CXX_TRACE(dashboard_logger, "Registering metrics callbacks");
        metrics_panel_->RegisterMetricsCapabilityCallback(capability_);
        LOG4CXX_TRACE(dashboard_logger, "Metrics callbacks registered");

        // 5. Validate heights
        ValidateHeights();

        // 6. Apply heights to layout
        ApplyHeights();

        initialized_ = true;
        LOG4CXX_TRACE(dashboard_logger, "Dashboard initialization complete");
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

    LOG4CXX_INFO(dashboard_logger, "Starting event loop (30 FPS, terminal input enabled)");

    using namespace ftxui;

    // Create screen
    auto screen = Screen::Create(Dimension::Full(), Dimension::Full());
    
    // Store initial dimensions
    last_screen_width_ = screen.dimx();
    last_screen_height_ = screen.dimy();
    
    LOG4CXX_INFO(dashboard_logger, "Terminal size: " << last_screen_width_ << "x" << last_screen_height_);

    // Main event loop
    last_frame_ = std::chrono::high_resolution_clock::now();
    int frame_count = 0;

    while (!should_exit_) {
        try {
            frame_count++;
            
            // 1. Check for terminal resize
            if (CheckForTerminalResize()) {
                LOG4CXX_DEBUG(dashboard_logger, "Terminal resized to " 
                    << last_screen_width_ << "x" << last_screen_height_);
                RecalculateLayout();
                screen = Screen::Create(Dimension::Full(), Dimension::Full());
            }

            // 2. Read and process events (check for keyboard input)
            // Note: FTXUI event handling will be added here
            // For now, we use a placeholder that checks for terminal input
            
            // 3. Update metrics from capability
            if (metrics_panel_ && metrics_panel_->GetMetricsTilePanel()) {
                metrics_panel_->GetMetricsTilePanel()->UpdateAllMetrics();
            }

            // 4. Build and render layout
            auto document = BuildLayout();
            Render(screen, document);
            std::cout << screen.ToString() << std::flush;

            // 5. Maintain 30 FPS
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_frame_);

            if (elapsed < frame_time_) {
                std::this_thread::sleep_for(frame_time_ - elapsed);
            }
            last_frame_ = std::chrono::high_resolution_clock::now();

            // Debug: Log every 30 frames (1 second at 30 FPS)
            if (frame_count % 30 == 0) {
                LOG4CXX_TRACE(dashboard_logger, "Event loop tick: " << frame_count << " frames");
            }

        } catch (const std::exception& e) {
            LOG4CXX_ERROR(dashboard_logger, "Error in event loop: " << e.what());
            should_exit_ = true;
        }
    }

    LOG4CXX_INFO(dashboard_logger, "Event loop exited after " << frame_count << " frames");
}

void Dashboard::HandleKeyEvent(const ftxui::Event& event) {
    using namespace ftxui;
    
    // Global quit command
    if (event == Event::Character('q')) {
        LOG4CXX_INFO(dashboard_logger, "User quit via 'q' key");
        should_exit_ = true;
        return;
    }
    
    // Tab navigation (left/right arrows) - if MetricsPanel supports it
    if (metrics_panel_) {
        if (event == Event::ArrowLeft) {
            LOG4CXX_DEBUG(dashboard_logger, "Left arrow pressed");
            // TODO: Call metrics_panel_->PreviousTab() if tab mode enabled
            return;
        }
        if (event == Event::ArrowRight) {
            LOG4CXX_DEBUG(dashboard_logger, "Right arrow pressed");
            // TODO: Call metrics_panel_->NextTab() if tab mode enabled
            return;
        }
    }
    
    // Route to CommandWindow for input
    if (command_window_) {
        // Characters go to command input
        if (event.is_character()) {
            LOG4CXX_TRACE(dashboard_logger, "Character input: " << event.character()[0]);
            // TODO: command_window_->AddInputCharacter(event.character()[0]);
            return;
        }
        
        // Enter executes command
        if (event == Event::Return) {
            LOG4CXX_DEBUG(dashboard_logger, "Enter pressed - executing command");
            // TODO: command_window_->ExecuteInputCommand();
            return;
        }
        
        // Backspace for editing
        if (event == Event::Backspace) {
            LOG4CXX_TRACE(dashboard_logger, "Backspace pressed");
            // TODO: command_window_->RemoveLastInputCharacter();
            return;
        }
    }
    
    LOG4CXX_TRACE(dashboard_logger, "Unhandled event (ignoring)");
}

bool Dashboard::CheckForTerminalResize() {
    auto term_size = ftxui::Terminal::Size();
    if (term_size.dimx != last_screen_width_ || 
        term_size.dimy != last_screen_height_) {
        last_screen_width_ = term_size.dimx;
        last_screen_height_ = term_size.dimy;
        return true;
    }
    return false;
}

void Dashboard::RecalculateLayout() {
    // Update all component heights based on new screen size
    ApplyHeights();
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
