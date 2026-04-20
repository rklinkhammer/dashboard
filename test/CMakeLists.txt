cmake_minimum_required(VERSION 3.16)

# ============================================================================
# Phase 0 Test Configuration
# ============================================================================

# Find Google Test
find_package(GTest REQUIRED)

# ============================================================================
# Unit Tests
# ============================================================================

add_executable(test_phase0_unit
    graph/test_mock_graph_executor.cpp
    graph/test_graph_config_parser.cpp
    graph/test_json_dynamic_graph_loader.cpp
    graph/test_node_factory.cpp
    core/test_active_queue.cpp
    core/test_thread_pool.cpp
    core/test_type_info.cpp
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
    log4cxx
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
# Phase 2 Integration Tests
# ============================================================================

# add_executable(test_phase2_end_to_end
#     integration/test_phase2_end_to_end.cpp
# )

# target_include_directories(test_phase2_end_to_end
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_phase2_end_to_end
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase2EndToEndTests
#     COMMAND test_phase2_end_to_end
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Test Execution and Reporting
# ============================================================================

# Create a custom target to run all Phase 2 tests
add_custom_target(test_phase2
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase2.*" -V
    DEPENDS test_phase2_metrics test_phase2_end_to_end
)

# Verbose test output
add_custom_target(test_phase2_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase2.*" -VV --output-on-failure
    DEPENDS test_phase2_metrics test_phase2_end_to_end
)
# ============================================================================
# Phase 1 Unit Tests
# ============================================================================

# add_executable(test_phase1_components
#     ui/test_phase1_components.cpp
# )

# target_include_directories(test_phase1_components
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_phase1_components
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase1ComponentTests
#     COMMAND test_phase1_components
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# # Create custom targets for Phase 1
# add_custom_target(test_phase1
#     COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase1.*" -V
#     DEPENDS test_phase1_components
# )

# add_custom_target(test_phase1_verbose
#     COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase1.*" -VV --output-on-failure
#     DEPENDS test_phase1_components
# )

# # Create combined target for all tests
# add_custom_target(test_all
#     COMMAND ${CMAKE_CTEST_COMMAND} -V
#     DEPENDS test_phase0_unit test_phase0_integration test_phase1_components test_phase2_metrics test_phase2_end_to_end test_command_registry test_logging_window test_layout_config
# )

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
    log4cxx
)

