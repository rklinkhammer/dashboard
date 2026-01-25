#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * LayoutConfig: Persists dashboard layout preferences to JSON
 * 
 * Saves/restores:
 * - Window heights (metrics %, logging %, command %)
 * - Logging level filter (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Command history (up to 10 recent commands)
 * - Search filters for various windows
 * 
 * Storage location: ~/.gdashboard/layout.json
 */
class LayoutConfig {
public:
    // Constructor with optional custom path for testing
    explicit LayoutConfig(const std::string& config_path = "");

    // Load configuration from disk
    bool Load();

    // Save configuration to disk
    bool Save();

    // Window height configuration
    int GetMetricsHeightPercent() const { return config_["window_heights"]["metrics"].get<int>(); }
    int GetLoggingHeightPercent() const { return config_["window_heights"]["logging"].get<int>(); }
    int GetCommandHeightPercent() const { return config_["window_heights"]["command"].get<int>(); }
    int GetStatusHeightPercent() const { return config_["window_heights"]["status"].get<int>(); }

    void SetMetricsHeight(int percent) { config_["window_heights"]["metrics"] = percent; }
    void SetLoggingHeight(int percent) { config_["window_heights"]["logging"] = percent; }
    void SetCommandHeight(int percent) { config_["window_heights"]["command"] = percent; }

    // Logging filter
    std::string GetLoggingLevelFilter() const {
        return config_["logging"]["level_filter"].get<std::string>();
    }
    void SetLoggingLevelFilter(const std::string& level) {
        config_["logging"]["level_filter"] = level;
    }

    // Logging search filter
    std::string GetLoggingSearchFilter() const {
        return config_["logging"]["search_filter"].get<std::string>();
    }
    void SetLoggingSearchFilter(const std::string& search) {
        config_["logging"]["search_filter"] = search;
    }

    // Command history
    std::vector<std::string> GetCommandHistory() const;
    void AddCommandToHistory(const std::string& command);
    void ClearCommandHistory();

    // Scroll positions
    int GetLoggingScrollOffset() const {
        return config_["scroll"]["logging_offset"].get<int>();
    }
    void SetLoggingScrollOffset(int offset) {
        config_["scroll"]["logging_offset"] = offset;
    }

    // Validation
    bool IsValid() const;

    // Get configuration path
    std::string GetConfigPath() const { return config_path_; }

    // Get raw JSON for testing
    const json& GetRawConfig() const { return config_; }

    // Pretty print configuration
    std::string ToString() const;

private:
    json config_;
    std::string config_path_;
    static constexpr size_t MAX_HISTORY = 10;
    static constexpr int DEFAULT_METRICS_HEIGHT = 40;
    static constexpr int DEFAULT_LOGGING_HEIGHT = 35;
    static constexpr int DEFAULT_COMMAND_HEIGHT = 23;
    static constexpr int DEFAULT_STATUS_HEIGHT = 2;

    // Initialize config with defaults
    void InitializeDefaults();

    // Get the default config directory
    std::string GetDefaultConfigPath() const;

    // Ensure config directory exists
    bool EnsureConfigDirectory() const;

    // Validate window height percentages
    bool ValidateHeights() const;
};
