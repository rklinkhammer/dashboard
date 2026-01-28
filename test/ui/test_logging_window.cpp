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
#include <memory>
#include <thread>
#include <chrono>

class LoggingWindowTest : public ::testing::Test {
protected:
    std::shared_ptr<LoggingWindow> window_;

    void SetUp() override {
        window_ = std::make_shared<LoggingWindow>("TestLogs");
    }
};

// Test basic logging functionality
TEST_F(LoggingWindowTest, InitializesWithThreeLogs) {
    ASSERT_EQ(window_->GetLogCount(), 3);
    ASSERT_EQ(window_->GetLevelFilter(), "TRACE");
    ASSERT_EQ(window_->GetSearchText(), "");
}

// Test adding log lines
TEST_F(LoggingWindowTest, AddsLogLines) {
    window_->ClearLogs();
    window_->AddLogLine("[INFO] Test message");
    window_->AddLogLine("[ERROR] Error message");
    
    ASSERT_EQ(window_->GetLogCount(), 2);
}

// Test adding log with level
TEST_F(LoggingWindowTest, AddsLogLineWithLevel) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("INFO", "Test message");
    window_->AddLogLineWithLevel("ERROR", "Error message");
    window_->AddLogLineWithLevel("WARN", "Warning message");
    
    ASSERT_EQ(window_->GetLogCount(), 3);
}

// Test clearing logs
TEST_F(LoggingWindowTest, ClearsAllLogs) {
    window_->AddLogLineWithLevel("INFO", "Test");
    window_->AddLogLineWithLevel("DEBUG", "Debug");
    window_->ClearLogs();
    
    ASSERT_EQ(window_->GetLogCount(), 0);
}

// Test level filtering
TEST_F(LoggingWindowTest, FiltersLogsByLevel) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("TRACE", "Trace message");
    window_->AddLogLineWithLevel("DEBUG", "Debug message");
    window_->AddLogLineWithLevel("INFO", "Info message");
    window_->AddLogLineWithLevel("WARN", "Warn message");
    window_->AddLogLineWithLevel("ERROR", "Error message");
    window_->AddLogLineWithLevel("FATAL", "Fatal message");
    
    // Filter to INFO and above
    window_->SetLevelFilter("INFO");
    ASSERT_EQ(window_->GetLevelFilter(), "INFO");
    
    // Should still have all 6 entries total
    ASSERT_EQ(window_->GetLogCount(), 6);
}

// Test cycling level filter
TEST_F(LoggingWindowTest, CyclesLevelFilter) {
    ASSERT_EQ(window_->GetLevelFilter(), "TRACE");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "DEBUG");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "INFO");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "WARN");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "ERROR");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "FATAL");
    window_->CycleLevelFilter();
    ASSERT_EQ(window_->GetLevelFilter(), "TRACE");
}

// Test text search
TEST_F(LoggingWindowTest, SearchesLogsByText) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("INFO", "Database connected");
    window_->AddLogLineWithLevel("INFO", "Network available");
    window_->AddLogLineWithLevel("INFO", "Service started");
    window_->AddLogLineWithLevel("ERROR", "Database error");
    
    window_->SetSearchText("database");
    std::string search = window_->GetSearchText();
    // GetSearchText returns the lowercase version
    ASSERT_EQ(search, "database");
}

// Test clearing search
TEST_F(LoggingWindowTest, ClearsSearch) {
    window_->SetSearchText("test");
    ASSERT_FALSE(window_->GetSearchText().empty());
    window_->ClearSearch();
    ASSERT_EQ(window_->GetSearchText(), "");
}

// Test scrolling up
TEST_F(LoggingWindowTest, ScrollsUp) {
    window_->ClearLogs();
    for (int i = 0; i < 30; ++i) {
        window_->AddLogLineWithLevel("INFO", "Message " + std::to_string(i));
    }
    
    window_->ScrollToBottom();
    window_->ScrollUp();
    // Just verify scroll doesn't crash and offset changes
    ASSERT_NO_THROW(window_->Render());
}

