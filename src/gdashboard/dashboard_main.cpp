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

#include "ui/Dashboard.hpp"
#include "graph/GraphExecutor.hpp"
#include "graph/GraphExecutorBuilder.hpp"
#include "plugins/PluginLoader.hpp"
#include "plugins/PluginRegistry.hpp"
#include "app/capabilities/GraphCapability.hpp"
#include "graph/NodeFactory.hpp"
#include <iostream>
#include <string>
#include <map>
#include <stdexcept>
#include <thread>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include "app/SignalHandler.hpp"

static log4cxx::LoggerPtr dashboard_logger = log4cxx::Logger::getLogger("dashboard.dashboard_main");

app::capabilities::GraphCapability *g_graph_capability = nullptr;

void ExitHandler()
{    
    LOG4CXX_WARN(dashboard_logger, "ExitHandler invoked, requesting shutdown...");
    if (g_graph_capability)
    {
        LOG4CXX_WARN(dashboard_logger, "ExitHandler stopping..");
        g_graph_capability->SetStopped();
    }
}

class DashboardApplication {
public:

     /**
     * @brief Parse command-line arguments and build executor
     *
     * Supported arguments:
     * - --json-config PATH: Path to graph configuration JSON file (required)
     * - --plugin-dir PATH: Plugin directory (default: ./plugins)
     * - --log-config PATH: Path to log4cxx.properties file for logging configuration
     * - --csv-input PATH,NODE: CSV input file and target node (can be used multiple times)
     * - --csv-injection-rate MS: CSV row injection rate in milliseconds (default: 10)
     * - --executor-timeout SECONDS: Execution timeout (default: 30)
     * - --graph-threads N: Worker thread count (default: 4)
     * - --verbose: Enable verbose logging
     * - --help: Print usage information
     *
     * @param argc Argument count
     * @param argv Argument values
     *
     * @return Configured executor ready for execution, or nullptr if help requested
     * @throws std::invalid_argument if required argument missing or invalid
     */
    
