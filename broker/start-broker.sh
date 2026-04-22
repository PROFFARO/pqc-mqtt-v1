#!/usr/bin/env bash
# ============================================================================
# start-broker.sh — Launch the PQC-enabled Mosquitto broker
# Usage: bash broker/start-broker.sh [pqc|classical]
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../toolchain/env.sh"

MODE="${1:-pqc}"
MOSQUITTO_BIN="${MOSQUITTO_SRC_DIR}/src/mosquitto"
PID_FILE="/tmp/mosquitto-${MODE}.pid"

case "${MODE}" in
    pqc)
        CONF="${SCRIPT_DIR}/mosquitto-pqc.conf"
        echo "[broker] Starting Mosquitto in PQC-TLS mode ..."
        ;;
    classical)
        CONF="${SCRIPT_DIR}/mosquitto-classical.conf"
        echo "[broker] Starting Mosquitto in Classical-TLS mode ..."
        ;;
    *)
        echo "Usage: $0 [pqc|classical]"
        exit 1
        ;;
esac

if [ ! -f "${MOSQUITTO_BIN}" ]; then
    echo "[broker] ERROR: Mosquitto binary not found at ${MOSQUITTO_BIN}"
    echo "[broker] Run: bash toolchain/build-mosquitto.sh"
    exit 1
fi

# Export env so Mosquitto picks up our custom OpenSSL + oqs-provider
export OPENSSL_CONF="${PQC_PROJECT_ROOT}/pki/openssl-pqc.cnf"
export OPENSSL_MODULES="${PQC_INSTALL_PREFIX}/lib64/ossl-modules"

cd "${PQC_PROJECT_ROOT}"

echo "[broker] Config: ${CONF}"
echo "[broker] Binary: ${MOSQUITTO_BIN}"
echo "[broker] PID:    ${PID_FILE}"

# Launch in foreground (Ctrl+C to stop) or background with &
${MOSQUITTO_BIN} -c "${CONF}" -v &
BROKER_PID=$!
echo "${BROKER_PID}" > "${PID_FILE}"

echo "[broker] ✓ Mosquitto started (PID: ${BROKER_PID})"
echo "[broker] Stop with: bash broker/stop-broker.sh ${MODE}"

wait "${BROKER_PID}"
