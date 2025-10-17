#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FIRMWARE_DIR="${SCRIPT_DIR%/tools}"
APP_DIR="${FIRMWARE_DIR}/test_all_app"

PORT="${1:-}"

echo "[unity] Building test app at ${APP_DIR}"
(
  cd "$APP_DIR"
  idf.py build
)

if [[ -n "$PORT" ]]; then
  echo "[unity] Flashing to ${PORT} and monitoring for test output..."
  (
    cd "$APP_DIR"
    idf.py -p "$PORT" flash monitor
  )
else
  echo "[unity] Build complete. To flash & run tests:"
  echo "  cd ${APP_DIR} && idf.py -p <PORT> flash monitor"
  echo "(Pass serial port as first arg to this script to run automatically)"
fi

