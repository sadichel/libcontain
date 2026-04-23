#!/bin/bash

# Benchmark runner for libcontain
set -e

echo "========================================="
echo "  libcontain Benchmark Suite"
echo "========================================="

# Build and run all benchmarks
echo "Building and running benchmarks..."
make bench-all 2>/dev/null || make bench

echo ""
echo "========================================="
echo "  libcontain Benchmarks complete"
echo "========================================="