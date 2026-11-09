#!/usr/bin/env bash
# Configure, build and test. Usage: scripts/build.sh [Release|Debug]
set -euo pipefail
cd "$(dirname "$0")/.."
TYPE="${1:-Release}"
QT_PREFIX="$(brew --prefix qt)"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE="$TYPE" -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build build
ctest --test-dir build --output-on-failure
