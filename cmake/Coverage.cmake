# ============================================================================
# Code Coverage Support (gcov/lcov integration)
# ============================================================================
#
# This module provides coverage reporting functionality when ENABLE_COVERAGE=ON
#
# Usage:
#   cmake -DENABLE_COVERAGE=ON ..
#   make coverage
#
# Output:
#   coverage/index.html  - HTML report
#   coverage.json        - JSON report for CI/CD
#   coverage-summary.txt - Terminal summary
#

find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(ENABLE_COVERAGE)
    if(NOT LCOV_EXECUTABLE OR NOT GENHTML_EXECUTABLE)
        message(WARNING "lcov or genhtml not found. Coverage reports will not be generated.")
        message(STATUS "Install with: brew install lcov  (macOS) or apt-get install lcov (Linux)")
        return()
    endif()

    message(STATUS "Code Coverage Tools Found:")
    message(STATUS "  lcov: ${LCOV_EXECUTABLE}")
    message(STATUS "  genhtml: ${GENHTML_EXECUTABLE}")

    # Create coverage target that calls the coverage script
    add_custom_target(coverage
        COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/generate_coverage.sh" "${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )

    # Make coverage depend on all test targets
    if(TARGET test_phase0_unit)
        add_dependencies(coverage test_phase0_unit)
    endif()
    if(TARGET test_phase0_integration)
        add_dependencies(coverage test_phase0_integration)
    endif()
    if(TARGET test_phase2_metrics)
        add_dependencies(coverage test_phase2_metrics)
    endif()

    message(STATUS "Coverage target created. Run: make coverage")

else()
    message(STATUS "Code coverage disabled. Enable with: cmake -DENABLE_COVERAGE=ON ..")
endif()
