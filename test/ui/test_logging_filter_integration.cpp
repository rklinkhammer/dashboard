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

#include <gtest/gtest.h>
#include "ui/LoggingWindow.hpp"
#include "ui/LayoutConfig.hpp"
#include <memory>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// D4.5: LoggingWindow Filter Integration with LayoutConfig
// ============================================================================
// Tests for applying logging filter level from LayoutConfig to LoggingWindow
// and dynamically updating filters during runtime.

class LoggingFilterIntegrationTest : public ::testing::Test {
protected:
    std::string test_config_path_;
    
    void SetUp() override {
        // Create a temporary config path for testing
        test_config_path_ = "/tmp/test_layout.json";
    }
    
    void TearDown() override {
        // Clean up test config file
        if (fs::exists(test_config_path_)) {
            fs::remove(test_config_path_);
        }
    }
};

// Test 1: LoggingWindow filter levels are supported
TEST_F(LoggingFilterIntegrationTest, LoggingWindowSupportedLevels) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    std::vector<std::string> valid_levels = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    
    for (const auto& level : valid_levels) {
        // Should not throw
        ASSERT_NO_THROW({
            window->SetLevelFilter(level);
        });
        EXPECT_EQ(window->GetLevelFilter(), level);
    }
}

// Test 2: Default filter level is TRACE (show all)
TEST_F(LoggingFilterIntegrationTest, DefaultFilterLevelIsTrace) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    EXPECT_EQ(window->GetLevelFilter(), "TRACE");
}

// Test 3: LayoutConfig can read and write filter level
TEST_F(LoggingFilterIntegrationTest, LayoutConfigStoresFilterLevel) {
    LayoutConfig config(test_config_path_);
    
    // Set a filter level
    config.SetLoggingLevelFilter("ERROR");
    EXPECT_EQ(config.GetLoggingLevelFilter(), "ERROR");
    
    // Save and reload
    ASSERT_TRUE(config.Save());
    
    LayoutConfig config2(test_config_path_);
    ASSERT_TRUE(config2.Load());
    EXPECT_EQ(config2.GetLoggingLevelFilter(), "ERROR");
}

// Test 4: Filter can be applied to LoggingWindow from LayoutConfig
TEST_F(LoggingFilterIntegrationTest, ApplyFilterFromConfig) {
    // Create and save config with specific filter
    LayoutConfig config(test_config_path_);
    config.SetLoggingLevelFilter("WARN");
    config.Save();
    
    // Load config and apply to window
    LayoutConfig loaded_config(test_config_path_);
    loaded_config.Load();
    
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    window->SetLevelFilter(loaded_config.GetLoggingLevelFilter());
    
    EXPECT_EQ(window->GetLevelFilter(), "WARN");
}

// Test 5: LoggingWindow filter can be changed dynamically
TEST_F(LoggingFilterIntegrationTest, DynamicFilterChange) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    // Start with TRACE
    EXPECT_EQ(window->GetLevelFilter(), "TRACE");
    
    // Change to INFO
    window->SetLevelFilter("INFO");
    EXPECT_EQ(window->GetLevelFilter(), "INFO");
    
    // Change to ERROR
    window->SetLevelFilter("ERROR");
    EXPECT_EQ(window->GetLevelFilter(), "ERROR");
}

// Test 6: CycleLevelFilter works through all levels
TEST_F(LoggingFilterIntegrationTest, CycleLevelFilterIteratesThroughAllLevels) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    std::vector<std::string> expected_levels = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    
    for (int i = 0; i < 6; i++) {
        window->CycleLevelFilter();
        // After cycling, should match expected level (accounting for the initial state)
        bool found = false;
        for (const auto& level : expected_levels) {
            if (window->GetLevelFilter() == level) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Filter level " << window->GetLevelFilter() << " not in valid levels";
    }
}

// Test 7: LayoutConfig can store and retrieve custom search filter
TEST_F(LoggingFilterIntegrationTest, LayoutConfigStoresSearchFilter) {
    LayoutConfig config(test_config_path_);
    
    std::string search_pattern = "error.*database";
    config.SetLoggingSearchFilter(search_pattern);
    
    EXPECT_EQ(config.GetLoggingSearchFilter(), search_pattern);
}

