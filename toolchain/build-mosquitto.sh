#!/usr/bin/env bash
# ============================================================================
# build-mosquitto.sh — Compile Eclipse Mosquitto from source
# Links against our PQC-enabled OpenSSL build.
# Prerequisites: OpenSSL already built
# ============================================================================

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

MOSQUITTO_GIT_URL="https://github.com/eclipse/mosquitto.git"
MOSQUITTO_GIT_TAG="v2.0.20"

echo "============================================"
echo " PQC-MQTT: Building Mosquitto ${MOSQUITTO_GIT_TAG}"
echo "============================================"

# --- Clone ---
mkdir -p "${PQC_VENDOR_DIR}"
if [ ! -d "${MOSQUITTO_SRC_DIR}" ]; then
    echo "[mosquitto] Cloning from ${MOSQUITTO_GIT_URL} ..."
    git clone --depth 1 --branch "${MOSQUITTO_GIT_TAG}" \
        "${MOSQUITTO_GIT_URL}" "${MOSQUITTO_SRC_DIR}"
else
    echo "[mosquitto] Source directory already exists, skipping clone."
fi

cd "${MOSQUITTO_SRC_DIR}"

# --- Patch config.mk to use our OpenSSL ---
echo "[mosquitto] Patching config.mk ..."
sed -i \
    -e 's|^WITH_TLS:=.*|WITH_TLS:=yes|' \
    -e 's|^WITH_TLS_PSK:=.*|WITH_TLS_PSK:=yes|' \
    -e 's|^WITH_CJSON:=.*|WITH_CJSON:=yes|' \
    config.mk

# --- Build with custom OpenSSL paths ---
echo "[mosquitto] Compiling with ${NPROC} jobs ..."
make -j"${NPROC}" \
    CFLAGS="-I${PQC_INSTALL_PREFIX}/include" \
    LDFLAGS="-L${PQC_INSTALL_PREFIX}/lib64 -L${PQC_INSTALL_PREFIX}/lib -Wl,-rpath,${PQC_INSTALL_PREFIX}/lib64"

echo ""
echo "[mosquitto] ✓ Build complete."
echo "[mosquitto] Binary at: ${MOSQUITTO_SRC_DIR}/src/mosquitto"
echo "[mosquitto] Next: bash toolchain/build-paho-mqtt.sh"
