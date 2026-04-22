#!/usr/bin/env bash
# ============================================================================
# env.sh — Shared environment variables for the PQC-MQTT toolchain
# Source this file before running any other toolchain script:
#   source toolchain/env.sh
# ============================================================================

set -euo pipefail

# --- Project root (auto-detected) ---
export PQC_PROJECT_ROOT
PQC_PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# --- Vendor directory (cloned dependencies live here) ---
export PQC_VENDOR_DIR="${PQC_PROJECT_ROOT}/toolchain/_vendor"

# --- Install prefix (all custom-built libs go here) ---
export PQC_INSTALL_PREFIX="${PQC_PROJECT_ROOT}/toolchain/_install"

# --- Individual component paths ---
export LIBOQS_SRC_DIR="${PQC_VENDOR_DIR}/liboqs"
export LIBOQS_BUILD_DIR="${LIBOQS_SRC_DIR}/build"
export LIBOQS_INSTALL_DIR="${PQC_INSTALL_PREFIX}"

export OPENSSL_SRC_DIR="${PQC_VENDOR_DIR}/openssl"
export OPENSSL_BUILD_DIR="${OPENSSL_SRC_DIR}"
export OPENSSL_INSTALL_DIR="${PQC_INSTALL_PREFIX}"

export OQS_PROVIDER_SRC_DIR="${PQC_VENDOR_DIR}/oqs-provider"
export OQS_PROVIDER_BUILD_DIR="${OQS_PROVIDER_SRC_DIR}/build"

export MOSQUITTO_SRC_DIR="${PQC_VENDOR_DIR}/mosquitto"
export MOSQUITTO_BUILD_DIR="${MOSQUITTO_SRC_DIR}"

export PAHO_MQTT_SRC_DIR="${PQC_VENDOR_DIR}/paho.mqtt.c"
export PAHO_MQTT_BUILD_DIR="${PAHO_MQTT_SRC_DIR}/build"
export PAHO_MQTT_INSTALL_DIR="${PQC_INSTALL_PREFIX}"

# --- Compiler / linker flags pointing to our custom OpenSSL ---
export OPENSSL_ROOT_DIR="${PQC_INSTALL_PREFIX}"
export PKG_CONFIG_PATH="${PQC_INSTALL_PREFIX}/lib64/pkgconfig:${PQC_INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="${PQC_INSTALL_PREFIX}/lib64:${PQC_INSTALL_PREFIX}/lib:${LD_LIBRARY_PATH:-}"
export PATH="${PQC_INSTALL_PREFIX}/bin:${PATH}"

# --- OpenSSL configuration file ---
export OPENSSL_CONF="${PQC_PROJECT_ROOT}/pki/openssl-pqc.cnf"

# --- Certificate output directories ---
export PQC_CERTS_DIR="${PQC_PROJECT_ROOT}/pki/output/pqc"
export CLASSICAL_CERTS_DIR="${PQC_PROJECT_ROOT}/pki/output/classical"

# --- Evaluation output ---
export EVAL_RAW_DIR="${PQC_PROJECT_ROOT}/eval/results/raw"
export EVAL_FIGURES_DIR="${PQC_PROJECT_ROOT}/eval/results/figures"
export EVAL_TABLES_DIR="${PQC_PROJECT_ROOT}/eval/results/tables"

# --- Parallel jobs ---
export NPROC
NPROC="$(nproc 2>/dev/null || echo 4)"

echo "[env] PQC_PROJECT_ROOT  = ${PQC_PROJECT_ROOT}"
echo "[env] PQC_INSTALL_PREFIX = ${PQC_INSTALL_PREFIX}"
echo "[env] NPROC              = ${NPROC}"
