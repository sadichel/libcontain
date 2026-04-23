#!/bin/bash

# Setup script for development environment
set -e

echo "========================================="
echo "  libcontain Setup Script"
echo "========================================="

# Install development dependencies
if command -v apt-get &> /dev/null; then
    echo "Installing dependencies (Ubuntu/Debian)..."
    sudo apt-get update
    sudo apt-get install -y build-essential clang clang-format clang-tidy valgrind
elif command -v brew &> /dev/null; then
    echo "Installing dependencies (macOS)..."
    brew install clang-format clang-tidy valgrind
else
    echo "Please install: clang, clang-format, clang-tidy, valgrind"
fi

# Build library
echo ""
echo "Building library..."
make

# Run tests
echo ""
echo "Running tests..."
make test

echo ""
echo "========================================="
echo "  Setup complete!"
echo "========================================="