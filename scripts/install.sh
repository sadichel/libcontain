#!/bin/bash

# Install script for libcontain
set -e

PREFIX=${PREFIX:-/usr/local}

echo "========================================="
echo "  libcontain Install Script"
echo "========================================="

echo "Installing to $PREFIX..."

# Build if needed
if [ ! -f build/libcontain.a ]; then
    echo "Building library..."
    make
fi

# Install
sudo make install PREFIX="$PREFIX"

echo ""
echo "Installation complete!"
echo "Headers installed in: $PREFIX/include/contain"
echo "Library installed in: $PREFIX/lib/libcontain.a"