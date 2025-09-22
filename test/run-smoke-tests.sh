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

fail() {
  echo "FAILED: $*"
  exit 1
}

run_case() {
  local suffix="$1"
  local fname="$TMPDIR/test${suffix}"
  echo "\n=== Test: $suffix -> $fname ==="
  # write
  "$WRITER_WF" "$fname"
  # read
  set +e
  out=$("$READER_WF" "$fname" 2>&1)
  rc=$?
  set -e
  echo "$out"
  if [[ $rc -ne 0 ]]; then
    fail "reader returned $rc for $fname"
  fi
  # quick heuristic: reader should have printed at least one 'read:' line
  if ! echo "$out" | grep -q "read:"; then
    fail "reader output did not contain expected 'read:' lines for $fname"
  fi
  echo "PASS: $suffix"
}

run_case ""    # raw
run_case ".gz"  # gzip
run_case ".zst" # zstd via zlibWrapper

echo "All smoke tests passed. Clean up: $TMPDIR contains test files if you need them." 
exit 0
