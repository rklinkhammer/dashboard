# ============================================================================
# Core Source Build Configuration
# ============================================================================

# Collect all source files
# Note: We only build Phase 0 mock for now
# GraphExecutorBuilder.cpp requires additional dependencies not yet configured
set(GDASHBOARD_SOURCES
    graph/mock/MockGraphExecutor.cpp
    ui/Window.cpp
    ui/MetricsPanel.cpp
    ui/LoggingWindow.cpp
    ui/LoggingAppender.cpp
    ui/CommandWindow.cpp
    ui/CommandRegistry.cpp
    ui/BuiltinCommands.cpp
    ui/StatusBar.cpp
    ui/MetricsTileWindow.cpp
    ui/MetricsTilePanel.cpp
    gdashboard/DashboardApplication.cpp
)

# Create library target
add_library(gdashboard_lib STATIC
    ${GDASHBOARD_SOURCES}
)

# Set include directories
target_include_directories(gdashboard_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link dependencies
target_link_libraries(gdashboard_lib PUBLIC
    nlohmann_json::nlohmann_json
    ftxui::component
    ftxui::dom
    ftxui::screen
)

# Create gdashboard executable
add_executable(gdashboard
    gdashboard/dashboard_main.cpp
)

# Link with library and dependencies
target_link_libraries(gdashboard PRIVATE
    gdashboard_lib
    ftxui::component
    ftxui::dom
    ftxui::screen
)

target_include_directories(gdashboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Set C++ standard
set_target_properties(gdashboard PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

# Compiler flags
if(MSVC)
    target_compile_options(gdashboard PRIVATE /W4 /WX)
else()
    target_compile_options(gdashboard PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Compiler flags
if(MSVC)
    target_compile_options(gdashboard_lib PRIVATE /W4 /WX)
else()
    target_compile_options(gdashboard_lib PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Set C++ standard
set_target_properties(gdashboard_lib PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Built gdashboard_lib with sources:")
foreach(src ${GDASHBOARD_SOURCES})
    message(STATUS "  - ${src}")
endforeach()
