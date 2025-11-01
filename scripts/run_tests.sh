#!/bin/bash
# Balance - Test Runner Script

set -e

cd "$(dirname "$0")/.."

if [ ! -d "build" ]; then
    echo "Error: Build directory not found. Run build.sh first."
    exit 1
fi

cd build

echo "======================================"
echo "Running Balance Tests"
echo "======================================"
echo ""

# Run tests with CTest
ctest --output-on-failure --verbose

echo ""
echo "======================================"
echo "All tests completed!"
echo "======================================"
