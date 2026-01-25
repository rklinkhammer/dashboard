#include <gtest/gtest.h>
#include "ui/LayoutConfig.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;

class LayoutConfigTest : public ::testing::Test {
protected:
    std::string test_config_path_;

    void SetUp() override {
        // Create a temporary test config file in /tmp
        test_config_path_ = "/tmp/test_layout_" + std::to_string(std::time(nullptr)) + ".json";
    }

    void TearDown() override {
        // Clean up test file
        if (fs::exists(test_config_path_)) {
            fs::remove(test_config_path_);
        }
    }

    LayoutConfig CreateTestConfig() {
        return LayoutConfig(test_config_path_);
    }
};

// Test initialization with defaults
TEST_F(LayoutConfigTest, InitializesWithDefaults) {
    LayoutConfig config(test_config_path_);
    
    ASSERT_EQ(config.GetMetricsHeightPercent(), 40);
    ASSERT_EQ(config.GetLoggingHeightPercent(), 35);
    ASSERT_EQ(config.GetCommandHeightPercent(), 23);
    ASSERT_EQ(config.GetStatusHeightPercent(), 2);
}

// Test default logging filter
TEST_F(LayoutConfigTest, DefaultLoggingFilter) {
    LayoutConfig config(test_config_path_);
    ASSERT_EQ(config.GetLoggingLevelFilter(), "TRACE");
}

// Test empty search filter by default
TEST_F(LayoutConfigTest, DefaultSearchFilter) {
    LayoutConfig config(test_config_path_);
    ASSERT_EQ(config.GetLoggingSearchFilter(), "");
}

// Test saving configuration
TEST_F(LayoutConfigTest, SavesConfiguration) {
    LayoutConfig config(test_config_path_);
    config.SetLoggingHeight(40);
    config.SetCommandHeight(20);
    config.SetLoggingLevelFilter("INFO");
    
    ASSERT_TRUE(config.Save());
    ASSERT_TRUE(fs::exists(test_config_path_));
}

// Test loading configuration
TEST_F(LayoutConfigTest, LoadsConfiguration) {
    // First save
    {
        LayoutConfig config1(test_config_path_);
        config1.SetLoggingHeight(40);
        config1.SetCommandHeight(18);  // 40+40+18+2 = 100
        config1.SetLoggingLevelFilter("ERROR");
        config1.Save();
    }
    
    // Then load
    LayoutConfig config2(test_config_path_);
    ASSERT_TRUE(config2.Load());
    
    ASSERT_EQ(config2.GetLoggingHeightPercent(), 40);
    ASSERT_EQ(config2.GetCommandHeightPercent(), 18);
    ASSERT_EQ(config2.GetLoggingLevelFilter(), "ERROR");
}

// Test setting window heights
TEST_F(LayoutConfigTest, SetsWindowHeights) {
    LayoutConfig config(test_config_path_);
    
    config.SetMetricsHeight(35);
    config.SetLoggingHeight(40);
    config.SetCommandHeight(23);
    
    ASSERT_EQ(config.GetMetricsHeightPercent(), 35);
    ASSERT_EQ(config.GetLoggingHeightPercent(), 40);
    ASSERT_EQ(config.GetCommandHeightPercent(), 23);
}

// Test validation of valid config
TEST_F(LayoutConfigTest, ValidatesValidConfig) {
    LayoutConfig config(test_config_path_);
    ASSERT_TRUE(config.IsValid());
}

// Test validation of invalid heights
TEST_F(LayoutConfigTest, ValidatesInvalidHeights) {
    LayoutConfig config(test_config_path_);
    config.SetMetricsHeight(50);
    config.SetLoggingHeight(40);
    config.SetCommandHeight(20);
    
    // Sum is 110 (metrics 50 + logging 40 + command 20 + status 2)
    ASSERT_FALSE(config.IsValid());
}

