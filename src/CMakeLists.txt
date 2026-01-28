# ============================================================================
# Core Source Build Configuration
# ============================================================================

# Collect all source files
# Note: We only build Phase 0 mock for now
# GraphExecutorBuilder.cpp requires additional dependencies not yet configured
set(GDASHBOARD_SOURCES
    graph/GraphConfigParser.cpp
    graph/GraphExecutor.cpp
    graph/GraphExecutorBuilder.cpp
    graph/ExecutionState.cpp
    graph/EdgeRegistration.cpp
    graph/EdgeRegistry.cpp
    graph/JsonDynamicGraphLoader.cpp
    graph/JsonView.cpp
    graph/NodeFacade.cpp
    graph/NodeFactory.cpp
    graph/NodeFactoryRegistry.cpp
    graph/SensorClassification.cpp
    graph/StaticNodeAdapter.cpp
    graph/ThreadPool.cpp
    # ui/Window.cpp
    # ui/MetricsPanel.cpp
    # ui/LoggingWindow.cpp
    # ui/LoggingAppender.cpp
    # ui/LoggingWindowAppender.cpp
    # #ui/CommandWindow.cpp
    ui/CommandRegistry.cpp
    ui/BuiltinCommands.cpp
    # ui/LayoutConfig.cpp
    # ui/TabContainer.cpp
    # ui/StatusBar.cpp
    # ui/MetricsTileWindow.cpp
    # ui/MetricsTilePanel.cpp
    # ui/NodeMetricsTile.cpp
    ui/Dashboard.cpp
    app/FactoryManager.cpp
    app/GraphBuilder.cpp
    app/policies/MetricsPolicy.cpp
    app/policies/CompletionPolicy.cpp
    app/policies/DataInjectionPolicy.cpp
    app/SignalHandler.cpp
    csv/CSVDataInjectionManager.cpp
    csv/CSVParser.cpp
    plugins/PluginLoader.cpp
    plugins/PluginRegistry.cpp
    graph/mock/MockGraphExecutor.cpp
    avionics/estimators/SensorVoter.cpp
    avionics/FlightPhaseClassifier.cpp
    avionics/FeedbackParameterComputer.cpp
    avionics/FlightMonitorNode.cpp
    avionics/nodes/AltitudeFusionNode.cpp
    avionics/nodes/StateVectorCaptureSinkNode.cpp
    avionics/estimators/AngularRateIntegrator.cpp
    avionics/estimators/AltitudeFusionEstimator.cpp
    avionics/estimators/SensorVoter.cpp
    avionics/estimators/VelocityCrossCheck.cpp
    avionics/estimators/MagnetometerIntegrator.cpp
    avionics/estimators/EKFCore.cpp
    avionics/estimators/AttitudeQuaternion.cpp
    avionics/estimators/ConfidenceAggregator.cpp
    avionics/nodes/EstimationPipelineNode.cpp
)

# Create library target
add_library(gdashboard_lib STATIC
    ${GDASHBOARD_SOURCES}
)

# Set include directories
target_include_directories(gdashboard_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(gdashboard_lib PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")
# Link dependencies
target_link_libraries(gdashboard_lib PUBLIC
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
)

# Create gdashboard executable
add_executable(gdashboard
    gdashboard/dashboard_main.cpp
)

# Link with library and dependencies
target_link_libraries(gdashboard PRIVATE
    gdashboard_lib
    ncurses
    Eigen3::Eigen
    log4cxx
)

target_compile_definitions(gdashboard PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

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
    target_compile_options(gdashboard_lib PRIVATE -Wall -Wextra -Wpedantic -Werror -finput-charset=UTF-8 -fexec-charset=UTF-8)
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

# # Create demo_gui executable
# add_executable(demo_gui
#     gdashboard/demo_gui_improved.cpp
#     ui/Dashboard.cpp
# )

# # Link with library and dependencies
# target_link_libraries(demo_gui PRIVATE
#     Eigen3::Eigen
#     log4cxx
#     ncurses
# )

# target_compile_definitions(demo_gui PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

# target_include_directories(demo_gui PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
# )


# # Set C++ standard
# set_target_properties(demo_gui PROPERTIES
#     CXX_STANDARD 23
#     CXX_STANDARD_REQUIRED ON
# )

# Compiler flags
# if(MSVC)
#     target_compile_options(demo_gui PRIVATE /W4 /WX)
# else()
#     target_compile_options(demo_gui PRIVATE -Wall -Wextra -Wpedantic -Werror -finput-charset=UTF-8 -fexec-charset=UTF-8)
# endif()
