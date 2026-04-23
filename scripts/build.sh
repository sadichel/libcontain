#!/bin/bash

# Build script for libcontain
set -e

echo "========================================="
echo "  libcontain Build Script"
echo "========================================="

# Default build type
BUILD_TYPE=${1:-release}

case $BUILD_TYPE in
    release)
        echo "Building release version..."
        make clean
        make
        ;;
    debug)
        echo "Building debug version..."
        make clean
        make debug
        ;;
    test)
        echo "Building and running tests..."
        make clean
        make test
        ;;
    all)
        echo "Building everything..."
        make clean
        make all
        make test
        make tour
        ;;
    *)
        echo "Usage: $0 [release|debug|test|all]"
        echo "  release - Build release library (default)"
        echo "  debug   - Build debug library with CONTAINER_DEBUG"
        echo "  test    - Build and run tests"
        echo "  all     - Build everything"
        exit 1
        ;;
esac

echo ""
echo "Build complete!"