// Test logging level filter
TEST_F(LayoutConfigTest, SetsLoggingLevelFilter) {
    LayoutConfig config(test_config_path_);
    
    const std::vector<std::string> levels = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    for (const auto& level : levels) {
        config.SetLoggingLevelFilter(level);
        ASSERT_EQ(config.GetLoggingLevelFilter(), level);
    }
}

// Test logging search filter
TEST_F(LayoutConfigTest, SetsLoggingSearchFilter) {
    LayoutConfig config(test_config_path_);
    config.SetLoggingSearchFilter("database");
    
    ASSERT_EQ(config.GetLoggingSearchFilter(), "database");
}

// Test command history - adding commands
TEST_F(LayoutConfigTest, AddsCommandToHistory) {
    LayoutConfig config(test_config_path_);
    
    config.AddCommandToHistory("status");
    config.AddCommandToHistory("clear");
    
    auto history = config.GetCommandHistory();
    ASSERT_EQ(history.size(), 2);
    ASSERT_EQ(history[0], "clear");  // Most recent first
    ASSERT_EQ(history[1], "status");
}

// Test command history - max size
TEST_F(LayoutConfigTest, EnforcesMaxCommandHistory) {
    LayoutConfig config(test_config_path_);
    
    // Add 15 commands (max is 10)
    for (int i = 0; i < 15; ++i) {
        config.AddCommandToHistory("cmd_" + std::to_string(i));
    }
    
    auto history = config.GetCommandHistory();
    ASSERT_LE(history.size(), 10);
    ASSERT_EQ(history[0], "cmd_14");  // Most recent
}

// Test command history - deduplication
TEST_F(LayoutConfigTest, DeduplicatesCommandHistory) {
    LayoutConfig config(test_config_path_);
    
    config.AddCommandToHistory("status");
    config.AddCommandToHistory("run");
    config.AddCommandToHistory("status");  // Add again
    
    auto history = config.GetCommandHistory();
    ASSERT_EQ(history.size(), 2);
    ASSERT_EQ(history[0], "status");  // Moved to front
    ASSERT_EQ(history[1], "run");
}

// Test clearing command history
TEST_F(LayoutConfigTest, ClearsCommandHistory) {
    LayoutConfig config(test_config_path_);
    
    config.AddCommandToHistory("status");
    config.AddCommandToHistory("run");
    config.ClearCommandHistory();
    
    auto history = config.GetCommandHistory();
    ASSERT_TRUE(history.empty());
}

// Test scroll offset
TEST_F(LayoutConfigTest, SetsScrollOffset) {
    LayoutConfig config(test_config_path_);
    config.SetLoggingScrollOffset(42);
    
    ASSERT_EQ(config.GetLoggingScrollOffset(), 42);
}

// Test full save/load cycle with all features
TEST_F(LayoutConfigTest, FullSaveLoadCycle) {
    // Save with custom values
    {
        LayoutConfig config1(test_config_path_);
        config1.SetLoggingHeight(40);
        config1.SetCommandHeight(18);  // 40+40+18+2 = 100
        config1.SetLoggingLevelFilter("ERROR");
        config1.SetLoggingSearchFilter("network");
        config1.AddCommandToHistory("run");
        config1.AddCommandToHistory("status");
        config1.SetLoggingScrollOffset(15);
        
        ASSERT_TRUE(config1.Save());
    }
    
    // Load and verify
    {
        LayoutConfig config2(test_config_path_);
        ASSERT_TRUE(config2.Load());
        
        ASSERT_EQ(config2.GetLoggingHeightPercent(), 40);
        ASSERT_EQ(config2.GetCommandHeightPercent(), 18);
        ASSERT_EQ(config2.GetLoggingLevelFilter(), "ERROR");
        ASSERT_EQ(config2.GetLoggingSearchFilter(), "network");
        ASSERT_EQ(config2.GetLoggingScrollOffset(), 15);
        
        auto history = config2.GetCommandHistory();
        ASSERT_EQ(history.size(), 2);
        ASSERT_EQ(history[0], "status");
        ASSERT_EQ(history[1], "run");
    }
}

