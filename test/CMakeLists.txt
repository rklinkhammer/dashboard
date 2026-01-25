cmake_minimum_required(VERSION 3.16)

# ============================================================================
# Phase 0 Test Configuration
# ============================================================================

# Find Google Test
find_package(GTest REQUIRED)

# Enable testing
enable_testing()

# ============================================================================
# Unit Tests
# ============================================================================

add_executable(test_phase0_unit
    graph/test_mock_graph_executor.cpp
)

target_include_directories(test_phase0_unit
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_phase0_unit
    PRIVATE
    gdashboard_lib
    GTest::GTest
    GTest::Main
    nlohmann_json::nlohmann_json
)

add_test(
    NAME Phase0UnitTests
    COMMAND test_phase0_unit
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Integration Tests
# ============================================================================

add_executable(test_phase0_integration
    integration/test_phase0_metrics_discovery.cpp
)

target_include_directories(test_phase0_integration
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_phase0_integration
    PRIVATE
    gdashboard_lib
    GTest::GTest
    GTest::Main
    nlohmann_json::nlohmann_json
)

add_test(
    NAME Phase0IntegrationTests
    COMMAND test_phase0_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Test Execution and Reporting
# ============================================================================

# Create a custom target to run all Phase 0 tests
add_custom_target(test_phase0
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase0.*" -V
    DEPENDS test_phase0_unit test_phase0_integration
)

# Verbose test output
add_custom_target(test_phase0_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase0.*" -VV --output-on-failure
    DEPENDS test_phase0_unit test_phase0_integration
)
# ============================================================================
# Phase 1 Unit Tests
# ============================================================================

add_executable(test_phase1_components
    ui/test_phase1_components.cpp
)

target_include_directories(test_phase1_components
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_phase1_components
    PRIVATE
    gdashboard_lib
    GTest::GTest
    GTest::Main
    nlohmann_json::nlohmann_json
    ftxui::component
    ftxui::dom
    ftxui::screen
)

add_test(
    NAME Phase1ComponentTests
    COMMAND test_phase1_components
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Create custom targets for Phase 1
add_custom_target(test_phase1
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase1.*" -V
    DEPENDS test_phase1_components
)

add_custom_target(test_phase1_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase1.*" -VV --output-on-failure
    DEPENDS test_phase1_components
)

# Create combined target for all tests
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS test_phase0_unit test_phase0_integration test_phase1_components test_phase2_metrics
)

# ============================================================================
# Phase 2 Unit Tests
# ============================================================================

add_executable(test_phase2_metrics
    ui/test_metrics_tile_window.cpp
)

target_include_directories(test_phase2_metrics
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_phase2_metrics
    PRIVATE
    gdashboard_lib
    GTest::GTest
    GTest::Main
    nlohmann_json::nlohmann_json
    ftxui::component
    ftxui::dom
    ftxui::screen
)

add_test(
    NAME Phase2MetricsTests
    COMMAND test_phase2_metrics
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Create custom targets for Phase 2
add_custom_target(test_phase2
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase2.*" -V
    DEPENDS test_phase2_metrics
)

add_custom_target(test_phase2_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase2.*" -VV --output-on-failure
    DEPENDS test_phase2_metrics
)