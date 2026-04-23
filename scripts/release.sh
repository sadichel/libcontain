#!/bin/bash

# Release script for libcontain
set -e

VERSION=${1:-1.0.0}

echo "========================================="
echo "  libcontain Release Script v$VERSION"
echo "========================================="

# Run tests
echo "Running tests..."
./scripts/test.sh

# Run benchmarks
echo "Running benchmarks..."
./scripts/bench.sh

# Build documentation
echo "Building documentation..."
# Add documentation build commands here

# Create release archive
echo "Creating release archive..."
git archive --format=tar --prefix=libcontain-$VERSION/ HEAD | gzip > libcontain-$VERSION.tar.gz

echo ""
echo "Release prepared: libcontain-$VERSION.tar.gz"
echo "========================================="