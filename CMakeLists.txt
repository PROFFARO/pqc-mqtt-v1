# ============================================================================
# CMakeLists.txt — Top-level CMake build for PQC-MQTT project
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(pqc-mqtt
    VERSION   1.0.0
    LANGUAGES C
    DESCRIPTION "Post-Quantum Cryptographic MQTT Integration & Evaluation"
)

# --- Build options ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Look for our custom-built dependencies first
if(DEFINED ENV{PQC_INSTALL_PREFIX})
    list(PREPEND CMAKE_PREFIX_PATH $ENV{PQC_INSTALL_PREFIX})
    set(OPENSSL_ROOT_DIR $ENV{PQC_INSTALL_PREFIX})
endif()

# --- Find dependencies ---
find_package(OpenSSL 3.0 REQUIRED)
find_package(Threads REQUIRED)

# Find Paho MQTT C (installed by build-paho-mqtt.sh)
find_library(PAHO_MQTT_LIB
    NAMES paho-mqtt3cs paho-mqtt3c
    HINTS $ENV{PQC_INSTALL_PREFIX}/lib $ENV{PQC_INSTALL_PREFIX}/lib64
)
find_path(PAHO_MQTT_INCLUDE
    NAMES MQTTClient.h
    HINTS $ENV{PQC_INSTALL_PREFIX}/include
)

if(NOT PAHO_MQTT_LIB OR NOT PAHO_MQTT_INCLUDE)
    message(FATAL_ERROR
        "Paho MQTT C library not found. Run: bash toolchain/build-paho-mqtt.sh")
endif()

message(STATUS "OpenSSL:    ${OPENSSL_VERSION} at ${OPENSSL_INCLUDE_DIR}")
message(STATUS "Paho MQTT:  ${PAHO_MQTT_LIB}")
message(STATUS "Paho inc:   ${PAHO_MQTT_INCLUDE}")

# --- Shared include directory ---
set(PQC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include)

# --- Internal utility library (static) ---
add_library(pqc_utils STATIC
    src/lib/tls_config.c
    src/lib/timing_utils.c
    src/lib/csv_logger.c
)
target_include_directories(pqc_utils PUBLIC
    ${PQC_INCLUDE_DIR}
    ${PAHO_MQTT_INCLUDE}
    ${OPENSSL_INCLUDE_DIR}
)
target_link_libraries(pqc_utils PUBLIC
    ${PAHO_MQTT_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# --- Publisher ---
add_executable(pqc_publisher
    src/publisher/pqc_publisher.c
)
target_link_libraries(pqc_publisher PRIVATE pqc_utils)

# --- Subscriber ---
add_executable(pqc_subscriber
    src/subscriber/pqc_subscriber.c
)
target_link_libraries(pqc_subscriber PRIVATE pqc_utils)

# --- Handshake Benchmark ---
add_executable(handshake_bench
    src/bench/handshake_bench.c
)
target_link_libraries(handshake_bench PRIVATE pqc_utils)

# --- Throughput Benchmark ---
add_executable(throughput_bench
    src/bench/throughput_bench.c
)
target_link_libraries(throughput_bench PRIVATE pqc_utils)

# --- Stress Test ---
add_executable(stress_test
    src/bench/stress_test.c
)
target_link_libraries(stress_test PRIVATE pqc_utils Threads::Threads)

# --- Install targets ---
install(TARGETS
    pqc_publisher pqc_subscriber
    handshake_bench throughput_bench stress_test
    RUNTIME DESTINATION bin
)
