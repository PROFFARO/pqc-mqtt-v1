#!/usr/bin/env bash
# ============================================================================
# capture-handshake.sh — Capture TLS handshake packets with tshark
#
# Usage: bash eval/capture/capture-handshake.sh [pqc|classical] [duration_sec]
# Output: eval/results/raw/<mode>_handshake.pcap
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../../toolchain/env.sh"

MODE="${1:-pqc}"
DURATION="${2:-30}"
IFACE="${3:-lo}"   # loopback for localhost testing

case "${MODE}" in
    pqc)       PORT=8883 ;;
    classical) PORT=8884 ;;
    *)
        echo "Usage: $0 [pqc|classical] [duration_sec] [interface]"
        exit 1
        ;;
esac

mkdir -p "${EVAL_RAW_DIR}"
PCAP_FILE="${EVAL_RAW_DIR}/${MODE}_handshake.pcap"

echo "[capture] Interface: ${IFACE}"
echo "[capture] Port:      ${PORT}"
echo "[capture] Duration:  ${DURATION}s"
echo "[capture] Output:    ${PCAP_FILE}"
echo ""
echo "[capture] Starting capture ... (run your client in another terminal)"

tshark -i "${IFACE}" \
    -f "tcp port ${PORT}" \
    -a "duration:${DURATION}" \
    -w "${PCAP_FILE}" \
    -q

echo ""
echo "[capture] ✓ Capture complete: ${PCAP_FILE}"
echo "[capture] Analyze with: python3 eval/analysis/parse_pcap.py ${PCAP_FILE}"
