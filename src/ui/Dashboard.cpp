#include "ui/Dashboard.hpp"
#include "ui/MetricsPanel.hpp"
#include "ui/LoggingWindow.hpp"
#include "ui/LoggingWindowAppender.hpp"
#include "ui/CommandWindow.hpp"
#include "ui/CommandRegistry.hpp"
#include "ui/BuiltinCommands.hpp"
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
#include <ftxui/component/screen_interactive.hpp>
#include <log4cxx/level.h>
#include <log4cxx/patternlayout.h>

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

        // Register LoggingWindow with log4cxx root logger (Phase 3)
        auto logging_appender = logging_window_->GetAppender();
        if (logging_appender) {
            auto screen = ftxui::ScreenInteractive::Fullscreen();

            // Set a callback on the appender so log messages trigger a screen redraw
            // This ensures messages appear immediately instead of waiting for the next event
            logging_appender->SetCallback([&screen](...) {
                screen.PostEvent(ftxui::Event::Custom);
            });
            
            // Create proper log4cxx appender that wraps LoggingAppender
            auto window_appender = std::make_shared<LoggingWindowAppender>(logging_appender);
            
            // Set up layout for formatted output
            auto layout = std::make_shared<log4cxx::PatternLayout>("%d{ISO8601} [%p] %c - %m");
            window_appender->setLayout(layout);
            
            // Set appender threshold to TRACE to pass all messages through appender filtering
            // This is separate from the logger level - the appender filters messages below its threshold
            window_appender->setThreshold(log4cxx::Level::getTrace());
            
            // Register with root logger - now ALL LOG4CXX calls appear in LoggingWindow
            auto root_logger = log4cxx::Logger::getRootLogger();
            root_logger->addAppender(window_appender);
            root_logger->setLevel(log4cxx::Level::getTrace());  // Show all levels
            
            LOG4CXX_INFO(dashboard_logger, "LoggingWindowAppender registered with root logger - LOG4CXX output enabled");
        } else {
            LOG4CXX_WARN(dashboard_logger, "Failed to get LoggingWindow appender - LOG4CXX integration disabled");
        }

        command_window_ = std::make_shared<CommandWindow>("Command");
        command_window_->SetHeight(window_heights_.command_height_percent);

        // Phase 4: Create and initialize command registry with built-in commands
        command_registry_ = std::make_shared<CommandRegistry>();
        commands::RegisterBuiltinCommands(command_registry_, this);
        command_window_->SetCommandRegistry(command_registry_);
        LOG4CXX_TRACE(dashboard_logger, "CommandRegistry initialized with built-in commands");

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

    // Get terminal dimensions
    auto terminal_size = Terminal::Size();
    int terminal_height = terminal_size.dimy;
    int terminal_width = terminal_size.dimx;
    
    // Ensure minimum terminal dimensions
    if (terminal_height < 10 || terminal_width < 40) {
        // Terminal too small - return placeholder
        return text("Terminal too small. Minimum: 40x10");
    }

    // Calculate pixel heights from percentages
    int metrics_height = std::max(1, (terminal_height * window_heights_.metrics_height_percent) / 100);
    int logging_height = std::max(1, (terminal_height * window_heights_.logging_height_percent) / 100);
    int command_height = std::max(1, (terminal_height * window_heights_.command_height_percent) / 100);
    int status_height = std::max(1, (terminal_height * window_heights_.status_height_percent) / 100);
    
    // Adjust for rounding: if total < terminal_height, add difference to logging
    int total_height = metrics_height + logging_height + command_height + status_height;
    if (total_height < terminal_height) {
        logging_height += (terminal_height - total_height);
    }

    std::vector<Element> layout_elements;

    // Add metrics panel with height constraint
    if (metrics_panel_) {
        layout_elements.push_back(
            metrics_panel_->Render()
            | size(HEIGHT, EQUAL, metrics_height)
        );
    }

    // Add logging window with height constraint
    if (logging_window_) {
        layout_elements.push_back(
            logging_window_->Render()
            | size(HEIGHT, EQUAL, logging_height)
        );
    }

    // Add command window with height constraint
    if (command_window_) {
        layout_elements.push_back(
            command_window_->Render()
            | size(HEIGHT, EQUAL, command_height)
        );
    }

    // Add status bar with height constraint
    if (status_bar_) {
        layout_elements.push_back(
            status_bar_->Render()
            | size(HEIGHT, EQUAL, status_height)
        );
    }

    return vbox(layout_elements);
}

void Dashboard::Run() {
    if (!initialized_) {
        throw std::runtime_error("Dashboard not initialized");
    }

    LOG4CXX_INFO(dashboard_logger, "Starting event loop (FTXUI 6.1.9 component-based)");

    using namespace ftxui;

    // Store initial dimensions for resize detection
    auto terminal_size = Terminal::Size();
    last_screen_width_ = terminal_size.dimx;
    last_screen_height_ = terminal_size.dimy;
    
    LOG4CXX_INFO(dashboard_logger, "Terminal size: " << last_screen_width_ << "x" << last_screen_height_);

    // Create a component that wraps our dashboard layout
    // For FTXUI 6.1.9, we need to use component-based rendering
    // Create a placeholder component that will call our render function
    auto dashboard_component = Renderer([this] {
        // Build and return our layout
        return BuildLayout();
    });

    auto screen = ScreenInteractive::Fullscreen();

    auto handler = CatchEvent(dashboard_component, [this, &screen](Event event) {
        if (event == Event::Character('q')) {
            LOG4CXX_INFO(dashboard_logger, "User quit via 'q'");
            screen.Exit();
            return true;  // consume
        }
        if(event == Event::Resize) {
            LOG4CXX_DEBUG(dashboard_logger, "Resize event received");
            RecalculateLayout();
            return false;  // consume
        }
        HandleKeyEvent(event);
        return false;
    });

    // Create and run the interactive screen
    screen.Loop(handler);

    LOG4CXX_INFO(dashboard_logger, "Event loop exited");
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
            command_window_->HandleKeyInput(static_cast<int>(event.character()[0]));
            return;
        }
        
        // Enter executes command
        if (event == Event::Return) {
            LOG4CXX_DEBUG(dashboard_logger, "Enter pressed - executing command");
            command_window_->HandleKeyInput(10);  // Enter = ASCII 10
            return;
        }
        
        // Backspace for editing
        if (event == Event::Backspace) {
            LOG4CXX_TRACE(dashboard_logger, "Backspace pressed");
            command_window_->HandleKeyInput(127);  // Backspace = ASCII 127
            return;
        }
        
        // Escape to clear input
        if (event == Event::Escape) {
            LOG4CXX_TRACE(dashboard_logger, "Escape pressed - clearing input");
            command_window_->HandleKeyInput(27);  // Escape = ASCII 27
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

const std::shared_ptr<CommandRegistry>& Dashboard::GetCommandRegistry() const {
    return command_registry_;
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
