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
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <log4cxx/logger.h>
#include <log4cxx/level.h>
#include <log4cxx/patternlayout.h>

static log4cxx::LoggerPtr dashboard_logger = log4cxx::Logger::getLogger("dashboard.Dashboard");

Dashboard::Dashboard(
    std::shared_ptr<app::capabilities::MetricsCapability> capability,
    const WindowHeightConfig& heights)
    : capability_(capability),
      screen_(ftxui::ScreenInteractive::Fullscreen()),  // <-- initialized here
      window_heights_(heights),
      initialized_(false),
      show_debug_overlay_(false) {

    if (!capability_) {
        throw std::invalid_argument("Executor cannot be null");
    }

    ValidateHeights();
    std::cerr << "[Dashboard] Constructed: " << window_heights_.DebugString() << "\n";
}

Dashboard::~Dashboard() {
    std::cerr << "[Dashboard] Destroying...\n";

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
        // Load layout configuration
        LayoutConfig config("");
        bool config_loaded = config.Load();
        if (config_loaded && config.IsValid()) {
            window_heights_.metrics_height_percent = config.GetMetricsHeightPercent();
            window_heights_.logging_height_percent = config.GetLoggingHeightPercent();
            window_heights_.command_height_percent = config.GetCommandHeightPercent();
        }

        // Create window components
        metrics_panel_ = std::make_shared<MetricsPanel>("Metrics");
        metrics_panel_->SetHeight(window_heights_.metrics_height_percent);
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Created MetricsPanel");
    

        logging_window_ = std::make_shared<LoggingWindow>("Logs");
        logging_window_->SetHeight(window_heights_.logging_height_percent);
        logging_window_->AddLogLine("[Dashboard] Initialized");
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Created LoggingWindow");
        
        // Register LoggingWindow appender - now working with component-based rendering
        // Note: The appender integration is working but disabled for now due to
        // FTXUI component rendering lifecycle. Appender callbacks would trigger
        // re-renders in the component system differently.
        // For now, use LoggingWindow::AddLogLine() directly from application code.
        auto logging_appender = logging_window_->GetAppender();
        if (logging_appender) {
            // Callback to mark screen dirty when log messages arrive
            logging_appender->SetCallback([this](const std::string&, const std::string&) {
                MarkScreenDirty();
            });
            
            LOG4CXX_DEBUG(dashboard_logger, "LoggingWindow appender configured with screen dirty callback");
        }

        command_window_ = std::make_shared<CommandWindow>("Command");
        command_window_->SetHeight(window_heights_.command_height_percent);
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Created CommandWindow");

        command_registry_ = std::make_shared<CommandRegistry>();
        commands::RegisterBuiltinCommands(command_registry_, this);
        command_window_->SetCommandRegistry(command_registry_);
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Created CommandRegistry and registered built-in commands");

        status_bar_ = std::make_shared<StatusBar>();
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Created StatusBar");
        // Metrics
        metrics_panel_->DiscoverMetricsFromExecutor(capability_);
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Discovered metrics from executor");
        metrics_panel_->RegisterMetricsCapabilityCallback(capability_);
        LOG4CXX_TRACE(dashboard_logger, "Dashboard::Initialize: Registered metrics capability callback");

        ValidateHeights();
        ApplyHeights();

        initialized_ = true;

    } catch (const std::exception& e) {
        std::cerr << "[Initialize] Error: " << e.what() << "\n";
        throw;
    }
}