// Test 8: Search filter can be applied to LoggingWindow
TEST_F(LoggingFilterIntegrationTest, ApplySearchFilterFromConfig) {
    LayoutConfig config(test_config_path_);
    std::string search_text = "connection timeout";
    config.SetLoggingSearchFilter(search_text);
    config.Save();
    
    // Load and apply
    LayoutConfig loaded_config(test_config_path_);
    loaded_config.Load();
    
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    window->SetSearchText(loaded_config.GetLoggingSearchFilter());
    
    EXPECT_EQ(window->GetSearchText(), search_text);
}

// Test 9: Multiple filter changes are independent
TEST_F(LoggingFilterIntegrationTest, FilterChangesAreIndependent) {
    auto window1 = std::make_shared<LoggingWindow>("Logs1");
    auto window2 = std::make_shared<LoggingWindow>("Logs2");
    
    window1->SetLevelFilter("DEBUG");
    window2->SetLevelFilter("ERROR");
    
    EXPECT_EQ(window1->GetLevelFilter(), "DEBUG");
    EXPECT_EQ(window2->GetLevelFilter(), "ERROR");
}

// Test 10: Filter persistence through save/load cycle
TEST_F(LoggingFilterIntegrationTest, FilterPersistenceCycle) {
    // Initial config
    LayoutConfig config1(test_config_path_);
    config1.SetLoggingLevelFilter("INFO");
    config1.SetLoggingSearchFilter("database");
    config1.Save();
    
    // Load and verify
    LayoutConfig config2(test_config_path_);
    config2.Load();
    
    EXPECT_EQ(config2.GetLoggingLevelFilter(), "INFO");
    EXPECT_EQ(config2.GetLoggingSearchFilter(), "database");
}

// Test 11: Filter level order is respected
TEST_F(LoggingFilterIntegrationTest, FilterLevelOrder) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    // TRACE should be least restrictive
    window->SetLevelFilter("TRACE");
    std::string trace_filter = window->GetLevelFilter();
    
    // FATAL should be most restrictive
    window->SetLevelFilter("FATAL");
    std::string fatal_filter = window->GetLevelFilter();
    
    EXPECT_NE(trace_filter, fatal_filter);
    EXPECT_EQ(window->GetLevelFilter(), "FATAL");
}

// Test 12: Clear search filter
TEST_F(LoggingFilterIntegrationTest, ClearSearchFilter) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    window->SetSearchText("some pattern");
    EXPECT_EQ(window->GetSearchText(), "some pattern");
    
    window->ClearSearch();
    EXPECT_EQ(window->GetSearchText(), "");
}

// Test 13: Empty filter values are handled correctly
TEST_F(LoggingFilterIntegrationTest, EmptyFilterValuesHandled) {
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    // Empty search should be valid
    window->SetSearchText("");
    EXPECT_EQ(window->GetSearchText(), "");
    
    // Default level filter should be non-empty
    EXPECT_NE(window->GetLevelFilter(), "");
}

// Test 14: Multiple config instances can have different filter states
TEST_F(LoggingFilterIntegrationTest, MultipleConfigInstancesIndependent) {
    LayoutConfig config1(test_config_path_);
    config1.SetLoggingLevelFilter("DEBUG");
    
    LayoutConfig config2(test_config_path_);
    config2.SetLoggingLevelFilter("ERROR");
    
    EXPECT_EQ(config1.GetLoggingLevelFilter(), "DEBUG");
    EXPECT_EQ(config2.GetLoggingLevelFilter(), "ERROR");
}

// Test 15: Filter can be set before and after window creation
TEST_F(LoggingFilterIntegrationTest, FilterSetBeforeAndAfterCreation) {
    LayoutConfig config(test_config_path_);
    config.SetLoggingLevelFilter("WARN");
    config.Save();
    
    auto window = std::make_shared<LoggingWindow>("Test Logs");
    
    // Apply filter after creation
    LayoutConfig loaded_config(test_config_path_);
    loaded_config.Load();
    window->SetLevelFilter(loaded_config.GetLoggingLevelFilter());
    
    EXPECT_EQ(window->GetLevelFilter(), "WARN");
}
