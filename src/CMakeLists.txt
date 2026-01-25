# ============================================================================
# Core Source Build Configuration
# ============================================================================

# Collect all source files
# Note: We only build Phase 0 mock for now
# GraphExecutorBuilder.cpp requires additional dependencies not yet configured
set(GDASHBOARD_SOURCES
    graph/mock/MockGraphExecutor.cpp
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
)

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
