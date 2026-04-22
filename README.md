# PQC-MQTT v1

**Integration & Evaluation of Post-Quantum Cryptographic Algorithms in IoT-Enabled MQTT Protocol**

A research-oriented system that integrates NIST-standardized post-quantum cryptographic algorithms (**ML-KEM-768**, **ML-DSA-65/87**) into MQTT broker/client communication over TLS 1.3, with a complete benchmarking and evaluation engine.

---

## Architecture

```
Publisher ──► [ TLS 1.3 + ML-KEM-768 + ML-DSA-65 ] ──► Mosquitto Broker
                                                              │
Subscriber ◄─ [ TLS 1.3 + ML-KEM-768 + ML-DSA-65 ] ◄────────┘
```

See [docs/architecture.md](docs/architecture.md) for detailed diagrams.

---

## Tech Stack

| Component | Technology | Role |
|-----------|-----------|------|
| PQC Primitives | [liboqs](https://github.com/open-quantum-safe/liboqs) | ML-KEM & ML-DSA implementations |
| TLS Layer | OpenSSL 3.4+ | TLS 1.3 engine |
| PQC Bridge | [oqs-provider](https://github.com/open-quantum-safe/oqs-provider) | Plugs PQC into OpenSSL |
| MQTT Broker | [Eclipse Mosquitto](https://github.com/eclipse/mosquitto) | Pub/Sub broker (C) |
| MQTT Client | [Paho MQTT C](https://github.com/eclipse/paho.mqtt.c) | Publisher & Subscriber |
| Analysis | Python + matplotlib + seaborn | Charts & statistics |

---

## Project Structure

```
pqc-mqtt-v1/
├── toolchain/          # Build scripts for all dependencies
│   ├── env.sh          # Shared environment variables (source first!)
│   ├── install-deps.sh # System packages
│   ├── build-liboqs.sh
│   ├── build-openssl.sh
│   ├── build-oqs-provider.sh
│   ├── build-mosquitto.sh
│   └── build-paho-mqtt.sh
├── pki/                # Certificate generation
│   ├── openssl-pqc.cnf
│   ├── openssl-classical.cnf
│   ├── generate-pqc-certs.sh
│   └── generate-classical-certs.sh
├── broker/             # Mosquitto configuration
│   ├── mosquitto-pqc.conf
│   ├── mosquitto-classical.conf
│   ├── start-broker.sh
│   └── stop-broker.sh
├── src/                # C source code
│   ├── include/        # Headers
│   ├── lib/            # Shared utilities (tls_config, timing, csv_logger)
│   ├── publisher/      # pqc_publisher.c
│   ├── subscriber/     # pqc_subscriber.c
│   └── bench/          # handshake_bench, throughput_bench, stress_test
├── eval/               # Evaluation engine
│   ├── capture/        # tshark scripts
│   └── analysis/       # Python analysis + plot generation
├── docs/               # Documentation
│   ├── architecture.md
│   └── references.bib  # 15 research paper citations
├── CMakeLists.txt
└── Makefile            # Convenience targets
```

---

## Quick Start (Ubuntu 22.04 / WSL2)

### 1. Install system dependencies
```bash
sudo bash toolchain/install-deps.sh
```

### 2. Build the full toolchain
```bash
make toolchain
# Builds: liboqs → OpenSSL 3.4 → oqs-provider → Mosquitto → Paho MQTT C
```

### 3. Generate certificates
```bash
make certs
# Creates ML-DSA + RSA + ECDSA certificate chains in pki/output/
```

### 4. Build the project
```bash
make build
# Compiles: pqc_publisher, pqc_subscriber, handshake_bench, throughput_bench, stress_test
```

### 5. Start the broker
```bash
make broker-pqc
# Starts Mosquitto on port 8883 with PQC-TLS
```

### 6. Run publisher & subscriber (in separate terminals)
```bash
./build/pqc_subscriber --mode pqc
./build/pqc_publisher  --mode pqc --count 10
```

### 7. Run benchmarks
```bash
make bench
# Runs handshake + throughput benchmarks for pqc, rsa, ecdsa
```

### 8. Generate analysis charts
```bash
pip install -r eval/analysis/requirements.txt
python3 eval/analysis/compute_statistics.py
python3 eval/analysis/generate_plots.py
```

---

## Research Papers

See [docs/references.bib](docs/references.bib) for BibTeX entries. Key references:

1. NIST FIPS 203 — ML-KEM Standard (2024)
2. NIST FIPS 204 — ML-DSA Standard (2024)
3. NIST FIPS 205 — SLH-DSA Standard (2024)
4. CRYSTALS-Kyber KEM — Bos et al. (2018)
5. CRYSTALS-Dilithium DSA — Ducas et al. (2018)
6. PQC for IoT Survey — arXiv (2024)
7. Constrained Device PQC Benchmarking — MDPI (2024)
8. PQC in Lightweight IoT Protocols — MDPI (2025)
9. Post-Quantum TLS 1.3 Performance — CoNEXT (2023)
10. Measuring TLS with PQ KEM — Kwiatkowski et al. (2019)
11. PQ-TLS without Handshake Signatures — Schwabe et al. (2020)
12. MQTT Security Study — IEEE Access (2023)
13. IoT TLS 1.3 Performance — Restuccia et al. (2020)
14. NIST PQC Round 3 Status Report (2022)
15. PQC Transition Review — arXiv (2024)

---

## License

See [LICENSE](LICENSE) for details.
