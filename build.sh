#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$root_dir/build"

cmake -S "$root_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
# cmake -S "$root_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir"
