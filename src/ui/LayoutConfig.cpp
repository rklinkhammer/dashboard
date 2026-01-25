#include "ui/LayoutConfig.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <algorithm>

namespace fs = std::filesystem;

LayoutConfig::LayoutConfig(const std::string& config_path)
    : config_path_(config_path.empty() ? GetDefaultConfigPath() : config_path) {
    InitializeDefaults();
    std::cerr << "[LayoutConfig] Initialized with path: " << config_path_ << "\n";
}

void LayoutConfig::InitializeDefaults() {
    config_["window_heights"] = {
        {"metrics", DEFAULT_METRICS_HEIGHT},
        {"logging", DEFAULT_LOGGING_HEIGHT},
        {"command", DEFAULT_COMMAND_HEIGHT},
        {"status", DEFAULT_STATUS_HEIGHT}
    };
    
    config_["logging"] = {
        {"level_filter", "TRACE"},
        {"search_filter", ""}
    };
    
    config_["command"] = {
        {"history", json::array()}
    };
    
    config_["scroll"] = {
        {"logging_offset", 0}
    };
    
    config_["metadata"] = {
        {"version", "1.0"},
        {"last_modified", ""}
    };
}

std::string LayoutConfig::GetDefaultConfigPath() const {
    // Get home directory
    const char* home = std::getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }
    
    if (!home) {
        // Fallback to current directory
        return ".gdashboard/layout.json";
    }
    
    return std::string(home) + "/.gdashboard/layout.json";
}

bool LayoutConfig::EnsureConfigDirectory() const {
    try {
        fs::path config_dir = fs::path(config_path_).parent_path();
        if (!fs::exists(config_dir)) {
            fs::create_directories(config_dir);
            std::cerr << "[LayoutConfig] Created directory: " << config_dir << "\n";
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error creating directory: " << e.what() << "\n";
        return false;
    }
}

bool LayoutConfig::Load() {
    try {
        if (fs::exists(config_path_)) {
            std::ifstream file(config_path_);
            if (!file.is_open()) {
                std::cerr << "[LayoutConfig] Failed to open: " << config_path_ << "\n";
                return false;
            }
            
            file >> config_;
            file.close();
            
            // Validate loaded configuration
            if (!IsValid()) {
                std::cerr << "[LayoutConfig] Loaded config failed validation, reinitializing\n";
                InitializeDefaults();
                return false;
            }
            
            std::cerr << "[LayoutConfig] Successfully loaded: " << config_path_ << "\n";
            return true;
        } else {
            std::cerr << "[LayoutConfig] Config file not found: " << config_path_ << "\n";
            return false;  // File doesn't exist, use defaults
        }
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error loading config: " << e.what() << "\n";
        InitializeDefaults();
        return false;
    }
}

bool LayoutConfig::Save() {
    try {
        if (!EnsureConfigDirectory()) {
            return false;
        }
        
        // Update timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        config_["metadata"]["last_modified"] = std::ctime(&time);
        
        std::ofstream file(config_path_);
        if (!file.is_open()) {
            std::cerr << "[LayoutConfig] Failed to open for writing: " << config_path_ << "\n";
            return false;
        }
        
        file << config_.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        std::cerr << "[LayoutConfig] Successfully saved: " << config_path_ << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error saving config: " << e.what() << "\n";
        return false;
    }
}

std::vector<std::string> LayoutConfig::GetCommandHistory() const {
    std::vector<std::string> history;
    try {
        if (config_.contains("command") && config_["command"].contains("history")) {
            for (const auto& cmd : config_["command"]["history"]) {
                history.push_back(cmd.get<std::string>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error reading command history: " << e.what() << "\n";
    }
    return history;
}

void LayoutConfig::AddCommandToHistory(const std::string& command) {
    try {
        auto& history = config_["command"]["history"];
        
        // Remove if already exists (avoid duplicates at different positions)
        auto it = std::find(history.begin(), history.end(), command);
        if (it != history.end()) {
            history.erase(it);
        }
        
        // Add to front
        history.insert(history.begin(), command);
        
        // Trim to MAX_HISTORY
        while (history.size() > MAX_HISTORY) {
            history.erase(history.end() - 1);
        }
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error adding to command history: " << e.what() << "\n";
    }
}

void LayoutConfig::ClearCommandHistory() {
    try {
        config_["command"]["history"] = json::array();
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error clearing command history: " << e.what() << "\n";
    }
}

bool LayoutConfig::IsValid() const {
    try {
        // Check window heights sum to 100
        if (!ValidateHeights()) {
            return false;
        }
        
        // Check logging filter is valid
        const std::vector<std::string> valid_levels = {
            "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
        };
        std::string level = GetLoggingLevelFilter();
        if (std::find(valid_levels.begin(), valid_levels.end(), level) == valid_levels.end()) {
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LayoutConfig] Error validating config: " << e.what() << "\n";
        return false;
    }
}

bool LayoutConfig::ValidateHeights() const {
    try {
        int total = GetMetricsHeightPercent() + GetLoggingHeightPercent() +
                   GetCommandHeightPercent() + GetStatusHeightPercent();
        return total == 100;
    } catch (const std::exception&) {
        return false;
    }
}

std::string LayoutConfig::ToString() const {
    return config_.dump(2);
}
