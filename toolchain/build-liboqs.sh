#!/usr/bin/env bash
# ============================================================================
# build-liboqs.sh — Clone and build the Open Quantum Safe liboqs library
# Prerequisites: source toolchain/env.sh
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

LIBOQS_GIT_URL="https://github.com/open-quantum-safe/liboqs.git"
LIBOQS_GIT_TAG="0.12.0"   # Pin to a stable release

echo "============================================"
echo " PQC-MQTT: Building liboqs ${LIBOQS_GIT_TAG}"
echo "============================================"

# --- Clone ---
mkdir -p "${PQC_VENDOR_DIR}"
if [ ! -d "${LIBOQS_SRC_DIR}" ]; then
    echo "[liboqs] Cloning from ${LIBOQS_GIT_URL} ..."
    git clone --depth 1 --branch "${LIBOQS_GIT_TAG}" \
        "${LIBOQS_GIT_URL}" "${LIBOQS_SRC_DIR}"
else
    echo "[liboqs] Source directory already exists, skipping clone."
fi

# --- Configure ---
echo "[liboqs] Configuring CMake build ..."
cmake -S "${LIBOQS_SRC_DIR}" -B "${LIBOQS_BUILD_DIR}" \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${LIBOQS_INSTALL_DIR}" \
    -DBUILD_SHARED_LIBS=ON \
    -DOQS_BUILD_ONLY_LIB=ON \
    -DOQS_USE_OPENSSL=ON

# --- Build ---
echo "[liboqs] Compiling with ${NPROC} jobs ..."
cmake --build "${LIBOQS_BUILD_DIR}" --parallel "${NPROC}"

# --- Install ---
echo "[liboqs] Installing to ${LIBOQS_INSTALL_DIR} ..."
cmake --install "${LIBOQS_BUILD_DIR}"

echo ""
echo "[liboqs] ✓ Build complete."
echo "[liboqs] Installed to: ${LIBOQS_INSTALL_DIR}"
echo "[liboqs] Next: bash toolchain/build-openssl.sh"