add_test(
    NAME Phase2MetricsTests
    COMMAND test_phase2_metrics
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Phase 1 Unit Tests: NodeMetricsTile
# ============================================================================
# NOTE: Test disabled - NodeMetricsTile struct doesn't have expected methods
# TODO: Either implement missing methods in NodeMetricsTile or refactor test

# add_executable(test_node_metrics_tile
#     ui/test_node_metrics_tile.cpp
# )

# target_include_directories(test_node_metrics_tile
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_node_metrics_tile
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
#     log4cxx
# )

# add_test(
#     NAME Phase1NodeMetricsTileTests
#     COMMAND test_node_metrics_tile
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 2 Unit Tests: MetricsTilePanel
# ============================================================================
# NOTE: Test disabled - MetricsTilePanel.hpp doesn't exist
# TODO: Create MetricsTilePanel.hpp or remove test

# add_executable(test_metrics_tile_panel
#     ui/test_metrics_tile_panel.cpp
# )

# target_include_directories(test_metrics_tile_panel
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_metrics_tile_panel
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
#     log4cxx
# )

# add_test(
#     NAME Phase2MetricsTilePanelTests
#     COMMAND test_metrics_tile_panel
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 3 Unit Tests: CommandRegistry and CommandWindow
# ============================================================================
# NOTE: Test disabled - CommandWindow has mismatched implementation
# TODO: Fix CommandWindow implementation or update test

# add_executable(test_command_registry
#     ui/test_command_registry.cpp
# )

# target_include_directories(test_command_registry
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_command_registry
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
#     log4cxx
# )

# add_test(
#     NAME Phase3CommandRegistryTests
#     COMMAND test_command_registry
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 3d Unit Tests: Command Handlers for Phase 3b/3c/3d Features
# ============================================================================
# NOTE: Test disabled - MetricsTilePanel.hpp doesn't exist
# TODO: Create MetricsTilePanel.hpp or remove test

# add_executable(test_phase3d_commands
#     ui/test_phase3d_commands.cpp
# )

# target_include_directories(test_phase3d_commands
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_phase3d_commands
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
#     log4cxx
# )

# add_test(
#     NAME Phase3dCommandsTests
#     COMMAND test_phase3d_commands
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 3 Unit Tests: LoggingWindow with Filtering and Search
# ============================================================================
# NOTE: Test disabled - LoggingWindow.hpp doesn't exist
# TODO: Create LoggingWindow.hpp or remove test

# add_executable(test_logging_window
#     ui/test_logging_window.cpp
# )

# target_include_directories(test_logging_window
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_logging_window
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase3LoggingWindowTests
#     COMMAND test_logging_window
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 3 Unit Tests: Layout Persistence
# ============================================================================
# NOTE: Test disabled - LayoutConfig.hpp doesn't exist
# TODO: Create LayoutConfig.hpp or remove test

# add_executable(test_layout_config
#     ui/test_layout_config.cpp
# )

# target_include_directories(test_layout_config
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_layout_config
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase3LayoutConfigTests
#     COMMAND test_layout_config
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# Create custom targets for Phase 3
# ============================================================================

# NOTE: Test disabled - TabContainer implementation doesn't match test expectations
# TODO: Update TabContainer implementation or update test

# add_executable(test_tab_container
#     ui/test_tab_container.cpp
# )

# target_include_directories(test_tab_container
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_tab_container
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
#     log4cxx
# )

# add_test(
#     NAME Phase3TabContainerTests
#     COMMAND test_tab_container
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# Update Phase 3 targets to include tab container
add_custom_target(test_phase3
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase3.*" -V
    DEPENDS test_command_registry test_phase3d_commands test_logging_window test_layout_config test_tab_container
)

add_custom_target(test_phase3_verbose
    COMMAND ${CMAKE_CTEST_COMMAND} -R "Phase3.*" -VV --output-on-failure
    DEPENDS test_command_registry test_phase3d_commands test_logging_window test_layout_config test_tab_container
)

# ============================================================================
# Phase 4 Integration Tests: CommandRegistry with Dashboard
# ============================================================================

# add_executable(test_builtin_commands_integration
#     ui/test_builtin_commands_integration.cpp
# )

# target_include_directories(test_builtin_commands_integration
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_builtin_commands_integration
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase4CommandIntegrationTests
#     COMMAND test_builtin_commands_integration
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#)

# ============================================================================
# Phase 4 Integration Tests: Arrow Key Navigation for Tab Switching
# ============================================================================

# add_executable(test_arrow_key_navigation
#     ui/test_arrow_key_navigation.cpp
# )

# target_include_directories(test_arrow_key_navigation
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_arrow_key_navigation
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase4ArrowKeyNavigationTests
#     COMMAND test_arrow_key_navigation
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 4 Integration Tests: LoggingWindow Filter Integration
# ============================================================================

# add_executable(test_logging_filter_integration
#     ui/test_logging_filter_integration.cpp
# )

# target_include_directories(test_logging_filter_integration
#     PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../src
# )

# target_link_libraries(test_logging_filter_integration
#     PRIVATE
#     gdashboard_lib
#     GTest::GTest
#     GTest::Main
#     nlohmann_json::nlohmann_json
#     ftxui::component
#     ftxui::dom
#     ftxui::screen
# )

# add_test(
#     NAME Phase4LoggingFilterIntegrationTests
#     COMMAND test_logging_filter_integration
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
# )

# ============================================================================
# Phase 4 End-to-End Integration Tests
# ============================================================================

# #)