#!/usr/bin/env bash
set -euo pipefail

# Build the library (assumes top-level CMake has been run and build dir exists)
# This script will try to build the project if necessary and then compile/run the smoke test.

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
TEST_SRC="${REPO_ROOT}/test/smoke_encode_decode.c"
TEST_BIN="${BUILD_DIR}/smoke_encode_decode"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# If Makefile doesn't exist, run cmake
if [ ! -f Makefile ]; then
  cmake ..
fi

# Build the library target
make -j 2 pueorawdata || true

# Compile the test program linking against the built library
cc -I"${REPO_ROOT}/inc" -I"${REPO_ROOT}/src" -L. -Wl,-rpath,. "${TEST_SRC}" -lpueorawdata -o "${TEST_BIN}"

# Run it
"${TEST_BIN}" 

echo "Smoke test completed." 
