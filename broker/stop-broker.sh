#!/usr/bin/env bash
# ============================================================================
# stop-broker.sh — Stop the running Mosquitto broker
# Usage: bash broker/stop-broker.sh [pqc|classical]
# ============================================================================

set -euo pipefail

MODE="${1:-pqc}"
PID_FILE="/tmp/mosquitto-${MODE}.pid"

if [ ! -f "${PID_FILE}" ]; then
    echo "[broker] No PID file found for mode '${MODE}'. Is the broker running?"
    exit 1
fi

BROKER_PID=$(cat "${PID_FILE}")

if kill -0 "${BROKER_PID}" 2>/dev/null; then
    echo "[broker] Stopping Mosquitto (PID: ${BROKER_PID}) ..."
    kill "${BROKER_PID}"
    rm -f "${PID_FILE}"
    echo "[broker] ✓ Stopped."
else
    echo "[broker] Process ${BROKER_PID} is not running. Cleaning up PID file."
    rm -f "${PID_FILE}"
fi