    bool ParseArgs(int argc, char* argv[]) {
   
        // Parse arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                PrintUsage(argv[0]);
                return false;
            }
            else if (arg == "--json-config") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--json-config requires argument");
                }
                graph_config_ = argv[++i];
                LOG4CXX_TRACE(dashboard_logger, "Parsed json-config: " << graph_config_);
            }
            else if (arg == "--plugin-dir") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--plugin-dir requires argument");
                }
                plugin_dir_ = argv[++i];
                LOG4CXX_TRACE(dashboard_logger, "Parsed plugin-dir: " << plugin_dir_);
            }
            else if (arg == "--csv-input") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--csv-input requires argument");
                }
                std::string csv_spec = argv[++i];
                // Parse "path,node" format
                size_t comma_pos = csv_spec.find(',');
                if (comma_pos != std::string::npos) {
                    csv_inputs_.push_back({csv_spec.substr(0, comma_pos), csv_spec.substr(comma_pos + 1)});
                } else {
                    csv_inputs_.push_back({csv_spec, ""});
                }
                LOG4CXX_TRACE(dashboard_logger, "Parsed csv-input: " << csv_spec);
            }
            else if (arg == "--executor-timeout") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--executor-timeout requires argument");
                }
                try {
                    auto timeout_sec = std::stoul(argv[++i]);
                    executor_timeout_ = std::chrono::seconds(timeout_sec);
                    LOG4CXX_TRACE(dashboard_logger, "Parsed executor-timeout: " << timeout_sec << "s");
                } catch (const std::exception& e) {
                    throw std::invalid_argument("--executor-timeout must be a positive integer");
                }
            }
            else if (arg == "--graph-threads") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--graph-threads requires argument");
                }
                try {
                    graph_threads_ = std::stoul(argv[++i]);
                    LOG4CXX_TRACE(dashboard_logger, "Parsed graph-threads: " << graph_threads_);
                } catch (const std::exception& e) {
                    throw std::invalid_argument("--graph-threads must be a positive integer");
                }
            }
            else if (arg == "--graph-threads") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--graph-threads requires argument");
                }
                try {
                    graph_threads_ = std::stoul(argv[++i]);
                    LOG4CXX_TRACE(dashboard_logger, "Parsed graph-threads: " << graph_threads_);
                } catch (const std::exception& e) {
                    throw std::invalid_argument("--graph-threads must be a positive integer");
                }
            }
            else if (arg == "--logging-height") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--logging-height requires argument");
                }
                try {
                    logging_height_percent_ = std::stoul(argv[++i]);
                    LOG4CXX_TRACE(dashboard_logger, "Parsed logging-height: " << logging_height_percent_);
                } catch (const std::exception& e) {
                    throw std::invalid_argument("--logging-height must be a positive integer");
                }
            }
            else if (arg == "--command-height") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--command-height requires argument");
                }
                try {
                    command_height_percent_ = std::stoul(argv[++i]);
                    LOG4CXX_TRACE(dashboard_logger, "Parsed command-height: " << command_height_percent_);
                } catch (const std::exception& e) {
                    throw std::invalid_argument("--command-height must be a positive integer");
                }
            }
            else if (arg == "--verbose") {
                verbose_logging_ = true;
                LOG4CXX_TRACE(dashboard_logger, "Parsed verbose: enabled");
            }
            else if (arg == "--cli") {
                cli_ = true;
                LOG4CXX_TRACE(dashboard_logger, "Parsed cli: enabled");
            }
            else {
                LOG4CXX_WARN(dashboard_logger, "Unknown argument: " << arg);
            }
        }

        if( graph_config_.empty()) {
            throw std::invalid_argument("--json-config is required");
        }   
        return true;
    }

    /**
     * @brief Parse command-line arguments for log configuration only
     *
     * @param argc Argument count
     * @param argv Argument values
     */
    void ParseForLogConfig(int argc, char *argv[])
    {
        // Simple parse to get log config only
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--log-config")
            {
                if (i + 1 < argc)
                {
                    log_config_ = argv[++i];
                }
            }
        }
    }

    /**
     * @brief Build the GraphExecutor based on parsed arguments
     * @return Configured GraphExecutor instance
     */
    std::shared_ptr<graph::GraphExecutor> BuildExecutor()
    {
        // Build executor via builder pattern
        LOG4CXX_TRACE(dashboard_logger, "Building executor from configuration");
        auto builder = graph::GraphExecutorBuilder().
                    WithJsonConfig(graph_config_);
        if (!csv_inputs_.empty())
        {
            builder.WithCSVInputs(csv_inputs_);
        }
        if (!plugin_dir_.empty())
        {
            builder.WithPluginDirectory(plugin_dir_);
        }
        builder.WithExecutorTimeout(executor_timeout_)
            .WithGraphThreads(graph_threads_)
            .WithCliMode(cli_)
            .WithVerboseLogging(verbose_logging_);

        auto executor = builder.Build();

        LOG4CXX_TRACE(dashboard_logger, "Executor built successfully");
        return executor;
    }   

    /**
     * @brief Print usage information
     *
     * @param program_name Name of executable
     */
    void PrintUsage(const char* program_name) {
        std::cout << "GraphApp - Dynamic Computation Graph Executor\n\n";
        std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
        std::cout << "Required:\n";
        std::cout << "  --json-config PATH         Path to graph configuration JSON file\n\n";
        std::cout << "Optional:\n";
        std::cout << "  --log-config PATH          Path to log4cxx.properties file for logging\n";
        std::cout << "                             configuration\n";
        std::cout << "  --plugin-dir PATH          Plugin directory (default: ./plugins)\n";
        std::cout << "  --csv-input PATH[,NODE]    CSV input file and optional target node\n";
        std::cout << "                             (can be specified multiple times)\n";
        std::cout << "  --csv-injection-rate MS    CSV row injection rate in milliseconds\n";
        std::cout << "                             (default: 10, meaning 100Hz)\n";
        std::cout << "  --executor-timeout SECS    Execution timeout in seconds (default: 30)\n";
        std::cout << "  --graph-threads N          Number of worker threads (default: 4)\n";
        std::cout << "  --verbose                  Enable verbose logging\n";
        std::cout << "  --cli                      Enable command-line interface mode\n";
        std::cout << "  --help                     Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  # Simple execution\n";
        std::cout << "  " << program_name << " --json-config graph.json\n\n";
        std::cout << "  # With CSV input\n";
        std::cout << "  " << program_name << " --json-config graph.json --csv-input data.csv,SensorNode\n\n";
        std::cout << "  # With custom injection rate and threads\n";
        std::cout << "  " << program_name << " --json-config graph.json \\\n";
        std::cout << "                      --csv-input data.csv \\\n";
        std::cout << "                      --csv-injection-rate 20 \\\n";
        std::cout << "                      --graph-threads 8\n";
    }

    /**
     * @brief Get the log configuration file path
     * @return The path to the log4cxx.properties file, or empty string if not specified
     */
    std::string GetLogConfig()
    {
        return log_config_;
    }


 private:
    std::string graph_config_;
    std::string log_config_;
    std::string plugin_dir_{"./plugins"};
    std::vector<std::pair<std::string, std::string>> csv_inputs_;
    std::chrono::seconds executor_timeout_{30};
    int graph_threads_{4};
    int logging_height_percent_{20};  // Match WindowHeightConfig default
    int command_height_percent_{30};  // Match WindowHeightConfig default
    bool verbose_logging_{false};
    bool cli_{false};

    std::shared_ptr<graph::PluginRegistry> plugin_registry_{nullptr};
    std::shared_ptr<graph::PluginLoader> plugin_loader_{nullptr};
    std::shared_ptr<graph::NodeFactory> factory_{nullptr};
    std::shared_ptr<graph::GraphManager> graph_manager_{nullptr};

};


