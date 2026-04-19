cmake_minimum_required(VERSION 3.23)
project(gdashboard 
    VERSION 0.1.0
    DESCRIPTION "Real-time metrics visualization dashboard for graph execution engines"
    LANGUAGES CXX
)

# ============================================================================
# Build Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Add Homebrew CMake modules path (for macOS)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    # Add Eigen path for Homebrew on macOS
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/eigen")
endif()


# ============================================================================
# Dependencies
# ============================================================================

# Find required packages
find_package(nlohmann_json REQUIRED)
find_package(FTXUI REQUIRED)
find_package(GTest)
find_package(log4cxx REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Curses REQUIRED)

# Include directories (for all targets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Testing Setup
# ============================================================================

# Enable testing at root level so CTest discovers all tests
enable_testing()

# ============================================================================
# Source Organization
# ============================================================================

# Add subdirectories in order of dependencies
add_subdirectory(src)

# Plugins
add_subdirectory(plugins)

# Add tests (optional, controlled by ENABLE_TESTS)
if(GTest_FOUND)
    add_subdirectory(test)
else()
    message(WARNING "Google Test not found. Tests will not be built. Install gtest to enable tests.")
endif()

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "gdashboard Build Configuration")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Source: ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "  Include: ${CMAKE_CURRENT_SOURCE_DIR}/include")
if(GTest_FOUND)
    message(STATUS "  Tests: ENABLED")
else()
    message(STATUS "  Tests: DISABLED")
endif()

# ============================================================================
# Documentation (Doxygen)
# ============================================================================

include(${CMAKE_SOURCE_DIR}/cmake/Doxygen.cmake)

# ============================================================================
# Build Options
# ============================================================================

option(ENABLE_VERBOSE_LOGGING "Enable verbose logging output" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

if(ENABLE_VERBOSE_LOGGING)
    add_compile_definitions(GDASHBOARD_VERBOSE_LOGGING)
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()
