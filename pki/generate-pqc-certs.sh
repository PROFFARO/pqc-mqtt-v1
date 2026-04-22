#!/usr/bin/env bash
# ============================================================================
# generate-pqc-certs.sh — Generate ML-DSA certificate chain for PQC-TLS
# Produces: Root CA (ML-DSA-87) → Broker cert (ML-DSA-65) → Client cert (ML-DSA-65)
# Prerequisites: oqs-provider built and configured
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../toolchain/env.sh"

OPENSSL="${PQC_INSTALL_PREFIX}/bin/openssl"
CONF="${SCRIPT_DIR}/openssl-pqc.cnf"
OUT="${PQC_CERTS_DIR}"
DAYS=365

# --- Signature algorithms ---
CA_ALG="mldsa87"
BROKER_ALG="mldsa65"
CLIENT_ALG="mldsa65"

echo "============================================"
echo " PQC-MQTT: Generating PQC Certificate Chain"
echo " CA:     ${CA_ALG}"
echo " Broker: ${BROKER_ALG}"
echo " Client: ${CLIENT_ALG}"
echo "============================================"

export OPENSSL_CONF="${CONF}"
export OPENSSL_MODULES="${PQC_INSTALL_PREFIX}/lib64/ossl-modules"

mkdir -p "${OUT}"

# ---- 1. Root CA (self-signed) ----
echo "[pki] Generating Root CA ..."
${OPENSSL} req -x509 -new \
    -newkey "${CA_ALG}" \
    -keyout "${OUT}/ca.key" \
    -out    "${OUT}/ca.crt" \
    -nodes \
    -days "${DAYS}" \
    -subj "/C=IN/ST=Research/L=PQC-Lab/O=PQC-MQTT-Research/OU=Quantum-Safe/CN=PQC Root CA (${CA_ALG})" \
    -extensions v3_ca \
    -config "${CONF}"

echo "[pki] ✓ Root CA created: ${OUT}/ca.crt"

# ---- 2. Broker Certificate ----
echo "[pki] Generating Broker CSR ..."
${OPENSSL} req -new \
    -newkey "${BROKER_ALG}" \
    -keyout "${OUT}/broker.key" \
    -out    "${OUT}/broker.csr" \
    -nodes \
    -subj "/C=IN/ST=Research/L=PQC-Lab/O=PQC-MQTT-Research/OU=Broker/CN=pqc-broker" \
    -config "${CONF}"

echo "[pki] Signing Broker certificate with CA ..."
${OPENSSL} x509 -req \
    -in "${OUT}/broker.csr" \
    -CA "${OUT}/ca.crt" \
    -CAkey "${OUT}/ca.key" \
    -CAcreateserial \
    -out "${OUT}/broker.crt" \
    -days "${DAYS}" \
    -extfile "${CONF}" \
    -extensions v3_broker

echo "[pki] ✓ Broker cert created: ${OUT}/broker.crt"

# ---- 3. Client Certificate ----
echo "[pki] Generating Client CSR ..."
${OPENSSL} req -new \
    -newkey "${CLIENT_ALG}" \
    -keyout "${OUT}/client.key" \
    -out    "${OUT}/client.csr" \
    -nodes \
    -subj "/C=IN/ST=Research/L=PQC-Lab/O=PQC-MQTT-Research/OU=Client/CN=pqc-client" \
    -config "${CONF}"

echo "[pki] Signing Client certificate with CA ..."
${OPENSSL} x509 -req \
    -in "${OUT}/client.csr" \
    -CA "${OUT}/ca.crt" \
    -CAkey "${OUT}/ca.key" \
    -CAcreateserial \
    -out "${OUT}/client.crt" \
    -days "${DAYS}" \
    -extfile "${CONF}" \
    -extensions v3_client

echo "[pki] ✓ Client cert created: ${OUT}/client.crt"

# ---- Summary ----
echo ""
echo "============================================"
echo " PQC Certificate Chain — Summary"
echo "============================================"
echo ""
echo "--- Root CA ---"
${OPENSSL} x509 -in "${OUT}/ca.crt" -noout -subject -issuer -dates
echo ""
echo "--- Broker ---"
${OPENSSL} x509 -in "${OUT}/broker.crt" -noout -subject -issuer -dates
echo ""
echo "--- Client ---"
${OPENSSL} x509 -in "${OUT}/client.crt" -noout -subject -issuer -dates
echo ""
echo "--- File sizes (PQC overhead indicator) ---"
ls -lh "${OUT}"/*.crt "${OUT}"/*.key
echo ""
echo "[pki] ✓ PQC certificate chain complete."
