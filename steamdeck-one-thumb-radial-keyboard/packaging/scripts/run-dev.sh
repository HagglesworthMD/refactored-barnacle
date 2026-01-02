#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"

"${BUILD_DIR}/radialkb-engine" &
ENGINE_PID=$!

cleanup() {
  kill ${ENGINE_PID} >/dev/null 2>&1 || true
}
trap cleanup EXIT

"${BUILD_DIR}/radialkb-ui"
