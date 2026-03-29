#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"

echo "==> Cleaning build directory..."
cd "$BUILD_DIR"
rm -rf *

echo "==> Running CMake setup (Xcode generator)..."
cmake -G Xcode ..

echo "==> Building debug package..."
cmake --build . --target MacOSNotePP_package --config Debug

echo "==> Done. App bundle is at: $(cd "$SCRIPT_DIR/../dist" && pwd)/MacNote++.app"
