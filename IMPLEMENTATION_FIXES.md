# Implementation Plan: Fix Critical MVP Gaps

**Date**: January 25, 2026  
**Status**: Planning  
**Target**: Complete working event loop and full-screen layout by end of sprint

---

## Overview

This plan addresses the 4 critical gaps preventing MVP deployment:
1. Height constraints not applied to FTXUI elements
2. Event loop exits after 30 frames; no resize/input handling
3. LoggingAppender not connected to LOG4CXX pipeline
4. CommandRegistry not integrated with CommandWindow

**Execution Strategy**: Fix in dependency order (event loop first, then others)

---

## Phase 1: Implement Real Event Loop (CRITICAL PATH)

**Objective**: Replace hard-coded 30-frame exit with real event loop that handles terminal events

**Why First**: All other fixes depend on a working event loop

### 1.1: Analyze FTXUI Event System

**Files to Review**:
- Check how FTXUI's `GetEvent()` works
- Understand `Event::Character()`, `Event::Resize()`, `Event::CtrlC()`
- Review FTXUI's loop pattern (if any examples exist)

**Key Questions**:
- How to read stdin events without blocking?
- How to detect terminal resize?
- What event types are available?

**Acceptance Criteria**:
- [ ] Document FTXUI event API usage
- [ ] Create example event loop that reads one event

### 1.2: Implement Event Reading Infrastructure

**Files to Modify**:
- `include/ui/Dashboard.hpp` - Add event handling methods
- `src/ui/Dashboard.cpp` - Implement event loop logic
- `include/ui/Window.hpp` - Add event handling interface (if needed)

**Changes Required**:

**1.2.1: Dashboard.hpp - Add event loop support**
```cpp
class Dashboard {
private:
    // Event loop management
    std::atomic<bool> should_exit_{false};
    std::chrono::high_resolution_clock::time_point last_frame_;
    std::chrono::milliseconds frame_time_{33};  // 30 FPS
    
    // Screen management
    int last_screen_width_ = -1;
    int last_screen_height_ = -1;
    
    // Event handlers
    void HandleKeyEvent(const ftxui::Event& event);
    void HandleResizeEvent();
    bool CheckForTerminalResize();
    void RecalculateLayout();
    
public:
    // New public methods for event testing
    bool AreHeightsValid() const;
    int GetScreenHeight() const;
    int GetScreenWidth() const;
};
```

**1.2.2: Dashboard.cpp - Implement real event loop**

Replace `Run()` method (lines 197-245):
```cpp
void Dashboard::Run() {
    if (!initialized_) {
        throw std::runtime_error("Dashboard not initialized");
    }

    LOG4CXX_INFO(dashboard_logger, "Starting event loop (30 FPS, terminal input enabled)");

    using namespace ftxui;

    // Enable raw mode for keyboard input
    auto screen = Screen::Create(Dimension::Full(), Dimension::Full());
    
    // Store initial dimensions
    last_screen_width_ = screen.dimx();
    last_screen_height_ = screen.dimy();
    
    // Main event loop
    auto last_frame = std::chrono::high_resolution_clock::now();
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

            // 2. Read and process events (non-blocking)
            if (auto event = GetEvent()) {
                HandleKeyEvent(*event);
            }

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
                now - last_frame);

            if (elapsed < frame_time_) {
                std::this_thread::sleep_for(frame_time_ - elapsed);
            }
            last_frame = std::chrono::high_resolution_clock::now();

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
    
    // Tab navigation (left/right arrows)
    if (metrics_panel_ && metrics_panel_->IsTabModeEnabled()) {
        if (event == Event::ArrowLeft) {
            metrics_panel_->PreviousTab();
            LOG4CXX_DEBUG(dashboard_logger, "Navigated to previous tab");
            return;
        }
        if (event == Event::ArrowRight) {
            metrics_panel_->NextTab();
            LOG4CXX_DEBUG(dashboard_logger, "Navigated to next tab");
            return;
        }
    }
    
    // Route to CommandWindow for input
    if (command_window_) {
        // Characters go to command input
        if (event.is_character()) {
            command_window_->AddInputCharacter(event.character()[0]);
            return;
        }
        
        // Enter executes command
        if (event == Event::Return) {
            command_window_->ExecuteInputCommand();
            return;
        }
        
        // Backspace for editing
        if (event == Event::Backspace) {
            command_window_->RemoveLastInputCharacter();
            return;
        }
    }
    
    LOG4CXX_DEBUG(dashboard_logger, "Unhandled event (ignoring)");
}

bool Dashboard::CheckForTerminalResize() {
    auto term_size = Terminal::Size();
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

int Dashboard::GetScreenHeight() const {
    return last_screen_height_;
}

int Dashboard::GetScreenWidth() const {
    return last_screen_width_;
}
```