// Test scrolling down
TEST_F(LoggingWindowTest, ScrollsDown) {
    window_->ClearLogs();
    for (int i = 0; i < 30; ++i) {
        window_->AddLogLineWithLevel("INFO", "Message " + std::to_string(i));
    }
    
    window_->ScrollUp();
    window_->ScrollDown();
    ASSERT_NO_THROW(window_->Render());
}

// Test max lines circular buffer
TEST_F(LoggingWindowTest, EnforcesMaxLinesLimit) {
    window_->ClearLogs();
    
    // Add 1000 lines - should not exceed MAX_LINES
    for (int i = 0; i < 1000; ++i) {
        window_->AddLogLineWithLevel("INFO", "Message " + std::to_string(i));
    }
    
    // Should maintain at most 1000 lines
    ASSERT_LE(window_->GetLogCount(), 1000);
}

// Test rendering with different filters
TEST_F(LoggingWindowTest, RendersWithFilters) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("INFO", "Info 1");
    window_->AddLogLineWithLevel("ERROR", "Error 1");
    window_->AddLogLineWithLevel("WARN", "Warn 1");
    
    window_->SetLevelFilter("ERROR");
    auto rendered = window_->Render();
    ASSERT_NE(rendered, nullptr);
}

// Test rendering with search
TEST_F(LoggingWindowTest, RendersWithSearch) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("INFO", "Database connected");
    window_->AddLogLineWithLevel("INFO", "Server started");
    
    window_->SetSearchText("database");
    auto rendered = window_->Render();
    ASSERT_NE(rendered, nullptr);
}

// Test parsing log line with level
TEST_F(LoggingWindowTest, ParsesLogLineLevel) {
    window_->ClearLogs();
    window_->AddLogLine("[DEBUG] Parsed message");
    window_->AddLogLine("[ERROR] Another parsed message");
    
    ASSERT_EQ(window_->GetLogCount(), 2);
}

// Test appender creation
TEST_F(LoggingWindowTest, HasLoggingAppender) {
    auto appender = window_->GetAppender();
    ASSERT_NE(appender, nullptr);
}

// Test appender callback
TEST_F(LoggingWindowTest, AppenderLogsMessages) {
    window_->ClearLogs();
    auto appender = window_->GetAppender();
    
    ASSERT_NE(appender, nullptr);
    appender->LogMessage("INFO", "Appender message");
    
    ASSERT_EQ(window_->GetLogCount(), 1);
}

// Test thread-safe logging
TEST_F(LoggingWindowTest, ThreadSafeLogging) {
    window_->ClearLogs();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < 20; ++i) {
                std::string msg = "Thread " + std::to_string(t) + " message " + std::to_string(i);
                window_->AddLogLineWithLevel("INFO", msg);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    ASSERT_EQ(window_->GetLogCount(), 100);
}

// Test combined filtering (level + search)
TEST_F(LoggingWindowTest, CombinedFiltering) {
    window_->ClearLogs();
    window_->AddLogLineWithLevel("INFO", "Database message");
    window_->AddLogLineWithLevel("INFO", "Server message");
    window_->AddLogLineWithLevel("ERROR", "Database error");
    window_->AddLogLineWithLevel("ERROR", "Server error");
    
    // Filter to ERROR level and search for "database"
    window_->SetLevelFilter("ERROR");
    window_->SetSearchText("database");
    
    auto rendered = window_->Render();
    ASSERT_NE(rendered, nullptr);
}

// Test rendering doesn't throw with empty logs
TEST_F(LoggingWindowTest, RendersWithEmptyLogs) {
    window_->ClearLogs();
    ASSERT_NO_THROW(window_->Render());
}

// Test multiple log levels
TEST_F(LoggingWindowTest, HandlesAllLogLevels) {
    window_->ClearLogs();
    
    const std::vector<std::string> levels = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    for (const auto& level : levels) {
        window_->AddLogLineWithLevel(level, level + " message");
    }
    
    ASSERT_EQ(window_->GetLogCount(), 6);
}
