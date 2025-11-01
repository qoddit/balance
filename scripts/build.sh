#!/bin/bash
# Balance - Linux/macOS Build Script

set -e  # Exit on error

echo "======================================"
echo "Balance Encryption Toolkit - Builder"
echo "======================================"
echo ""

# Configuration
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"
echo "Parallel jobs: $JOBS"
echo ""

# Create build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo ""
echo "Building..."
make -j"$JOBS"

echo ""
echo "======================================"
echo "Build completed successfully!"
echo "======================================"
echo ""
echo "Executable: $BUILD_DIR/balance"
echo "Tests: $BUILD_DIR/tests/balance_tests"
echo ""
echo "To run the application:"
echo "  cd $BUILD_DIR && ./balance"
echo ""
echo "To run tests:"
echo "  cd $BUILD_DIR && ctest --output-on-failure"
echo ""