**Need to check**: FTXUI's `GetEvent()` function availability
- May need to use `Screen::HandleInput()` or `ftxui::GetEvent()`
- May need to implement stdin reading manually
- Check examples in FTXUI codebase

**Acceptance Criteria**:
- [ ] Dashboard starts and doesn't exit until 'q' is pressed
- [ ] Pressing 'q' cleanly exits
- [ ] Event loop runs at ~30 FPS
- [ ] Terminal resize detected (test by resizing terminal)
- [ ] Arrow keys cause console output (for verification)

---

## Phase 2: Apply Height Constraints to FTXUI Elements

**Objective**: Use terminal height and window percentages to size elements properly

**Dependency**: Phase 1 (needs working event loop + resize handling)

### 2.1: Modify Dashboard::BuildLayout()

**Files to Modify**:
- `src/ui/Dashboard.cpp` - BuildLayout() method (lines 155-180)

**Current Code** (problematic):
```cpp
ftxui::Element Dashboard::BuildLayout() const {
    using namespace ftxui;
    
    std::vector<Element> layout_elements;
    
    if (metrics_panel_) {
        layout_elements.push_back(metrics_panel_->Render());
    }
    if (logging_window_) {
        layout_elements.push_back(logging_window_->Render());
    }
    if (command_window_) {
        layout_elements.push_back(command_window_->Render());
    }
    if (status_bar_) {
        layout_elements.push_back(status_bar_->Render());
    }
    
    return vbox(layout_elements);  // ← NO HEIGHT CONSTRAINTS
}
```

**New Implementation**:
```cpp
ftxui::Element Dashboard::BuildLayout() const {
    using namespace ftxui;
    
    // Get terminal dimensions
    auto terminal_size = Terminal::Size();
    int screen_height = terminal_size.dimy;
    int screen_width = terminal_size.dimx;
    
    // Calculate pixel heights based on percentages
    int metrics_height = std::max(1, (screen_height * window_heights_.metrics_height_percent) / 100);
    int logging_height = std::max(1, (screen_height * window_heights_.logging_height_percent) / 100);
    int command_height = std::max(1, (screen_height * window_heights_.command_height_percent) / 100);
    int status_height = std::max(1, (screen_height * window_heights_.status_height_percent) / 100);
    
    LOG4CXX_TRACE(dashboard_logger, "Layout: screen=" << screen_height << "px, "
        << "metrics=" << metrics_height << "px, "
        << "logging=" << logging_height << "px, "
        << "command=" << command_height << "px, "
        << "status=" << status_height << "px");
    
    // Build layout with explicit height constraints
    std::vector<Element> layout;
    
    if (metrics_panel_) {
        layout.push_back(
            metrics_panel_->Render() 
            | size(HEIGHT, EQUAL, metrics_height)
            | focusable
            | focus
        );
    }
    
    if (logging_window_) {
        layout.push_back(
            logging_window_->Render() 
            | size(HEIGHT, EQUAL, logging_height)
            | focusable
        );
    }
    
    if (command_window_) {
        layout.push_back(
            command_window_->Render() 
            | size(HEIGHT, EQUAL, command_height)
            | focusable
        );
    }
    
    if (status_bar_) {
        layout.push_back(
            status_bar_->Render() 
            | size(HEIGHT, EQUAL, status_height)
        );
    }
    
    return vbox(layout);
}
```

**Key Changes**:
1. Get terminal dimensions via `Terminal::Size()`
2. Calculate pixel heights from percentages
3. Apply `size(HEIGHT, EQUAL, pixels)` modifier to each element
4. Add `focusable` and `focus` attributes for event routing
5. Ensure minimum height of 1 pixel (avoid 0)

**Acceptance Criteria**:
- [ ] MetricsPanel takes ~40% of terminal height
- [ ] LoggingWindow takes ~35% of terminal height
- [ ] CommandWindow takes ~23% of terminal height
- [ ] StatusBar takes ~2% of terminal height
- [ ] Heights recalculate correctly on terminal resize
- [ ] Total height equals terminal height (no gaps)
- [ ] All window content visible and not clipped

