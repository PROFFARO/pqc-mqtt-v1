#!/usr/bin/env bash
# ============================================================================
# build-openssl.sh — Build OpenSSL 3.4+ from source
# We build our own OpenSSL so Mosquitto and Paho link to a controlled version.
# Prerequisites: source toolchain/env.sh
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

OPENSSL_GIT_URL="https://github.com/openssl/openssl.git"
OPENSSL_GIT_TAG="openssl-3.4.1"

echo "============================================"
echo " PQC-MQTT: Building OpenSSL ${OPENSSL_GIT_TAG}"
echo "============================================"

# --- Clone ---
mkdir -p "${PQC_VENDOR_DIR}"
if [ ! -d "${OPENSSL_SRC_DIR}" ]; then
    echo "[openssl] Cloning from ${OPENSSL_GIT_URL} ..."
    git clone --depth 1 --branch "${OPENSSL_GIT_TAG}" \
        "${OPENSSL_GIT_URL}" "${OPENSSL_SRC_DIR}"
else
    echo "[openssl] Source directory already exists, skipping clone."
fi

# --- Configure ---
echo "[openssl] Configuring ..."
cd "${OPENSSL_SRC_DIR}"
./Configure \
    --prefix="${OPENSSL_INSTALL_DIR}" \
    --openssldir="${OPENSSL_INSTALL_DIR}/ssl" \
    shared \
    enable-tls1_3 \
    no-tests \
    -Wl,-rpath,"${OPENSSL_INSTALL_DIR}/lib64"

# --- Build ---
echo "[openssl] Compiling with ${NPROC} jobs ..."
make -j"${NPROC}"

# --- Install ---
echo "[openssl] Installing to ${OPENSSL_INSTALL_DIR} ..."
make install_sw install_ssldirs

# --- Verify ---
echo ""
echo "[openssl] Verifying installation ..."
"${OPENSSL_INSTALL_DIR}/bin/openssl" version
echo ""
echo "[openssl] ✓ Build complete."
echo "[openssl] Next: bash toolchain/build-oqs-provider.sh"
