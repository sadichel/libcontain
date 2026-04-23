#!/bin/bash

# Uninstall script for libcontain
set -e

PREFIX=${PREFIX:-/usr/local}

echo "========================================="
echo "  libcontain Uninstall Script"
echo "========================================="

echo "Removing from $PREFIX..."

sudo rm -rf "$PREFIX/include/contain"
sudo rm -f "$PREFIX/lib/libcontain.a"

echo ""
echo "Uninstall complete!"