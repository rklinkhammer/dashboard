# Doxygen Quick Reference

## TL;DR - Generate Documentation

```bash
# From project root
cd build
make doc              # Generate HTML documentation
make doc_view         # Generate and open in browser

# Or directly
doxygen ../Doxyfile
open build/docs/html/index.html
```

## Essential Documentation Format

Copy and adapt this template for your code:

### For Classes
```cpp
/**
 * @brief One-line description of your class
 * 
 * Longer description explaining the class purpose,
 * usage patterns, and any important notes.
 * 
 * @see RelatedClass
 */
class MyClass {
public:
    /**
     * @brief Brief function description
     * @param arg1 What is this parameter
     * @return What does it return
     */
    int doSomething(const std::string& arg1);
};
```

### For Enums
```cpp
/**
 * @enum MyEnum
 * @brief Brief description of the enum
 */
enum class MyEnum {
    VALUE1,  ///< Description of VALUE1
    VALUE2   ///< Description of VALUE2
};
```

### For Structs/Records
```cpp
/**
 * @struct MyData
 * @brief Brief description of the data structure
 */
struct MyData {
    int id;           ///< Unique identifier
    std::string name; ///< Display name
};
```

## Common Tags

| Tag | Use | Example |
|-----|-----|---------|
| `@brief` | One-line summary | `@brief Displays metrics in real-time` |
| `@param` | Function parameter | `@param nodeId The node identifier` |
| `@return` | Return value | `@return Number of metrics updated` |
| `@see` | Cross-reference | `@see MetricsPanel` |
| `@warning` | Important caution | `@warning Not thread-safe` |
| `@note` | Additional info | `@note Requires valid schema` |
| `@example` | Code example | `@example MyClass obj; obj.init();` |

## Documentation Locations

- **Source code**: Look for `/**` comments in `.hpp` and `.cpp` files
- **Generated HTML**: `build/docs/html/index.html`
- **API Reference**: `build/docs/html/annotated.html`
- **File List**: `build/docs/html/files.html`
- **Namespace Index**: `build/docs/html/namespaces.html`

## .gitignore Entry

Documentation output is already excluded:
```
# In .gitignore
docs/
build/docs/
```

## Integration with IDEs

Many IDEs can display Doxygen documentation in code completion:

- **VS Code**: Uses Intellisense with Doxygen comments
- **CLion**: Automatically parses Doxygen documentation
- **Xcode**: Shows documentation in quick help

## Viewing Generated Documentation

```bash
# Open in default browser
open build/docs/html/index.html

# Or serve locally
cd build/docs/html
python3 -m http.server 8000
# Then visit http://localhost:8000
```

## Troubleshooting

**Q: Documentation not updating?**  
A: Run `rm -rf build/docs && make doc` to force regeneration

**Q: Missing documentation for new files?**  
A: Doxygen needs cmake reconfiguration: `cmake -B build && make doc`

**Q: Want to exclude certain files?**  
A: Edit `Doxyfile`, find `EXCLUDE_PATTERNS` and add patterns

**Q: How to add custom CSS?**  
A: Place CSS file in project, set `HTML_EXTRA_STYLESHEET` in Doxyfile

## Full Documentation

See [DOXYGEN_GUIDE.md](DOXYGEN_GUIDE.md) for comprehensive information.
