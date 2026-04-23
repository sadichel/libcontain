#!/bin/bash

# Test runner for libcontain
set -e

echo "========================================="
echo "  libcontain Test Suite"
echo "========================================="

# Build tests if needed
if [ ! -f build/libcontain.a ]; then
    echo "Building library..."
    make
fi

# Run unit tests
echo ""
echo "Running unit tests..."
for test in build/tests/*_test; do
    if [ -f "$test" ] && [ -x "$test" ]; then
        echo ""
        echo "=== $(basename $test) ==="
        ./"$test"
    fi
done

# Run tour
echo ""
echo "=== tour ==="
if [ ! -f build/tests/tour ]; then
    echo "Building tour..."
    make tour
fi
./build/tests/tour

echo ""
echo "========================================="
echo "  All tests passed!"
echo "========================================="