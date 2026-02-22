#!/bin/bash

# Debug build script for CRS-DSO
# Usage: ./build-debug.sh [--run] [--clean] [--verbose]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/src/crs-dso"

# Detect number of CPU cores for parallel build
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse arguments
RUN_AFTER_BUILD=false
CLEAN_BUILD=false
VERBOSE=""

for arg in "$@"; do
    case $arg in
        --run|-r)
            RUN_AFTER_BUILD=true
            ;;
        --clean|-c)
            CLEAN_BUILD=true
            ;;
        --verbose|-v)
            VERBOSE="VERBOSE=1"
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --run, -r      Run the application after building"
            echo "  --clean, -c    Clean build directory before building"
            echo "  --verbose, -v  Show verbose build output"
            echo "  --help, -h     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"

# Configure
echo "Configuring (Debug)..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo "Building with ${JOBS} parallel jobs..."
make -C "${BUILD_DIR}" -j${JOBS} ${VERBOSE}

# Run if requested
if [ "$RUN_AFTER_BUILD" = true ]; then
    echo ""
    echo "Running crs-dso..."
    echo "----------------------------------------"
    "${EXECUTABLE}"
fi

echo ""
echo "Build complete: ${EXECUTABLE}"

