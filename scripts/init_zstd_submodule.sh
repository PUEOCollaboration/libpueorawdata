#!/usr/bin/env bash
# Helper script to add zstd as a git submodule under third_party/zstd
# Run this from the project root (repo must be a git repo).

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -d .git ]; then
  echo "Error: current directory is not a git repository."
  echo "Please run the following commands from the project root yourself:"
  echo
  echo "  git submodule add https://github.com/facebook/zstd.git third_party/zstd"
  echo "  git submodule update --init --recursive"
  exit 2
fi

echo "Adding zstd as submodule under third_party/zstd..."
mkdir -p third_party
if git submodule status third_party/zstd >/dev/null 2>&1; then
  echo "Submodule third_party/zstd already present. Running update..."
  git submodule update --init --recursive third_party/zstd
else
  git submodule add https://github.com/facebook/zstd.git third_party/zstd
  git submodule update --init --recursive third_party/zstd
fi

echo "Done. Re-run CMake to pick up the bundled zlibWrapper sources:"
echo "  cmake -S . -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo"
echo "  cmake --build build"
