#!/bin/bash

# Clean script for libcontain
set -e

echo "========================================="
echo "  libcontain Clean Script"
echo "========================================="

# Clean build artifacts
echo "Cleaning build artifacts..."
make clean

# Remove coverage files
echo "Removing coverage files..."
rm -f build/*.gcda build/*.gcno build/*.gcov
rm -f build/coverage.html build/*.css

# Remove benchmark binaries
echo "Removing benchmark binaries..."
rm -f build/benchmarks/bench_* build/benchmarks/bench-*

# Remove test binaries
echo "Removing test binaries..."
rm -f build/tests/*_test
rm -f build/tests/tour

# Remove example binaries
echo "Removing example binaries..."
rm -f examples/*

echo ""
echo "Clean complete!"