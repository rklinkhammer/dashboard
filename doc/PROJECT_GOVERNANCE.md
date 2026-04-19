# Project Governance

## Include File Paths

**Rule**: All `#include` directives must specify the complete subdirectory path relative to the `include/` root, not assume local inclusion.

### Correct Style
```cpp
#include "app/metrics/NodeMetricsSchema.hpp"
#include "graph/executor/GraphExecutor.hpp"
#include "graph/mock/MockGraphExecutor.hpp"
```

### Incorrect Style (Avoid)
```cpp
#include "NodeMetricsSchema.hpp"              // ❌ Assumes local location
#include "GraphExecutor.hpp"                  // ❌ Ambiguous
#include "MockGraphExecutor.hpp"              // ❌ Unclear origin
```

### Rationale
- **Clarity**: Full paths make it immediately clear where files are located in the codebase
- **Maintainability**: Easier to refactor and reorganize code without breaking includes
- **Consistency**: Prevents confusion when multiple files have the same name
- **Build Configuration**: Simplifies CMakeLists.txt include path configuration

---

## Directory Structure Standards

Include files are organized as:
```
include/
├── app/
│   ├── capabilities/
│   ├── metrics/
│   └── policies/
├── graph/
│   ├── executor/
│   └── mock/
└── ui/
```

Source files mirror the include structure:
```
src/
├── app/metrics/
├── graph/executor/
├── graph/mock/
└── ui/
```

---

## Code Organization Principles

1. **Separation of Concerns**: Each module has clear, single responsibility
2. **Interface Segregation**: Abstract interfaces separate from implementations
3. **Dependency Injection**: Classes receive dependencies via constructors
4. **RAII**: Resource management through constructor/destructor pairs
5. **Thread Safety**: Mutex protection for shared state, clear documentation of thread contracts

---

## Naming Conventions

### Files
- Headers: `PascalCase.hpp` (e.g., `MetricsCapability.hpp`, `MockGraphExecutor.hpp`)
- Implementation: `PascalCase.cpp` (e.g., `MockGraphExecutor.cpp`)
- Tests: `test_snake_case.cpp` (e.g., `test_mock_graph_executor.cpp`)

### Classes
- Interfaces: `IPascalCase` or `PascalCase` (e.g., `MetricsCapability`, `GraphExecutor`)
- Implementations: `ConcreteNamePascalCase` (e.g., `MockMetricsCapability`, `MockGraphExecutor`)
- Utilities: `PascalCase` (e.g., `NodeMetricsSchema`, `MetricsEvent`)

### Variables
- Member variables: `snake_case_` (with trailing underscore for private)
- Local variables: `snake_case`
- Constants: `UPPER_SNAKE_CASE` or `kPascalCase`

---

## Include Guards

All headers must use `#pragma once` at the top:

```cpp
#pragma once

#include "required/headers.hpp"

// Forward declarations
class SomeClass;

// Class definition...
```

---

## Documentation Standards

### Classes
Every public class/interface must have a brief comment describing its purpose:

```cpp
// Manages metrics discovery and callback registration for dashboard updates
class MetricsCapability {
public:
    virtual ~MetricsCapability() = default;
    
    // Get all node metrics schemas for dashboard tile creation
    virtual std::vector<NodeMetricsSchema> GetNodeMetricsSchemas() const = 0;
    
    // Register callback to receive metrics events from publisher thread
    virtual void RegisterMetricsCallback(
        std::function<void(const MetricsEvent&)> callback) = 0;
```

### Methods
Public methods should document:
- **Purpose**: What does it do?
- **Parameters**: Input descriptions
- **Return**: What is returned?
- **Threading**: Thread safety guarantees
- **Exceptions**: What can throw?

---

## Build Configuration

### CMakeLists.txt Guidelines
- Each module has its own `CMakeLists.txt`
- Root `CMakeLists.txt` coordinates module inclusion
- Include paths specified relative to project root:
  ```cmake
  target_include_directories(target
      PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/../../include
  )
  ```

---

## Testing Standards

### Unit Tests
- Located in `test/` mirroring `src/` structure
- Naming: `test_<module>_<feature>.cpp`
- Framework: Google Test (gtest)
- Coverage goal: 80%+ for critical code

### Integration Tests
- Located in `test/integration/`
- Test complete workflows across modules
- Validate data flow and thread coordination

### Example Test Structure
```cpp
#include <gtest/gtest.h>
#include "graph/mock/MockGraphExecutor.hpp"

class MockGraphExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        executor = std::make_shared<MockGraphExecutor>(30);
    }
    
    std::shared_ptr<MockGraphExecutor> executor;
};

TEST_F(MockGraphExecutorTest, ConstructsSuccessfully) {
    EXPECT_NE(executor, nullptr);
}
```

---

## Code Review Checklist

- [ ] Includes use full subdirectory paths
- [ ] `#pragma once` guards present
- [ ] Classes have documentation comments
- [ ] Public methods documented (purpose, parameters, return, threading)
- [ ] RAII patterns followed (no naked new/delete)
- [ ] Thread-safety clearly documented
- [ ] Error handling appropriate (exceptions or error codes)
- [ ] No memory leaks (use smart pointers)
- [ ] Unit tests provided (80%+ coverage target)
- [ ] No compiler warnings
- [ ] Follows naming conventions

---

## Version Control

### Commit Messages
- Use present tense: "Add feature" not "Added feature"
- First line <= 72 characters
- Reference issues: "Fixes #123"
- Detailed explanation in body if needed

### Branch Naming
- Features: `feature/description`
- Fixes: `fix/description`
- Refactoring: `refactor/description`

---

## Documentation Maintenance

- Keep ARCHITECTURE.md synchronized with major design decisions
- Update IMPLEMENTATION_PLAN.md as phases progress
- Add code comments for non-obvious logic
- Document thread safety and concurrency considerations
- Include examples for complex features