### 2.2: Update Window::Render() Interface (if needed)

**Files to Review**:
- `include/ui/Window.hpp` - Check if Render() needs modifications
- Check each Window subclass to ensure Render() works with height constraints

**Possible Issue**: Some windows may have hard-coded heights or not respect FTXUI constraints

**Acceptance Criteria**:
- [ ] All Window subclasses Render() without hard-coded heights
- [ ] Content adapts to allocated height

---

## Phase 3: Connect LoggingAppender to LOG4CXX Pipeline

**Objective**: Route LOG4CXX messages to LoggingWindow via appender

**Dependency**: Phase 1 (event loop should be running)

### 3.1: Expose Appender from LoggingWindow

**Files to Modify**:
- `include/ui/LoggingWindow.hpp` - Add public getter
- `src/ui/LoggingWindow.cpp` - Implement getter

**Changes**:

**LoggingWindow.hpp**:
```cpp
class LoggingWindow : public Window {
private:
    std::shared_ptr<log4cxx::Appender> appender_;
    // ... existing members ...

public:
    // Get the appender for registration with LOG4CXX
    std::shared_ptr<log4cxx::Appender> GetAppender() const {
        return appender_;
    }
    
    // ... existing methods ...
};
```

**Acceptance Criteria**:
- [ ] GetAppender() returns non-null shared_ptr
- [ ] Appender is properly initialized in constructor

### 3.2: Register Appender in Dashboard::Initialize()

**Files to Modify**:
- `src/ui/Dashboard.cpp` - Initialize() method (around line 90-100)

**Add after LoggingWindow creation**:
```cpp
// In Dashboard::Initialize(), after creating logging_window_:

// Connect LoggingWindow appender to LOG4CXX pipeline
if (logging_window_) {
    auto appender = logging_window_->GetAppender();
    if (appender) {
        auto root_logger = log4cxx::Logger::getRootLogger();
        root_logger->addAppender(appender);
        
        LOG4CXX_INFO(dashboard_logger, 
            "LoggingWindow appender registered with LOG4CXX root logger");
        // ← This message now appears in LoggingWindow
    } else {
        LOG4CXX_WARN(dashboard_logger, 
            "LoggingWindow appender is null");
    }
}
```

**Placement**: In Dashboard::Initialize(), after line ~95 where logging_window_ is created

**Acceptance Criteria**:
- [ ] LOG4CXX_INFO, LOG4CXX_DEBUG, LOG4CXX_TRACE messages appear in LoggingWindow
- [ ] Messages from all application components appear
- [ ] No duplicate messages
- [ ] Level filtering works (only shows messages at or above filter level)

### 3.3: Verify Appender Thread Safety

**Files to Review**:
- `src/ui/LoggingAppender.cpp` - Check for thread safety
- `include/ui/LoggingAppender.hpp` - Check interface

**Verify**:
- [ ] LoggingAppender::append() uses mutex protection
- [ ] No race conditions on log_entries_ deque
- [ ] Thread-safe callback invocation

---

## Phase 4: Integrate CommandRegistry with CommandWindow

**Objective**: Wire CommandRegistry so user input → command execution → output

**Dependency**: Phase 1 (event loop for input handling)

### 4.1: Create CommandRegistry in Dashboard

**Files to Modify**:
- `include/ui/Dashboard.hpp` - Add registry member
- `src/ui/Dashboard.cpp` - Create and initialize registry

**Dashboard.hpp - Add member**:
```cpp
class Dashboard {
private:
    std::shared_ptr<CommandRegistry> command_registry_;  // Add this
    // ... existing members ...
};
```

**Dashboard.cpp - Initialize() method**:
```cpp
void Dashboard::Initialize() {
    // ... existing code ...
    
    // Create command registry
    command_registry_ = std::make_shared<CommandRegistry>();
    
    // Register built-in commands
    RegisterBuiltinCommands(command_registry_, this);
    
    LOG4CXX_INFO(dashboard_logger, "CommandRegistry created with built-in commands");
    
    // Pass registry to CommandWindow
    if (command_window_) {
        command_window_->SetCommandRegistry(command_registry_);
        LOG4CXX_INFO(dashboard_logger, "CommandRegistry connected to CommandWindow");
    }
    
    // ... rest of initialization ...
}
```

