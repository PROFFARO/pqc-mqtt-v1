#!/usr/bin/env bash
# ============================================================================
# install-deps.sh — Install system-level build dependencies (Ubuntu/Debian)
# Usage: sudo bash toolchain/install-deps.sh
# ============================================================================

set -euo pipefail

echo "============================================"
echo " PQC-MQTT: Installing system dependencies"
echo "============================================"

apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gcc \
    g++ \
    git \
    pkg-config \
    libssl-dev \
    python3 \
    python3-pip \
    python3-venv \
    wget \
    curl \
    unzip \
    xsltproc \
    doxygen \
    graphviz \
    astyle \
    tshark \
    wireshark-common \
    sysstat \
    jq \
    cJSON \
    libcjson-dev \
    uuid-dev

echo ""
echo "[install-deps] ✓ All system dependencies installed."
echo "[install-deps] Next: source toolchain/env.sh && bash toolchain/build-liboqs.sh"
