#include "ui/Dashboard.hpp"
#include "app/capabilities/MetricsCapability.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <deque>
#include <iostream>
#include <iostream>
#include <thread>
#include <chrono>
#include <log4cxx/logger.h>
#include <log4cxx/level.h>
#include <log4cxx/patternlayout.h>

static log4cxx::LoggerPtr dashboard_logger = log4cxx::Logger::getLogger("dashboard.Dashboard");

Dashboard::Dashboard(
    std::shared_ptr<app::capabilities::MetricsCapability> capability)
    : capability_(capability),
      screen_(ftxui::ScreenInteractive::Fullscreen()) {  // <-- initialized here

    if (!capability_) {
        throw std::invalid_argument("Executor cannot be null");
    }
}

Dashboard::~Dashboard() {

    // try {
    //     LayoutConfig config("");
    //     config.SetMetricsHeight(window_heights_.metrics_height_percent);
    //     config.SetLoggingHeight(window_heights_.logging_height_percent);
    //     config.SetCommandHeight(window_heights_.command_height_percent);

    //     if (logging_window_) {
    //         config.SetLoggingLevelFilter(logging_window_->GetLevelFilter());
    //         config.SetLoggingSearchFilter(logging_window_->GetSearchText());
    //     }

    //     // if (command_window_) {
    //     //     config.ClearCommandHistory();
    //     //     auto history = command_window_->GetHistory();
    //     //     for (const auto& cmd : history) {
    //     //         config.AddCommandToHistory(cmd);
    //     //     }
    //     // }

    //     if (config.Save()) {
    //         std::cerr << "[Dashboard] Saved layout configuration\n";
    //     } else {
    //         std::cerr << "[Dashboard] Warning: Failed to save layout configuration\n";
    //     }
    // } catch (const std::exception& e) {
    //     std::cerr << "[Dashboard] Error saving config: " << e.what() << "\n";
    // }

    // std::cerr << "[Dashboard] Destroyed\n";
}

void Dashboard::Run() {
    using namespace ftxui;  
    // ---------------------------
    // State
    // ---------------------------
    std::deque<std::string> metrics_lines = {
        "Metric 1: 42",
        "Metric 2: 13",
        "Metric 3: 99"
    };

    std::deque<std::string> log_lines = {
        "[INFO] System started",
        "[INFO] Waiting for commands..."
    };

    std::string command_input;

    // ---------------------------
    // Components
    // ---------------------------

    // Metrics renderer (read-only)
    auto metrics_renderer = Renderer([&] {
        std::vector<Element> elems;
        for (auto& line : metrics_lines)
            elems.push_back(text(line));
        return vbox(elems) | border | color(Color::Green);
    });

    // Logging renderer (read-only)
    auto logging_renderer = Renderer([&] {
        std::vector<Element> elems;
        for (auto& line : log_lines)
            elems.push_back(text(line));
        return vbox(elems) | border | color(Color::Blue);
    });

    // Command input (focusable)
    auto command_input_component = Input(&command_input, ">") | border | color(Color::Yellow);
    auto command_renderer = Renderer(command_input_component, [&] {
        return vbox({
            text("> " + command_input + "_")}) | border | color(Color::Yellow);
    });

    // ---------------------------
    // Focusable container
    // ---------------------------
    int focus_index = 2;
    auto container = Container::Vertical({
        metrics_renderer,
        logging_renderer,
        command_input_component  // ONLY this receives input
    }, &focus_index);


    // ---------------------------
    // Dashboard renderer
    // ---------------------------
    auto dashboard = Renderer(container, [&] {
        std::vector<Element> elems;

        // Layout: metrics 40%, logging 40%, command 20%
        elems.push_back(metrics_renderer->Render() | size(HEIGHT, EQUAL, 40));
        elems.push_back(logging_renderer->Render() | size(HEIGHT, EQUAL, 40));
        elems.push_back(command_renderer->Render() | size(HEIGHT, EQUAL, 20));

        return vbox(elems);
    });

    // ---------------------------
    // Event handler
    // ---------------------------
    auto root = CatchEvent(dashboard, [&](Event event) {
        // Quit
        if (event == Event::Character('q')) {
            screen_.Exit();
            return true;
        }

        // Enter executes command
        if (event == Event::Return) {
            if (!command_input.empty()) {
                log_lines.push_back("[CMD] " + command_input);
                command_input.clear();
            }
            return true;
        }

        return false; // pass other events to the focused component
    });

    screen_.Loop(root);
}