int main(int argc, char* argv[]) {
    try {
        DashboardApplication app;

        app::SignalHandler::GetInstance().RegisterSignals();
        app::SignalHandler::GetInstance().Register(ExitHandler);

        app.ParseForLogConfig(argc, argv);
        std::string log_config = app.GetLogConfig();
        if (!log_config.empty()) {
            try {
                log4cxx::PropertyConfigurator::configure(log_config);
                LOG4CXX_WARN(dashboard_logger, "Loaded log configuration from: " << log_config);
            } catch (const std::exception& e) {
                LOG4CXX_WARN(dashboard_logger, "Failed to load log configuration from " << log_config << ": " << e.what());
            }
        } else {
            log4cxx::BasicConfigurator::configure();
            LOG4CXX_TRACE(dashboard_logger, "Using basic log configuration");
        }
        if(!app.ParseArgs(argc, argv)) {
            app.PrintUsage(argv[0]);
            return 2;  // Help requested
        }

        auto executor = app.BuildExecutor();
        g_graph_capability = executor->GetCapability<app::capabilities::GraphCapability>().get();
        if(g_graph_capability == nullptr) {
            throw std::runtime_error("Failed to get GraphCapability from executor");
        }

        graph::InitializationResult init_result = executor->Init();
        if (!init_result.success) {
            LOG4CXX_ERROR(dashboard_logger, "Executor initialization failed: " << init_result.message);
            return 1;
        }
        LOG4CXX_DEBUG(dashboard_logger, "Executor initialized successfully in "
                        << init_result.elapsed_time_ms << " ms. Nodes initialized: "
                        << init_result.nodes_initialized << ", failed: " << init_result.nodes_failed);


        graph::ExecutionResult start_result = executor->Start();
        if (!start_result.success) {
            LOG4CXX_ERROR(dashboard_logger, "Executor start failed: " << start_result.message);
            return 1;
        }
        LOG4CXX_DEBUG(dashboard_logger, "Executor initialized successfully in "
                        << start_result.elapsed_time_ms << " ms. ");

        graph::ExecutionResult run_results = executor->Run();
        if (!run_results.success) {
            LOG4CXX_ERROR(dashboard_logger, "Executor run failed");
            return 1;
        }
        LOG4CXX_DEBUG(dashboard_logger, "Executor run completed successfully in "
                        << run_results.elapsed_time_ms << " ms.  ");

        graph::ExecutionResult stop_result = executor->Stop();
        if (!stop_result.success) {
            LOG4CXX_ERROR(dashboard_logger, "Executor stop failed: " << stop_result.message);
            return 1;
        }
        LOG4CXX_DEBUG(dashboard_logger, "Executor stopped successfully in "
                        << stop_result.elapsed_time_ms << " ms.  ");

        graph::ExecutionResult join_result = executor->Join();
        if (!join_result.success) {
            LOG4CXX_ERROR(dashboard_logger, "Executor join failed: " << join_result.message);
            return 1;
        }
        LOG4CXX_DEBUG(dashboard_logger, "Executor joined successfully in "
                        << join_result.elapsed_time_ms << " ms.  ");
   
        // Determine exit code based on results
        return join_result.success ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
