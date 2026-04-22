#!/usr/bin/env bash
# ============================================================================
# generate-classical-certs.sh — Generate RSA + ECDSA certificate chains
# Used as performance baselines to compare against PQC certificates.
# Prerequisites: OpenSSL (system or custom) available
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../toolchain/env.sh"

OPENSSL="${PQC_INSTALL_PREFIX}/bin/openssl"
CONF="${SCRIPT_DIR}/openssl-classical.cnf"
OUT="${CLASSICAL_CERTS_DIR}"
DAYS=365

echo "============================================"
echo " PQC-MQTT: Generating Classical Cert Chains"
echo "============================================"

# ================================================================
# RSA-2048 Chain
# ================================================================
RSA_DIR="${OUT}/rsa"
mkdir -p "${RSA_DIR}"

echo "[pki-classical] --- RSA-2048 Chain ---"

${OPENSSL} req -x509 -new \
    -newkey rsa:2048 \
    -keyout "${RSA_DIR}/ca.key" \
    -out    "${RSA_DIR}/ca.crt" \
    -nodes -days "${DAYS}" \
    -subj "/CN=Classical RSA Root CA" \
    -config "${CONF}" -extensions v3_ca

${OPENSSL} req -new \
    -newkey rsa:2048 \
    -keyout "${RSA_DIR}/broker.key" \
    -out    "${RSA_DIR}/broker.csr" \
    -nodes \
    -subj "/CN=classical-broker-rsa" \
    -config "${CONF}"

${OPENSSL} x509 -req \
    -in "${RSA_DIR}/broker.csr" \
    -CA "${RSA_DIR}/ca.crt" -CAkey "${RSA_DIR}/ca.key" -CAcreateserial \
    -out "${RSA_DIR}/broker.crt" -days "${DAYS}" \
    -extfile "${CONF}" -extensions v3_broker

${OPENSSL} req -new \
    -newkey rsa:2048 \
    -keyout "${RSA_DIR}/client.key" \
    -out    "${RSA_DIR}/client.csr" \
    -nodes \
    -subj "/CN=classical-client-rsa" \
    -config "${CONF}"

${OPENSSL} x509 -req \
    -in "${RSA_DIR}/client.csr" \
    -CA "${RSA_DIR}/ca.crt" -CAkey "${RSA_DIR}/ca.key" -CAcreateserial \
    -out "${RSA_DIR}/client.crt" -days "${DAYS}" \
    -extfile "${CONF}" -extensions v3_client

echo "[pki-classical] ✓ RSA-2048 chain complete."

# ================================================================
# ECDSA P-256 Chain
# ================================================================
EC_DIR="${OUT}/ecdsa"
mkdir -p "${EC_DIR}"

echo "[pki-classical] --- ECDSA P-256 Chain ---"

${OPENSSL} req -x509 -new \
    -newkey ec -pkeyopt ec_paramgen_curve:P-256 \
    -keyout "${EC_DIR}/ca.key" \
    -out    "${EC_DIR}/ca.crt" \
    -nodes -days "${DAYS}" \
    -subj "/CN=Classical ECDSA Root CA" \
    -config "${CONF}" -extensions v3_ca

${OPENSSL} req -new \
    -newkey ec -pkeyopt ec_paramgen_curve:P-256 \
    -keyout "${EC_DIR}/broker.key" \
    -out    "${EC_DIR}/broker.csr" \
    -nodes \
    -subj "/CN=classical-broker-ecdsa" \
    -config "${CONF}"

${OPENSSL} x509 -req \
    -in "${EC_DIR}/broker.csr" \
    -CA "${EC_DIR}/ca.crt" -CAkey "${EC_DIR}/ca.key" -CAcreateserial \
    -out "${EC_DIR}/broker.crt" -days "${DAYS}" \
    -extfile "${CONF}" -extensions v3_broker

${OPENSSL} req -new \
    -newkey ec -pkeyopt ec_paramgen_curve:P-256 \
    -keyout "${EC_DIR}/client.key" \
    -out    "${EC_DIR}/client.csr" \
    -nodes \
    -subj "/CN=classical-client-ecdsa" \
    -config "${CONF}"

${OPENSSL} x509 -req \
    -in "${EC_DIR}/client.csr" \
    -CA "${EC_DIR}/ca.crt" -CAkey "${EC_DIR}/ca.key" -CAcreateserial \
    -out "${EC_DIR}/client.crt" -days "${DAYS}" \
    -extfile "${CONF}" -extensions v3_client

echo "[pki-classical] ✓ ECDSA P-256 chain complete."

# ---- Summary ----
echo ""
echo "--- RSA cert sizes ---"
ls -lh "${RSA_DIR}"/*.crt "${RSA_DIR}"/*.key
echo ""
echo "--- ECDSA cert sizes ---"
ls -lh "${EC_DIR}"/*.crt "${EC_DIR}"/*.key
echo ""
echo "[pki-classical] ✓ All classical baselines generated."
