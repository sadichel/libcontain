#!/bin/bash

# Code formatter for libcontain
set -e

echo "========================================="
echo "  libcontain Code Formatter"
echo "========================================="

# Check for clang-format
if ! command -v clang-format &> /dev/null; then
    echo "clang-format not found. Please install:"
    echo "  Ubuntu: sudo apt install clang-format"
    echo "  macOS:  brew install clang-format"
    exit 1
fi

echo "Formatting headers..."
for file in include/contain/*.h; do
    if [ -f "$file" ]; then
        echo "  $file"
        clang-format -i "$file"
    fi
done

echo "Formatting typed headers..."
for file in include/contain/typed/*.h; do
    if [ -f "$file" ]; then
        echo "  $file"
        clang-format -i "$file"
    fi
done

echo "Formatting source files..."
for file in src/*.c; do
    if [ -f "$file" ]; then
        echo "  $file"
        clang-format -i "$file"
    fi
done

echo "Formatting test files..."
for file in tests/*.c; do
    if [ -f "$file" ]; then
        echo "  $file"
        clang-format -i "$file"
    fi
done

echo "Formatting benchmark files..."
for file in benchmarks/*.c; do
    if [ -f "$file" ]; then
        echo "  $file"
        clang-format -i "$file"
    fi
done

echo ""
echo "Format complete!"