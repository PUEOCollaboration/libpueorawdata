#!/usr/bin/env bash
# Simple smoke test runner for libpueorawdata
# Tests raw, .gz and .zst roundtrip using the bundled test binaries

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

WRITER_WF="$BUILD_DIR/write-wf"
READER_WF="$BUILD_DIR/read-wf"

if [[ ! -x "$WRITER_WF" || ! -x "$READER_WF" ]]; then
  echo "Please build the project first (mkdir build && cd build && cmake .. && make)"
  exit 2
fi

TMPDIR="$BUILD_DIR/smoke_tmp"
rm -rf "$TMPDIR"
mkdir -p "$TMPDIR"

fname_uncompressed="${TMPDIR}/test.wf"
fname_zstd="${TMPDIR}/test.zst"
fname_gzip="${TMPDIR}/test.gz"

${WRITER_WF} ${fname_uncompressed}
${WRITER_WF} ${fname_zstd}
${WRITER_WF} ${fname_gzip}

if (( $(stat -c %s "$fname_zstd") == $(stat -c %s "$fname_gzip") )); then
  echo "Warning: zstd and gzip have the same size; this probably means zstd support does not work."
  exit 1
fi

if (( $(diff <(zstdcat ${fname_zstd}) <(cat ${fname_uncompressed})) )); then
  echo "Error: zstd compression does not read correctly"
  exit 1
fi

echo "All smoke tests passed. Clean up: $TMPDIR contains test files if you need them."
exit 0
