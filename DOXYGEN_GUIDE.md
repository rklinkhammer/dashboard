# Doxygen Documentation Guide

## Overview

This project uses Doxygen for automated API documentation generation. Doxygen parses source code comments and generates comprehensive HTML documentation including class hierarchies, file relationships, and detailed API references.

## Installing Doxygen

### macOS (Homebrew)
```bash
brew install doxygen
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install doxygen
```

### Other systems
Visit [Doxygen Download Page](https://www.doxygen.nl/download.html)

## Generating Documentation

### Generate HTML Documentation
```bash
cd build
make doc
```

Output: `build/docs/html/index.html`

### Generate and Open in Browser
```bash
cd build
make doc_view
```

This will automatically open the documentation in your default web browser.

### Generate from Project Root
```bash
doxygen Doxyfile
```

## Documentation Comment Syntax

### Class Documentation
```cpp
/**
 * @class MyClass
 * @brief Brief description of the class
 * 
 * Detailed description of the class explaining its purpose,
 * usage, and any important implementation details.
 * 
 * @see RelatedClass
 */
class MyClass {
    // ...
};
```

### Function/Method Documentation
```cpp
/**
 * @brief Brief description of the function
 * 
 * Detailed description explaining what the function does,
 * its algorithm, and any important notes.
 * 
 * @param paramName Description of parameter
 * @param anotherParam Description of another parameter
 * @return Description of the return value
 * 
 * @throw std::exception Description of exception that might be thrown
 * 
 * @see RelatedFunction
 * @example
 *   MyClass obj;
 *   int result = obj.doSomething(42);
 */
void doSomething(int paramName, std::string anotherParam);
```

### Member Variable Documentation
```cpp
/**
 * @brief Description of the member variable
 * 
 * Additional details about what this variable represents
 * and how it's used within the class.
 */
int count_;
```

### Namespace Documentation
```cpp
/**
 * @namespace app::metrics
 * @brief Metrics and monitoring subsystem
 * 
 * This namespace contains all metrics-related classes and
 * functions for the dashboard application.
 */
namespace app::metrics {
    // ...
}
```

### Enumeration Documentation
```cpp
/**
 * @enum AlertLevel
 * @brief Alert severity levels for metric thresholds
 */
enum class AlertLevel {
    OK,         ///< No alert condition
    WARNING,    ///< Warning threshold exceeded
    CRITICAL    ///< Critical threshold exceeded
};
```

## Common Doxygen Tags

| Tag | Purpose |
|-----|---------|
| `@brief` | Brief one-line description |
| `@param` | Document function parameter |
| `@return` | Document return value |
| `@throw` | Document exception |
| `@see` | Cross-reference related items |
| `@deprecated` | Mark as deprecated |
| `@todo` | Mark incomplete features |
| `@warning` | Important warning |
| `@note` | Additional note |
| `@example` | Code example usage |
| `@file` | File-level documentation |
| `@namespace` | Namespace documentation |
| `@class` | Class documentation |
| `@struct` | Structure documentation |

## Documentation Examples

### Full Class Example
```cpp
/**
 * @class MetricsPanel
 * @brief Main panel for displaying node metrics in a tabbed interface
 * 
 * MetricsPanel manages a collection of NodeMetricsTiles organized in tabs,
 * with one tab per node in the execution graph. It handles metric updates
 * from the graph executor via callback mechanism and renders them in a
 * responsive grid layout.
 * 
 * Thread Safety: This class is not thread-safe for rendering operations.
 * Metric updates must be applied from the main event loop thread using
 * SetLatestValue(). Updates from executor callback threads are queued
 * safely via mutex-protected latestValues_ map.
 * 
 * @see NodeMetricsTile, MetricsCapability
 * 
 * @example
 *   MetricsPanel panel;
 *   // Add node tiles from discovered metrics
 *   auto schema = capability->GetNodeMetrics("Sensor");
 *   panel.AddNodeTab("Sensor", schema);
 *   
 *   // Update metrics from callback thread
 *   panel.SetLatestValue("Sensor::temperature", 42.5);
 *   
 *   // Render in main loop
 *   auto element = panel.Render();
 */
class MetricsPanel {
public:
    /**
     * @brief Add a new node tab to the panel
     * 
     * Creates a new NodeMetricsTile for the specified node and adds
     * it to the tab container at the next available position.
     * 
     * @param nodeName Name of the node (should be unique)
     * @param metrics Vector of metric descriptors for this node
     * 
     * @return Index of the newly added tab
     * 
     * @see NodeMetricsTile
     */
    size_t AddNodeTab(const std::string& nodeName, 
                      const std::vector<MetricDescriptor>& metrics);
    
    /**
     * @brief Update a metric value (thread-safe)
     * 
     * Records the latest value for a metric, identified by "NodeName::metric_name".
     * This method is designed to be called from executor callback threads and
     * uses mutex protection for thread safety.
     * 
     * @param metricId Full metric identifier in format "NodeName::metric_name"
     * @param value The numeric value to set
     * 
     * @warning This method acquires a mutex lock. For high-frequency updates,
     *          consider batching multiple updates.
     */
    void SetLatestValue(const std::string& metricId, double value);
};
```

### Full Function Example
```cpp
/**
 * @brief Convert a NodeMetricsSchema to a NodeMetricsTile
 * 
 * Transforms a schema definition (obtained from metrics discovery) into
 * a tile ready for rendering. This function handles the mapping between
 * metric descriptors and tile metric storage.
 * 
 * @param nodeName Name of the node containing these metrics
 * @param schema The metrics schema discovered from the node's capability
 * 
 * @return A NodeMetricsTile configured with metrics from the schema
 * 
 * @throw std::invalid_argument if nodeName is empty or schema contains
 *        invalid metric descriptors
 * 
 * @see NodeMetricsSchema, MetricsCapability::GetNodeMetrics
 * 
 * @example
 *   auto schema = capability->GetNodeMetrics("AltitudeFusion");
 *   auto tile = NodeMetricsSchemaToTile("AltitudeFusion", schema);
 *   panel.AddNodeTab("AltitudeFusion", tile);
 */
NodeMetricsTile NodeMetricsSchemaToTile(
    const std::string& nodeName,
    const NodeMetricsSchema& schema
);
```

## Configuration Details

The Doxyfile is configured with the following settings:

- **Input**: All `.hpp`, `.cpp`, `.h`, `.c` files in `include/` and `src/` directories
- **Markdown**: Processes README.md and ARCHITECTURE.md as documentation sources
- **HTML Output**: Located in `build/docs/html/`
- **Source Code**: Includes inline source code references
- **Diagrams**: Include files and relationships (requires GraphViz for UML diagrams)
- **Search**: Enabled for the HTML output
- **Warnings**: Reports undocumented classes and members

## Best Practices

1. **Document Public APIs**: All public classes, functions, and members should have `@brief` descriptions
2. **Use `@param` and `@return`**: Always document function parameters and return values
3. **Provide Examples**: Use `@example` tags for complex APIs
4. **Cross-reference**: Use `@see` to link related documentation
5. **Keep Updated**: Update documentation when changing function signatures
6. **Thread Safety Notes**: Document thread-safety guarantees in class/function descriptions
7. **Performance Notes**: Document performance characteristics and constraints
8. **Deprecation**: Mark deprecated APIs with `@deprecated` tag

## Continuous Documentation

To keep documentation in sync with code:

1. Run `make doc` before commits to verify no warnings
2. Review generated HTML for clarity and completeness
3. Update documentation when adding new features
4. Fix Doxygen warnings during code review

## Troubleshooting

### Doxygen not found
```bash
# Install Doxygen
brew install doxygen  # macOS
sudo apt-get install doxygen  # Linux
```

### Documentation not generating
- Check Doxyfile exists in project root
- Verify file paths are correct in Doxyfile
- Run `doxygen Doxyfile` directly for detailed output

### Missing documentation
- Ensure classes/functions have `@brief` tag
- Run `doxygen Doxyfile` to see warnings
- Check that documentation comments use `/**` format

### Want to generate additional formats?
Edit the Doxyfile to enable:
- LaTeX/PDF: Set `GENERATE_LATEX = YES`
- Man pages: Set `GENERATE_MAN = YES`
- XML: Set `GENERATE_XML = YES`

## Related Documentation

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Doxygen Commands](https://www.doxygen.nl/manual/commands.html)
- Project Documentation: See `build/docs/html/index.html` after running `make doc`
