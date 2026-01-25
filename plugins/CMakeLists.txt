cmake_minimum_required(VERSION 3.23)

# Include the plugin template
include(${CMAKE_SOURCE_DIR}/cmake/PluginTemplate.cmake)

# ============================================================================
# Plugin Directory Setup
# ============================================================================

add_graphx_plugins_directory()

# ============================================================================
# Define Plugins
# ============================================================================

# FlightFSMNode Plugin - Merge node with 5 inputs
add_graphx_plugin(
    TARGET_NAME flight_fsm_node
    SOURCE_FILE flight_fsm_node_plugin.cpp 
    DESCRIPTION "Flight state machine node "
)

# DataInjectionAccelerometerNode Plugin - CSV data source
add_graphx_plugin(
    TARGET_NAME csv_accelerometer_node
    SOURCE_FILE csv_accelerometer_node_plugin.cpp
    DESCRIPTION "CSV accelerometer data source node"
)

# DataInjectionBarometricNode Plugin - CSV data source
add_graphx_plugin(
    TARGET_NAME csv_barometric_node
    SOURCE_FILE csv_barometric_node_plugin.cpp
    DESCRIPTION "CSV barometric data source node"
)

# DataInjectionGPSPositionNode Plugin - CSV data source
add_graphx_plugin(
    TARGET_NAME csv_gps_position_node
    SOURCE_FILE csv_gps_position_node_plugin.cpp
    DESCRIPTION "CSV gps position data source node"
)

# DataInjectionMagnetometerNode Plugin - CSV data source
add_graphx_plugin(
    TARGET_NAME csv_magnetometer_node
    SOURCE_FILE csv_magnetometer_node_plugin.cpp
    DESCRIPTION "CSV magnetometer data source node"
)

# DataInjectionGyroscopeNode Plugin - CSV data source
add_graphx_plugin(
    TARGET_NAME csv_gyroscope_node
    SOURCE_FILE csv_gyroscope_node_plugin.cpp
    DESCRIPTION "CSV gyroscope data source node"
)

# FlightLoggerNode Plugin - Sink/logger node
add_graphx_plugin(
    TARGET_NAME flight_logger_node
    SOURCE_FILE flight_logger_node_plugin.cpp
    DESCRIPTION "Flight data logging sink node"
)

# CompletionAggregratorNode Plugin - Sink/logger node
add_graphx_plugin(
    TARGET_NAME completion_aggregator_node
    SOURCE_FILE completion_aggregator_node_plugin.cpp
    DESCRIPTION "Aggregration node"
)

# AltitudeFusionNode Plugin - AltitudeFusion
add_graphx_plugin(
    TARGET_NAME altitude_fusion_node
    SOURCE_FILE altitude_fusion_node_plugin.cpp ../src/avionics/nodes/AltitudeFusionNode.cpp ../src/avionics/estimators/AltitudeFusionEstimator.cpp
    DESCRIPTION "Altitude Fusion Node"
)

# StateVectorCaptureSinkNode Plugin - StateVector Capture Node
add_graphx_plugin(
    TARGET_NAME state_vector_capture_sink_node
    SOURCE_FILE state_vector_capture_sink_node_plugin.cpp ../src/avionics/nodes/StateVectorCaptureSinkNode.cpp 
    DESCRIPTION "State Vector Capture Node"
)

# FlightMonitorNode Plugin - StateVector Capture Node
add_graphx_plugin(
    TARGET_NAME flight_monitor_node
    SOURCE_FILE flight_monitor_node_plugin.cpp ../src/avionics/FlightMonitorNode.cpp 
    DESCRIPTION "Flight Monitor Node"
)

# EstimationPipelineNode Plugin - Estimation Pipeline
add_graphx_plugin(
    TARGET_NAME estimation_pipeline_node
    SOURCE_FILE estimation_pipeline_node_plugin.cpp ../src/avionics/nodes/EstimationPipelineNode.cpp 
    DESCRIPTION "Estimation Pipeline Node"
)

# FeedbackTestSinkNode
add_graphx_plugin(
    TARGET_NAME feedback_test_sink_node
    SOURCE_FILE feedback_test_sink_node_plugin.cpp 
    DESCRIPTION "Feedback Test Sink Node"
)
# ============================================================================
# Print Summary
# ============================================================================

message(STATUS "GraphX Plugins Summary")
message(STATUS "====================")
message(STATUS "✓ flight_fsm_node (Flight state machine)")
message(STATUS "✓ csv_accelerometer_node (CSV data source)")
message(STATUS "✓ flight_logger_node (Flight data logger)")
message(STATUS "")
