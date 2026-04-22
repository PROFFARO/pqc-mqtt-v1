#!/usr/bin/env bash
# ============================================================================
# build-oqs-provider.sh — Build the OQS provider plugin for OpenSSL 3.x
# This bridges liboqs algorithms into OpenSSL's EVP interface.
# Prerequisites: liboqs + OpenSSL already built
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

OQS_PROV_GIT_URL="https://github.com/open-quantum-safe/oqs-provider.git"
OQS_PROV_GIT_TAG="0.8.0"

echo "============================================"
echo " PQC-MQTT: Building oqs-provider ${OQS_PROV_GIT_TAG}"
echo "============================================"

# --- Preflight checks ---
if [ ! -f "${PQC_INSTALL_PREFIX}/bin/openssl" ]; then
    echo "[oqs-provider] ERROR: Custom OpenSSL not found. Run build-openssl.sh first."
    exit 1
fi
if [ ! -f "${PQC_INSTALL_PREFIX}/lib/liboqs.so" ] && \
   [ ! -f "${PQC_INSTALL_PREFIX}/lib64/liboqs.so" ]; then
    echo "[oqs-provider] ERROR: liboqs not found. Run build-liboqs.sh first."
    exit 1
fi

# --- Clone ---
mkdir -p "${PQC_VENDOR_DIR}"
if [ ! -d "${OQS_PROVIDER_SRC_DIR}" ]; then
    echo "[oqs-provider] Cloning from ${OQS_PROV_GIT_URL} ..."
    git clone --depth 1 --branch "${OQS_PROV_GIT_TAG}" \
        "${OQS_PROV_GIT_URL}" "${OQS_PROVIDER_SRC_DIR}"
else
    echo "[oqs-provider] Source directory already exists, skipping clone."
fi

# --- Configure ---
echo "[oqs-provider] Configuring CMake build ..."
cmake -S "${OQS_PROVIDER_SRC_DIR}" -B "${OQS_PROVIDER_BUILD_DIR}" \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${PQC_INSTALL_PREFIX}" \
    -Dliboqs_DIR="${PQC_INSTALL_PREFIX}/lib/cmake/liboqs" \
    -DOPENSSL_ROOT_DIR="${PQC_INSTALL_PREFIX}" \
    -DCMAKE_INSTALL_PREFIX="${PQC_INSTALL_PREFIX}"

# --- Build ---
echo "[oqs-provider] Compiling ..."
cmake --build "${OQS_PROVIDER_BUILD_DIR}" --parallel "${NPROC}"

# --- Install the provider .so into OpenSSL's oqsprovider directory ---
OQS_PROVIDER_LIB="${OQS_PROVIDER_BUILD_DIR}/lib/oqsprovider.so"
OSSL_MODULES_DIR="${PQC_INSTALL_PREFIX}/lib64/ossl-modules"
mkdir -p "${OSSL_MODULES_DIR}"
cp "${OQS_PROVIDER_LIB}" "${OSSL_MODULES_DIR}/oqsprovider.so"

echo ""
echo "[oqs-provider] ✓ Build complete."
echo "[oqs-provider] Provider installed to: ${OSSL_MODULES_DIR}/oqsprovider.so"
echo ""

# --- Verify ---
echo "[oqs-provider] Verifying provider loads ..."
OPENSSL_CONF="${PQC_PROJECT_ROOT}/pki/openssl-pqc.cnf" \
    "${PQC_INSTALL_PREFIX}/bin/openssl" list -providers 2>/dev/null || \
    echo "[oqs-provider] (Run after creating pki/openssl-pqc.cnf to verify)"

echo ""
echo "[oqs-provider] Next: bash toolchain/build-mosquitto.sh"
