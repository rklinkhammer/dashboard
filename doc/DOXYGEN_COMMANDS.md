# Doxygen Commands Reference

## Quick Commands

```bash
# From project root (after cloning/building)

# Generate documentation
cd build && make doc

# Generate and open in browser
cd build && make doc_view

# Or manually
doxygen ../Doxyfile
open build/docs/html/index.html
```

## Build Integration

When building the project:

```bash
cd /path/to/dashboard
cmake -B build                    # CMake finds Doxygen
cd build
make                              # Builds application
make doc                          # Optional: generate docs
make doc_view                     # Optional: view docs
```

## Documentation Artifacts

After running `make doc`, these files are available:

```
build/docs/
├── html/                         # Main documentation website
│   ├── index.html               # Start here
│   ├── annotated.html           # All classes/structs
│   ├── files.html               # All source files
│   ├── namespacelist.html       # Namespace index
│   └── ... (CSS, JS, search)
└── gdashboard.tags              # Tag file for external docs
```

## Viewing Documentation

```bash
# Open in default browser
open build/docs/html/index.html

# Or serve locally with Python
cd build/docs/html
python3 -m http.server 8000
# Visit http://localhost:8000

# Or with Ruby
cd build/docs/html
ruby -run -ehttpd . -p8000
# Visit http://localhost:8000
```

## IDE Integration

### VS Code
- Install "Doxygen Documentation Generator" extension
- Will parse and display Doxygen comments in hover tooltips

### CLion
- Built-in support - shows Doxygen comments in documentation popup
- Press Ctrl+J (or Cmd+J on Mac) on any symbol

### Xcode
- Documentation comments appear in Quick Help (Cmd+Option+2)

## Regenerating Documentation

### Full regeneration (clean)
```bash
cd build
rm -rf docs/
cmake ..
make doc
```

### Incremental (only changes)
```bash
cd build
make doc
```

## Configuration

### Modify Doxyfile location
Edit `CMakeLists.txt` and change:
```cmake
set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/Doxyfile)
```

### Change output directory
Edit `Doxyfile` and change:
```
OUTPUT_DIRECTORY = ./docs
```

### Add additional markdown files
Edit `Doxyfile` and modify:
```
INPUT = include src README.md ARCHITECTURE.md YOURFILE.md
```

## Troubleshooting

### Command not found: doxygen
```bash
# Install Doxygen
brew install doxygen  # macOS
sudo apt install doxygen  # Ubuntu/Debian
```

### CMake doesn't find Doxygen
```bash
# Remove CMake cache and reconfigure
cd build
rm -rf CMakeCache.txt CMakeFiles/
cmake ..
```

### Documentation incomplete
1. Check for Doxygen warnings: `doxygen ../Doxyfile`
2. Verify documentation format in source files
3. Look at example in [DOXYGEN_GUIDE.md](DOXYGEN_GUIDE.md)

### Want to disable Doxygen?
Edit `CMakeLists.txt` and comment out:
```cmake
# include(${CMAKE_SOURCE_DIR}/cmake/Doxygen.cmake)
```

## Advanced Options

### Generate man pages
Edit `Doxyfile`:
```
GENERATE_MAN = YES
MAN_OUTPUT = man
```

### Generate LaTeX/PDF
Edit `Doxyfile`:
```
GENERATE_LATEX = YES
LATEX_OUTPUT = latex
USE_PDFLATEX = YES
```

Then generate:
```bash
cd build/docs/latex
make
open refman.pdf
```

### Generate XML output
Edit `Doxyfile`:
```
GENERATE_XML = YES
XML_OUTPUT = xml
```

### Add custom CSS
1. Create `custom.css`
2. Edit `Doxyfile`:
```
HTML_EXTRA_STYLESHEET = /path/to/custom.css
```

## File Locations

| Item | Location |
|------|----------|
| Configuration | `./Doxyfile` |
| CMake module | `./cmake/Doxygen.cmake` |
| Full guide | `./DOXYGEN_GUIDE.md` |
| Quick start | `./DOXYGEN_QUICK_START.md` |
| Integration info | `./DOXYGEN_INTEGRATION.md` |
| Generated docs | `./build/docs/html/` |

## Next Steps

1. **Document your code**: Add Doxygen comments to your classes/functions
2. **Generate docs**: Run `make doc` from `build/` directory
3. **Review**: Open `build/docs/html/index.html` in browser
4. **Iterate**: Fix documentation issues and regenerate

## Resources

- [Doxygen Official Site](https://www.doxygen.nl/)
- [Command Reference](https://www.doxygen.nl/manual/commands.html)
- [FAQ](https://www.doxygen.nl/manual/faq.html)
- [Project Guide](DOXYGEN_GUIDE.md)