**Placement**: In Initialize(), around line 145, before or after command_window creation

### 4.2: Add Registry Reference to CommandWindow

**Files to Modify**:
- `include/ui/CommandWindow.hpp` - Add registry member and setter
- `src/ui/CommandWindow.cpp` - Implement setter

**CommandWindow.hpp**:
```cpp
class CommandWindow : public Window {
private:
    std::shared_ptr<CommandRegistry> registry_;
    // ... existing members ...

public:
    void SetCommandRegistry(std::shared_ptr<CommandRegistry> registry) {
        registry_ = registry;
        LOG4CXX_DEBUG("CommandWindow", "CommandRegistry connected");
    }
    
    // ... existing methods ...
};
```

### 4.3: Implement Command Execution in ParseAndExecuteCommand()

**Files to Modify**:
- `src/ui/CommandWindow.cpp` - ParseAndExecuteCommand() method (lines 85-105)

**Current Implementation** (broken):
```cpp
void CommandWindow::ParseAndExecuteCommand() {
    // Split input into command and arguments
    std::istringstream iss(input_buffer_);
    std::string command_name;
    iss >> command_name;
    
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    // TODO: Execute command via registry
    // Currently just echoes the command
    AddOutputLine("[ECHO] Command: " + command_name);
    for (const auto& a : args) {
        AddOutputLine("[ECHO]   Arg: " + a);
    }
}
```

**New Implementation**:
```cpp
void CommandWindow::ParseAndExecuteCommand() {
    if (input_buffer_.empty()) {
        return;
    }
    
    // Display command being executed
    AddOutputLine("> " + input_buffer_);
    
    // Add to history
    command_history_.push_back(input_buffer_);
    if (command_history_.size() > MAX_HISTORY) {
        command_history_.pop_front();
    }
    history_index_ = command_history_.size();
    
    // Parse input
    std::istringstream iss(input_buffer_);
    std::string command_name;
    iss >> command_name;
    
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    
    // Check for registry
    if (!registry_) {
        AddOutputLine("[ERROR] Command registry not configured");
        input_buffer_.clear();
        return;
    }
    
    // Special case: help command
    if (command_name == "help") {
        AddOutputLine("Available commands:");
        // List all registered commands
        // TODO: Add GetAllCommands() method to CommandRegistry if needed
        AddOutputLine("  status    - Display dashboard status");
        AddOutputLine("  run       - Execute the computation graph");
        AddOutputLine("  pause     - Pause the computation graph");
        AddOutputLine("  stop      - Stop the computation graph");
        AddOutputLine("  reset     - Reset layout to defaults");
        AddOutputLine("  help      - Display this help message");
        AddOutputLine("  q         - Quit the dashboard");
        input_buffer_.clear();
        return;
    }
    
    // Check if command exists
    if (!registry_->HasCommand(command_name)) {
        AddOutputLine("[ERROR] Unknown command: '" + command_name + "'");
        AddOutputLine("Type 'help' for available commands");
        input_buffer_.clear();
        return;
    }
    
    // Execute command
    try {
        auto result = registry_->ExecuteCommand(command_name, args);
        
        if (result.success) {
            // Add success output
            std::string output = result.output;
            // Split multi-line output
            std::istringstream output_stream(output);
            std::string line;
            while (std::getline(output_stream, line)) {
                if (!line.empty()) {
                    AddOutputLine("[SUCCESS] " + line);
                }
            }
        } else {
            // Add error output
            std::string output = result.output;
            std::istringstream output_stream(output);
            std::string line;
            while (std::getline(output_stream, line)) {
                if (!line.empty()) {
                    AddOutputLine("[ERROR] " + line);
                }
            }
        }
    } catch (const std::exception& e) {
        AddOutputLine("[ERROR] Exception: " + std::string(e.what()));
    }
    
    // Clear input
    input_buffer_.clear();
}
```

**Acceptance Criteria**:
- [ ] Typing "status" executes status command and displays output
- [ ] Typing "help" shows list of available commands
- [ ] Unknown command shows "[ERROR] Unknown command: ..."
- [ ] Command output appears in CommandWindow
- [ ] Command history works (up/down arrows scroll history)
- [ ] Error cases handled gracefully

### 4.4: Add Missing Input Handling in CommandWindow

