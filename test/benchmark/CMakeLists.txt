# ============================================================================
# Performance Benchmarks (google-benchmark)
# ============================================================================
# Benchmarks measure performance of critical hot paths:
# - ActiveQueue: lock-free queue operations
# - ThreadPool: worker loop and task scheduling
# - Message Routing: port and message creation
# - NodeFactory: node creation and plugin overhead

if(NOT benchmark_FOUND)
    message(FATAL_ERROR "google-benchmark required for benchmarks")
endif()

# Helper to add benchmark target
function(add_benchmark target_name source_file)
    add_executable(${target_name} ${source_file})

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    target_link_libraries(${target_name}
        gdashboard_lib
        benchmark::benchmark
        log4cxx
        pthread
    )

    # Add coverage instrumentation if enabled
    if(ENABLE_COVERAGE)
        target_compile_options(${target_name} PRIVATE --coverage)
        target_link_options(${target_name} PRIVATE --coverage)
    endif()

    # Add profiling support if enabled
    if(ENABLE_PROFILING)
        target_compile_options(${target_name} PRIVATE -g -p -fno-omit-frame-pointer -fno-inline)
        target_link_options(${target_name} PRIVATE -g -p)
    endif()
endfunction()

# ============================================================================
# Benchmark Targets
# ============================================================================

# ActiveQueue benchmarks - lock-free queue operations
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_active_queue.cpp")
    add_benchmark(benchmark_active_queue benchmark_active_queue.cpp)
endif()

# ThreadPool benchmarks - worker loop and scheduling
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_thread_pool.cpp")
    add_benchmark(benchmark_thread_pool benchmark_thread_pool.cpp)
endif()

# Message routing benchmarks - port and message creation
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_message_routing.cpp")
    add_benchmark(benchmark_message_routing benchmark_message_routing.cpp)
endif()

# NodeFactory benchmarks - node creation and plugin overhead
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark_node_factory.cpp")
    add_benchmark(benchmark_node_factory benchmark_node_factory.cpp)
endif()

message(STATUS "Benchmark targets configured")
