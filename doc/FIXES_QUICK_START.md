# Quick Start: Implementation Fix Guide

**For**: Fixing the 4 critical MVP gaps  
**Status**: Ready to implement  
**Time Budget**: 4-6.5 hours total  

---

## TL;DR - What to Fix

| Gap | Root Cause | Quick Fix | Time |
|-----|-----------|-----------|------|
| **Heights** | SetHeight() stores %, never used | Add `size(HEIGHT, EQUAL, pixels)` to BuildLayout() | 1h |
| **Event Loop** | Exits after 30 frames | Replace hard-coded exit with `while(!should_exit_)` + event reading | 2.5h |
| **Logging** | Appender created but not registered | Call `root_logger->addAppender()` in Initialize() | 30m |
| **Commands** | Registry exists but Window never integrated | Pass registry to CommandWindow, execute in ParseAndExecuteCommand() | 1h |

**Total**: 5 hours  
**Start**: Event Loop (dependency for others)  
**End**: All 223 tests passing + working MVP

---

## What You'll Change

### Phase 1: Event Loop (2.5 hours) ⚡
**File**: `src/ui/Dashboard.cpp`  
**Method**: `Run()` (lines 197-245)

**What's wrong**:
```cpp
if (frame_count >= 30) should_exit_ = true;  // Auto-exit after 1 sec!
// No event reading, no resize handling
```

**What to do**:
```cpp
while (!should_exit_) {
    if (CheckForTerminalResize()) RecalculateLayout();
    if (auto event = GetEvent()) HandleKeyEvent(*event);
    // Render...
    if (event == Event::Character('q')) should_exit_ = true;
}
```

**Test**: Run app, press 'q', cleanly exits ✓

---

### Phase 2: Height Constraints (1.5 hours) 📏
**File**: `src/ui/Dashboard.cpp`  
**Method**: `BuildLayout()` (lines 155-180)

**What's wrong**:
```cpp
return vbox(layout_elements);  // No size constraints!
// Elements render at natural height, waste screen
```

**What to do**:
```cpp
int h = (Terminal::Size().dimy * percent) / 100;
return vbox({
    metrics_panel_->Render() | size(HEIGHT, EQUAL, h),
    // ... more windows with calculated heights
});
```

**Test**: 40% + 35% + 23% + 2% = 100% of screen ✓

---

### Phase 3: LOG4CXX Integration (30 mins) 📝
**File**: `src/ui/Dashboard.cpp`  
**Method**: `Initialize()` (after line ~95)

**What's wrong**:
```cpp
logging_window_ = std::make_shared<LoggingWindow>("Logs");
// Appender created but never registered with LOG4CXX!
```

**What to do**:
```cpp
auto root_logger = log4cxx::Logger::getRootLogger();
root_logger->addAppender(logging_window_->GetAppender());
// Now LOG4CXX_INFO(...) appears in window
```

**Test**: LOG4CXX messages appear in LoggingWindow ✓

---

### Phase 4: Command Execution (1 hour) 💻
**Files**: `src/ui/Dashboard.cpp`, `src/ui/CommandWindow.cpp`

**Dashboard.cpp changes**:
```cpp
// In Initialize():
command_registry_ = std::make_shared<CommandRegistry>();
RegisterBuiltinCommands(command_registry_, this);
command_window_->SetCommandRegistry(command_registry_);
```

**CommandWindow.cpp changes**:
```cpp
// In ParseAndExecuteCommand():
if (registry_->HasCommand(command_name)) {
    auto result = registry_->ExecuteCommand(command_name, args);
    AddOutputLine(result.success ? "[SUCCESS] " : "[ERROR] " + result.output);
}
```

**Test**: Type "status" → see dashboard status ✓

---

## Implementation Checklist (Quick)

```
Phase 1: Event Loop
  [ ] Add event loop members to Dashboard.hpp
  [ ] Implement HandleKeyEvent()
  [ ] Implement CheckForTerminalResize()  
  [ ] Rewrite Run() method
  [ ] Build & test 'q' to quit
  
Phase 2: Height Constraints
  [ ] Update BuildLayout() calculations
  [ ] Apply size() modifiers
  [ ] Test multiple terminal sizes
  
Phase 3: LOG4CXX
  [ ] Add GetAppender() to LoggingWindow
  [ ] Register in Dashboard::Initialize()
  [ ] Test LOG4CXX messages appear
  
Phase 4: Commands
  [ ] Add registry member to Dashboard
  [ ] Add registry member to CommandWindow
  [ ] Update ParseAndExecuteCommand()
  [ ] Test status command
  [ ] Test help command
```

---

## Key Files to Modify

```
include/ui/Dashboard.hpp          ← Add members & methods
src/ui/Dashboard.cpp              ← Run(), Initialize(), BuildLayout()
include/ui/CommandWindow.hpp      ← Add registry member
src/ui/CommandWindow.cpp          ← ParseAndExecuteCommand()
include/ui/LoggingWindow.hpp      ← Add GetAppender()
```

---

## Testing After Each Phase

### Phase 1 ✓
```bash
./build/bin/gdashboard --json-config config/mock_graph.json
# Type 'q' → exits (not auto-exit)
# Resize terminal → layout updates
```

### Phase 2 ✓
```bash
# Verify visual proportions:
# 40% metrics + 35% logs + 23% command + 2% status = 100%
# No wasted space, content visible
```

### Phase 3 ✓
```bash
# Check LoggingWindow during startup:
# Should see "[Dashboard] Initialized..."
# Not just placeholder logs
```

### Phase 4 ✓
```bash
# Type commands:
> status        → see dashboard status
> help          → see command list
> invalid_cmd   → see error message
```

---

## Common Pitfalls to Avoid

1. **Event Loop**: Don't forget to actually call `GetEvent()` from FTXUI
2. **Heights**: Use `size(HEIGHT, EQUAL, pixels)`, not just percentages
3. **Logging**: Must add appender to root logger, not individual logger
4. **Commands**: Registry must be created BEFORE registering built-in commands

---

## Reference Documents

- **ANALYSIS.md** - High-level overview of each gap
- **TECHNICAL_GAPS.md** - Line-by-line code analysis with examples
- **IMPLEMENTATION_FIXES.md** - Full detailed plan (this one's longer)

---

## Success Criteria

When done:
- [ ] Dashboard runs at 30 FPS until user presses 'q'
- [ ] Window heights scale to terminal (40/35/23/2%)
- [ ] LOG4CXX messages appear in LoggingWindow
- [ ] Commands execute and show results
- [ ] All 223 tests still passing
- [ ] No regressions from original implementation

---

## Expected Time Per Phase

| Phase | Estimate | Actual | Status |
|-------|----------|--------|--------|
| Event Loop | 2-3h | — | Pending |
| Heights | 1-2h | — | Pending |
| Logging | 30m | — | Pending |
| Commands | 1h | — | Pending |
| **Total** | **4-6.5h** | — | **Pending** |

---

## Git Workflow

```bash
# Create feature branch
git checkout -b feature/event-loop

# After implementing Phase 1:
make clean && make
ctest  # Verify 223 tests still pass
git add -A
git commit -m "Phase 1: Implement real event loop"

# Continue for other phases...
```

---

## Need Help?

1. Check FTXUI documentation for GetEvent() API
2. Review FTXUI examples for screen size/resize handling
3. Verify LOG4CXX appender registration pattern
4. Test CommandRegistry.ExecuteCommand() API

All 4 issues are **fixable with 30-40 lines of code changes** per phase.

