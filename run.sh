#!/usr/bin/env bash
# ============================================================================
# run.sh — Wrapper to run any PQC-MQTT binary with correct environment
#
# Usage:
#   bash run.sh ./build/pqc_publisher --mode pqc --count 5
#   bash run.sh ./build/handshake_bench --mode pqc --iterations 100
#   bash run.sh ./build/throughput_bench --mode pqc --count 1000 --qos 1
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/toolchain/env.sh"

# --- Critical: Force our custom OpenSSL over system OpenSSL ---
export LD_LIBRARY_PATH="${PQC_INSTALL_PREFIX}/lib64:${PQC_INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"

# --- PQC provider configuration ---
export OPENSSL_MODULES="${PQC_INSTALL_PREFIX}/lib64/ossl-modules"
export OPENSSL_CONF="${PQC_PROJECT_ROOT}/pki/openssl-pqc.cnf"

# --- Verify provider loads (quick check) ---
if ! "${PQC_INSTALL_PREFIX}/bin/openssl" list -providers 2>/dev/null | grep -q oqsprovider; then
    echo "[run] WARNING: oqs-provider not detected. PQC mode may fail."
fi

if [ $# -eq 0 ]; then
    echo "Usage: bash run.sh <command> [args...]"
    echo ""
    echo "Examples:"
    echo "  bash run.sh ./build/pqc_publisher --mode pqc --count 5"
    echo "  bash run.sh ./build/handshake_bench --mode pqc --iterations 100"
    echo "  bash run.sh ./build/stress_test --mode pqc --clients 50"
    exit 1
fi

echo "[run] LD_LIBRARY_PATH includes: ${PQC_INSTALL_PREFIX}/lib64"
echo "[run] OPENSSL_MODULES = ${OPENSSL_MODULES}"
echo "[run] Executing: $@"
echo ""

exec "$@"
