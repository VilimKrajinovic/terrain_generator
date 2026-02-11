#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE="$BUILD_DIR/terrain_sim"

usage() {
  echo "Usage: $0 [command]"
  echo ""
  echo "Commands:"
  echo "  (none)    Build the project"
  echo "  run       Build and run the project"
  echo "  clean     Remove build directory"
  echo "  rebuild   Clean and build"
  echo "  help      Show this help message"
}

build() {
  echo "Configuring..."
  cmake -S "$PROJECT_DIR" -B "$BUILD_DIR"

  echo "Building..."
  cmake --build "$BUILD_DIR"

  echo "Build complete: $EXECUTABLE"
}

run() {
  build
  echo "Running..."
  "$EXECUTABLE"
}

clean() {
  echo "Cleaning..."
  rm -rf "$BUILD_DIR"
  echo "Clean complete"
}

case "${1:-}" in
"")
  build
  ;;
run)
  run
  ;;
clean)
  clean
  ;;
rebuild)
  clean
  build
  ;;
help | --help | -h)
  usage
  ;;
*)
  echo "Unknown command: $1"
  usage
  exit 1
  ;;
esac
