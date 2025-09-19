#!/usr/bin/env bash

# Simple install script for libpueorawdata.
# This script assumes you have CMake and a C++ compiler installed.
# It will create a build directory, configure the project, and build it.

#First make sure zstd submodule is initialized
bash scripts/init_zstd_submodule.sh

#Second, build zstd if not already built
if [ ! -f third_party/zstd/lib/libzstd.a ]; then
  echo "Building zstd..."
  cd third_party/zstd
  make -j$(nproc)
  cd ../../..

echo "zstd built."
fi

#Next, build libpueorawdata
cmake -S . -B build -D CMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
echo "libpueorawdata built.  Scripts available in build/"