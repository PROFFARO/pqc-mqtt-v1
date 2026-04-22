#!/usr/bin/env bash
# ============================================================================
# build-paho-mqtt.sh — Build the Eclipse Paho MQTT C client library
# Links against our PQC-enabled OpenSSL build.
# Prerequisites: OpenSSL already built
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

PAHO_GIT_URL="https://github.com/eclipse/paho.mqtt.c.git"
PAHO_GIT_TAG="v1.3.13"

echo "============================================"
echo " PQC-MQTT: Building Paho MQTT C ${PAHO_GIT_TAG}"
echo "============================================"

# --- Clone ---
mkdir -p "${PQC_VENDOR_DIR}"
if [ ! -d "${PAHO_MQTT_SRC_DIR}" ]; then
    echo "[paho] Cloning from ${PAHO_GIT_URL} ..."
    git clone --depth 1 --branch "${PAHO_GIT_TAG}" \
        "${PAHO_GIT_URL}" "${PAHO_MQTT_SRC_DIR}"
else
    echo "[paho] Source directory already exists, skipping clone."
fi

# --- Configure ---
echo "[paho] Configuring CMake build ..."
cmake -S "${PAHO_MQTT_SRC_DIR}" -B "${PAHO_MQTT_BUILD_DIR}" \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${PAHO_MQTT_INSTALL_DIR}" \
    -DPAHO_WITH_SSL=ON \
    -DPAHO_BUILD_SHARED=ON \
    -DPAHO_BUILD_STATIC=OFF \
    -DOPENSSL_ROOT_DIR="${PQC_INSTALL_PREFIX}" \
    -DCMAKE_PREFIX_PATH="${PQC_INSTALL_PREFIX}"

# --- Build ---
echo "[paho] Compiling with ${NPROC} jobs ..."
cmake --build "${PAHO_MQTT_BUILD_DIR}" --parallel "${NPROC}"

# --- Install ---
echo "[paho] Installing to ${PAHO_MQTT_INSTALL_DIR} ..."
cmake --install "${PAHO_MQTT_BUILD_DIR}"

echo ""
echo "[paho] ✓ Build complete."
echo "[paho] Installed to: ${PAHO_MQTT_INSTALL_DIR}"
echo "[paho] Next: bash pki/generate-pqc-certs.sh"
