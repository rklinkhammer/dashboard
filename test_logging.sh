#!/bin/bash

# Test script to check logging appender registration and message routing

cd /Users/rklinkhammer/workspace/dashboard/build

# Run the logging window test to see if messages are being routed
echo "=== Running logging window test with debug output ==="
./bin/test_logging_window 2>&1 | grep -E "(\[LoggingWindowAppender|Dashboard::Initialize|PASSED)"