**Files to Modify**:
- `include/ui/CommandWindow.hpp` - Check for AddInputCharacter() method
- `src/ui/CommandWindow.cpp` - Implement if missing

**Methods Needed**:
```cpp
class CommandWindow {
public:
    // Add character to input buffer
    void AddInputCharacter(char c) {
        if (input_buffer_.size() < MAX_INPUT_LENGTH) {
            input_buffer_ += c;
        }
    }
    
    // Remove last character from input
    void RemoveLastInputCharacter() {
        if (!input_buffer_.empty()) {
            input_buffer_.pop_back();
        }
    }
    
    // Get current input buffer for display
    std::string GetInputBuffer() const {
        return input_buffer_;
    }
};
```

**Acceptance Criteria**:
- [ ] Characters typed appear in input buffer
- [ ] Backspace removes characters
- [ ] Enter executes command
- [ ] Input buffer display shows cursor (_)

---

## Integration Testing Plan

### Test #1: Event Loop and Resize (Phase 1)
```bash
1. Run: ./build/bin/gdashboard --json-config config/mock_graph.json
2. Verify: Dashboard displays and doesn't auto-exit
3. Type: 'q'
4. Verify: Dashboard cleanly exits
5. Run again, resize terminal while running
6. Verify: Layout recalculates to fit new size
7. Verify: Frame rate stays ~30 FPS (check timing logs)
```

**Success Criteria**:
- [ ] Dashboard runs until user quits
- [ ] Resizing terminal works
- [ ] Performance maintained

### Test #2: Full-Screen Layout (Phase 2)
```bash
1. Run dashboard with 80x24 terminal (common size)
2. Verify: Metrics panel ~9-10 lines (40%)
3. Verify: Logging window ~8 lines (35%)
4. Verify: Command window ~5-6 lines (23%)
5. Verify: Status bar ~1 line (2%)
6. Resize to 100x40
7. Verify: All sections resize proportionally
8. Verify: No content cut off or wasted space
```

**Success Criteria**:
- [ ] Heights match percentages ±1 line
- [ ] Content visible in all sections
- [ ] Resize recalculates correctly

### Test #3: Logging Integration (Phase 3)
```bash
1. Run dashboard
2. Watch LoggingWindow during startup
3. Verify: Logs appear from application (not just placeholders)
4. Check log messages:
   - "[Dashboard] Initialized..."
   - "Metrics discovery complete"
   - "CommandRegistry created"
5. Type invalid command
6. Verify: Error appears in logging window AND command window
```

**Success Criteria**:
- [ ] Application logs appear in window
- [ ] No LOG4CXX messages lost
- [ ] Level filtering works

### Test #4: Command Execution (Phase 4)
```bash
1. Run dashboard
2. Type: status
3. Verify: Dashboard status displays
4. Type: help
5. Verify: List of commands appears
6. Type: unknown_command
7. Verify: "[ERROR] Unknown command" appears
8. Type: (nothing, just Enter)
9. Verify: No error, just returns
10. Type: status --flag
11. Verify: Command executes (flags may be ignored)
```

**Success Criteria**:
- [ ] Status command works
- [ ] Help command works
- [ ] Error handling works
- [ ] Empty input handled
- [ ] All built-in commands execute

---

## Implementation Checklist

### Phase 1: Event Loop
- [ ] 1.1: Analyze FTXUI GetEvent() API
- [ ] 1.2.1: Update Dashboard.hpp with event loop members
- [ ] 1.2.2: Implement Run() with event loop
- [ ] 1.2.2a: Implement HandleKeyEvent()
- [ ] 1.2.2b: Implement CheckForTerminalResize()
- [ ] 1.2.2c: Implement RecalculateLayout()
- [ ] Test event loop: build, run, quit with 'q'
- [ ] Test no auto-exit after 30 frames
- [ ] Test resize detection

### Phase 2: Height Constraints
- [ ] 2.1: Update BuildLayout() with height calculations
- [ ] 2.1: Apply size() modifiers to elements
- [ ] 2.2: Review all Window subclasses for hard-coded heights
- [ ] Test various terminal sizes
- [ ] Test resize recalculation
- [ ] Verify no gaps or clipping