void Dashboard::Run() {
    if (!initialized_) throw std::runtime_error("Dashboard not initialized");
    LOG4CXX_TRACE(dashboard_logger, "Dashboard::Run() called");

    using namespace ftxui;

    last_screen_width_ = Terminal::Size().dimx;
    last_screen_height_ = Terminal::Size().dimy;

    // Component that renders our dashboard layout
    auto dashboard_renderer = Renderer([this] {
        auto layout = BuildLayout();
        if (show_debug_overlay_) {
            layout |= border;
        }
        return layout;
    });

    // Event handling
    auto root_component = CatchEvent(dashboard_renderer, [this](Event event) {
        // Quit
        if (event == Event::Character('q')) {
            should_exit_ = true;
            screen_.Exit();
            return true;
        }

        // Toggle debug overlay (F1)
        if (event == Event::F1) {
            show_debug_overlay_ = !show_debug_overlay_;
            return true;
        }

        // // MetricsPanel tab switching
        if (metrics_panel_) {
            if (event == Event::ArrowLeft) {
                metrics_panel_->PreviousTab();
                return true;
            }
            if (event == Event::ArrowRight) {
                metrics_panel_->NextTab();
                return true;
            }
        }

        // CommandWindow input
        if (command_window_) {
            if (event.is_character()) {
                command_window_->HandleKeyInput(static_cast<int>(event.character()[0]));
                return true;
            }
            if (event == Event::Return) {
                command_window_->HandleKeyInput(10);
                return true;
            }
            if (event == Event::Backspace) {
                command_window_->HandleKeyInput(127);
                return true;
            }
            if (event == Event::Escape) {
                command_window_->HandleKeyInput(27);
                return true;
            }
        }

        return false;
    });

    // Run main loop
    screen_.Loop(root_component);
}

void Dashboard::ValidateHeights() {
    const int total = window_heights_.metrics_height_percent +
                      window_heights_.logging_height_percent +
                      window_heights_.command_height_percent +
                      window_heights_.status_height_percent;
    if (total != 100) {
        std::cerr << "[ValidateHeights] Heights sum to " << total << "%, resetting to defaults\n";
        window_heights_ = WindowHeightConfig();
    }
}

void Dashboard::ApplyHeights() {
    if (metrics_panel_) metrics_panel_->SetHeight(window_heights_.metrics_height_percent);
    if (logging_window_) logging_window_->SetHeight(window_heights_.logging_height_percent);
    if (command_window_) command_window_->SetHeight(window_heights_.command_height_percent);
}

ftxui::Element Dashboard::BuildLayout() const {
    using namespace ftxui;
    std::vector<Element> layout_elements;

    if (metrics_panel_)
        layout_elements.push_back(metrics_panel_->Render() | size(HEIGHT, EQUAL, metrics_panel_->GetHeight()));

    if (logging_window_)
        layout_elements.push_back(logging_window_->Render() | size(HEIGHT, EQUAL, logging_window_->GetHeight()));

    if (command_window_)
        layout_elements.push_back(command_window_->Render() | size(HEIGHT, EQUAL, command_window_->GetHeight()));

    if (status_bar_)
        layout_elements.push_back(status_bar_->Render() | size(HEIGHT, EQUAL, window_heights_.status_height_percent));

    return vbox(layout_elements);
}

// ---- Key / utility functions ----

void Dashboard::HandleKeyEvent(const ftxui::Event& event) {
    (void)event;
    // Already handled in Run()
}

bool Dashboard::CheckForTerminalResize() {
    auto term_size = ftxui::Terminal::Size();
    if (term_size.dimx != last_screen_width_ || term_size.dimy != last_screen_height_) {
        last_screen_width_ = term_size.dimx;
        last_screen_height_ = term_size.dimy;
        return true;
    }
    return false;
}

void Dashboard::RecalculateLayout() {
    ApplyHeights();
}

// Getters

const std::shared_ptr<MetricsPanel>& Dashboard::GetMetricsPanel() const { return metrics_panel_; }
const std::shared_ptr<LoggingWindow>& Dashboard::GetLoggingWindow() const { return logging_window_; }
const std::shared_ptr<CommandWindow>& Dashboard::GetCommandWindow() const { return command_window_; }
const std::shared_ptr<CommandRegistry>& Dashboard::GetCommandRegistry() const { return command_registry_; }
const std::shared_ptr<StatusBar>& Dashboard::GetStatusBar() const { return status_bar_; }
const WindowHeightConfig& Dashboard::GetWindowHeights() const { return window_heights_; }
bool Dashboard::AreHeightsValid() const { return window_heights_.Validate(); }
