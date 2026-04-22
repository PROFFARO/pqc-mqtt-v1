# System Architecture

## High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        PQC-MQTT System                              │
│                                                                     │
│  ┌──────────────┐    TLS 1.3 (ML-KEM-768)    ┌──────────────────┐  │
│  │  PQC Publisher│◄──────────────────────────►│                  │  │
│  │  (Paho C)    │   ML-DSA-65 Certificates   │   PQC Broker     │  │
│  └──────────────┘                             │   (Mosquitto)    │  │
│                                               │                  │  │
│  ┌──────────────┐    TLS 1.3 (ML-KEM-768)    │   Linked to:     │  │
│  │PQC Subscriber│◄──────────────────────────►│   - OpenSSL 3.4  │  │
│  │  (Paho C)    │   ML-DSA-65 Certificates   │   - oqs-provider │  │
│  └──────────────┘                             │   - liboqs       │  │
│                                               └──────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │                    Evaluation Engine                          │   │
│  │  • tshark packet capture    • Python statistical analysis    │   │
│  │  • Handshake latency bench  • Publication chart generation   │   │
│  │  • Throughput bench         • PQC vs Classical comparison    │   │
│  │  • Stress test (50 clients) • CSV + Markdown reporting       │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

## Cryptographic Stack

```
Application Layer     ┌─────────────────────────────────────┐
                      │  pqc_publisher / pqc_subscriber     │
                      │  (Paho MQTT C Client Library)        │
                      └──────────────┬──────────────────────┘
                                     │
TLS Layer             ┌──────────────▼──────────────────────┐
                      │        OpenSSL 3.4+ (TLS 1.3)       │
                      │                                      │
                      │  ┌────────────┐  ┌────────────────┐ │
                      │  │  default   │  │  oqs-provider  │ │
                      │  │  provider  │  │  (PQC bridge)  │ │
                      │  └────────────┘  └───────┬────────┘ │
                      └──────────────────────────┼──────────┘
                                                 │
Crypto Layer          ┌──────────────────────────▼──────────┐
                      │            liboqs 0.12.0             │
                      │                                      │
                      │  ┌──────────┐     ┌──────────────┐  │
                      │  │ ML-KEM   │     │   ML-DSA     │  │
                      │  │ (Kyber)  │     │ (Dilithium)  │  │
                      │  │ 768/1024 │     │  65 / 87     │  │
                      │  └──────────┘     └──────────────┘  │
                      └─────────────────────────────────────┘
```

## TLS 1.3 Handshake Flow (PQC)

```
Client (Publisher/Subscriber)              Broker (Mosquitto)
  │                                            │
  │──── ClientHello ──────────────────────────►│
  │     + supported_groups: mlkem768           │
  │     + key_share: ML-KEM-768 encaps key     │
  │                                            │
  │◄─── ServerHello ──────────────────────────│
  │     + key_share: ML-KEM-768 ciphertext     │
  │                                            │
  │◄─── EncryptedExtensions ──────────────────│
  │◄─── Certificate (ML-DSA-65 signed) ──────│
  │◄─── CertificateVerify (ML-DSA-65) ───────│
  │◄─── Finished ─────────────────────────────│
  │                                            │
  │──── Certificate (ML-DSA-65 signed) ──────►│
  │──── CertificateVerify (ML-DSA-65) ───────►│
  │──── Finished ─────────────────────────────►│
  │                                            │
  │◄═══ Application Data (MQTT) ═════════════►│
  │     (AES-256-GCM encrypted)                │
```

## Module Dependency Graph

```
toolchain/env.sh
    │
    ├── toolchain/build-liboqs.sh ──────────────────────────────► liboqs.so
    │
    ├── toolchain/build-openssl.sh ─────────────────────────────► openssl (binary + libs)
    │
    ├── toolchain/build-oqs-provider.sh ── (needs liboqs + openssl) ─► oqsprovider.so
    │
    ├── toolchain/build-mosquitto.sh ───── (needs openssl) ────► mosquitto (binary)
    │
    └── toolchain/build-paho-mqtt.sh ───── (needs openssl) ────► libpaho-mqtt3cs.so
                                                                       │
                                                                       ▼
                                                              CMakeLists.txt
                                                                       │
                                                        ┌──────────────┼──────────────┐
                                                        ▼              ▼              ▼
                                                  pqc_publisher  pqc_subscriber  benchmarks
```