// Test to_string for debugging
TEST_F(LayoutConfigTest, PrintsConfiguration) {
    LayoutConfig config(test_config_path_);
    config.SetLoggingHeight(40);
    config.SetLoggingLevelFilter("INFO");
    
    std::string output = config.ToString();
    ASSERT_TRUE(output.find("40") != std::string::npos);
    ASSERT_TRUE(output.find("INFO") != std::string::npos);
}

// Test loading non-existent file doesn't crash
TEST_F(LayoutConfigTest, HandlesNonexistentFile) {
    LayoutConfig config(test_config_path_);
    ASSERT_FALSE(config.Load());  // File doesn't exist
    
    // Should still have defaults
    ASSERT_EQ(config.GetMetricsHeightPercent(), 40);
}

// Test invalid JSON in config file
TEST_F(LayoutConfigTest, HandlesInvalidJSON) {
    // Create invalid JSON file
    std::ofstream file(test_config_path_);
    file << "{ invalid json }";
    file.close();
    
    LayoutConfig config(test_config_path_);
    ASSERT_FALSE(config.Load());
    
    // Should revert to defaults
    ASSERT_EQ(config.GetMetricsHeightPercent(), 40);
}

// Test config path getter
TEST_F(LayoutConfigTest, ReturnsConfigPath) {
    LayoutConfig config(test_config_path_);
    ASSERT_EQ(config.GetConfigPath(), test_config_path_);
}

// Test raw JSON access for advanced usage
TEST_F(LayoutConfigTest, ProvidesRawJSON) {
    LayoutConfig config(test_config_path_);
    const auto& raw = config.GetRawConfig();
    
    ASSERT_TRUE(raw.contains("window_heights"));
    ASSERT_TRUE(raw.contains("logging"));
    ASSERT_TRUE(raw.contains("command"));
}

// Test validation of logging filter values
TEST_F(LayoutConfigTest, ValidatesLoggingFilterValues) {
    LayoutConfig config(test_config_path_);
    
    // Valid values
    config.SetLoggingLevelFilter("TRACE");
    ASSERT_TRUE(config.IsValid());
    
    config.SetLoggingLevelFilter("DEBUG");
    ASSERT_TRUE(config.IsValid());
    
    config.SetLoggingLevelFilter("INFO");
    ASSERT_TRUE(config.IsValid());
    
    config.SetLoggingLevelFilter("WARN");
    ASSERT_TRUE(config.IsValid());
    
    config.SetLoggingLevelFilter("ERROR");
    ASSERT_TRUE(config.IsValid());
    
    config.SetLoggingLevelFilter("FATAL");
    ASSERT_TRUE(config.IsValid());
}

// Test preserve metadata after load
TEST_F(LayoutConfigTest, PreservesMetadata) {
    {
        LayoutConfig config1(test_config_path_);
        config1.Save();
    }
    
    LayoutConfig config2(test_config_path_);
    config2.Load();
    
    const auto& raw = config2.GetRawConfig();
    ASSERT_TRUE(raw.contains("metadata"));
    ASSERT_TRUE(raw["metadata"].contains("version"));
    ASSERT_EQ(raw["metadata"]["version"].get<std::string>(), "1.0");
}

// Test multiple sequential saves
TEST_F(LayoutConfigTest, HandlesMultipleSaves) {
    LayoutConfig config(test_config_path_);
    
    for (int i = 0; i < 5; ++i) {
        config.SetLoggingHeight(35 + i);
        config.SetCommandHeight(23 - i);  // Keep sum at 100
        ASSERT_TRUE(config.Save());
        ASSERT_TRUE(fs::exists(test_config_path_));
    }
    
    // Final state should be logging 39, command 19
    LayoutConfig config2(test_config_path_);
    config2.Load();
    ASSERT_EQ(config2.GetLoggingHeightPercent(), 39);
    ASSERT_EQ(config2.GetCommandHeightPercent(), 19);
}
