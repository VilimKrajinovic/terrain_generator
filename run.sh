#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE="$BUILD_DIR/terrain_sim"

if [ ! -f "$EXECUTABLE" ]; then
    echo "Executable not found. Run ./build.sh first."
    exit 1
fi

cd "$BUILD_DIR"
./terrain_sim "$@"