### Phase 3: LOG4CXX Integration
- [ ] 3.1: Add GetAppender() to LoggingWindow
- [ ] 3.2: Update Dashboard::Initialize() to register appender
- [ ] 3.3: Verify thread-safe append()
- [ ] Test LOG4CXX messages appear
- [ ] Test level filtering works

### Phase 4: Command Integration
- [ ] 4.1: Add registry member to Dashboard
- [ ] 4.1: Initialize registry in Dashboard::Initialize()
- [ ] 4.2: Add registry member/setter to CommandWindow
- [ ] 4.3: Implement ParseAndExecuteCommand() with registry lookup
- [ ] 4.4: Add AddInputCharacter() and RemoveLastInputCharacter()
- [ ] Test status command
- [ ] Test help command
- [ ] Test unknown command error
- [ ] Test command history

---

## Timeline Estimate

| Phase | Estimated Time | Priority |
|-------|---|---|
| 1: Event Loop | 2-3 hours | **CRITICAL** |
| 2: Height Constraints | 1-2 hours | **CRITICAL** |
| 3: LOG4CXX | 30 mins | Medium |
| 4: Commands | 1 hour | Medium |
| **Total** | **4-6.5 hours** | |

---

## Risk Mitigation

### Risk: FTXUI doesn't have GetEvent() or it's blocking
**Mitigation**: 
- Check FTXUI documentation/examples first
- May need to implement custom event loop using threads
- fallback: Poll stdin with non-blocking read

### Risk: Screen resize detection not available
**Mitigation**:
- Use `SIGWINCH` signal handler as fallback
- Poll Terminal::Size() every frame
- Keep last known size to detect changes

### Risk: Height constraints cause rendering issues
**Mitigation**:
- Test with multiple terminal sizes
- Verify FTXUI size() modifier works as expected
- May need to use flex() instead of size()

### Risk: LOG4CXX appender not thread-safe
**Mitigation**:
- Add additional mutex in LoggingAppender if needed
- Test with concurrent logging
- Verify no deadlocks

### Risk: CommandRegistry execution time blocks UI
**Mitigation**:
- Run long commands in background thread (future)
- For MVP, accept small blocking time
- Monitor frame rate during command execution

---

## Success Criteria (Overall)

MVP is fully functional when:

- [ ] **Event Loop**: Dashboard runs until 'q', handles resize, ~30 FPS
- [ ] **Layout**: Uses full screen, 40/35/23/2 proportions, resizable
- [ ] **Logging**: All LOG4CXX messages appear, filterable
- [ ] **Commands**: User can type commands and see results
- [ ] **Integration**: All 4 gaps fixed, 223 tests still passing
- [ ] **Performance**: Stable 30 FPS even with max metrics
- [ ] **Robustness**: Graceful error handling, no crashes

---

## Files Summary

### Files to Create
- None (only modifications)

### Files to Modify
| File | Phase | Lines | Changes |
|------|-------|-------|---------|
| Dashboard.hpp | 1,2,4 | Headers | Add members, methods |
| Dashboard.cpp | 1,2,3,4 | Run(), Initialize(), BuildLayout() | Event loop, registry, appender |
| CommandWindow.hpp | 4 | Header | Add registry member, setter |
| CommandWindow.cpp | 4 | ParseAndExecuteCommand() | Actual execution |
| LoggingWindow.hpp | 3 | Header | Add GetAppender() |
| Terminal.hpp | 2 | (check only) | Size() API |

### Files to Review
- Terminal.hpp - Understand Terminal::Size() usage
- LoggingAppender.cpp - Verify thread safety
- CommandRegistry.hpp - Understand ExecuteCommand() API

---

## Next Steps

1. **Verify FTXUI Event API**
   - Check if GetEvent() exists
   - Check examples in FTXUI repo
   - Decide on event loop pattern

2. **Create Feature Branches**
   ```bash
   git checkout -b feature/event-loop
   git checkout -b feature/height-constraints
   git checkout -b feature/logging-integration
   git checkout -b feature/command-execution
   ```

3. **Begin Phase 1 Implementation**
   - Start with event loop
   - Get 'q' to quit working
   - Test event reading

4. **Build and Test After Each Phase**
   - `make clean && make`
   - Verify no regressions in 223 tests
   - Manual testing per section

5. **Integration Testing**
   - Run through all 4 test scenarios
   - Verify MVP criteria met
   - Performance validation

6. **Final Validation**
   - All tests passing
   - Clean git history
   - Update documentation

