/* ============================================================================
 * pqc_mqtt_common.h — Shared constants and macros for PQC-MQTT project
 * ============================================================================ */

#ifndef PQC_MQTT_COMMON_H
#define PQC_MQTT_COMMON_H

/* --- Version --- */
#define PQC_MQTT_VERSION       "1.0.0"
#define PQC_MQTT_PROJECT_NAME  "PQC-MQTT"

/* --- Broker defaults --- */
#define PQC_BROKER_HOST            "localhost"
#define PQC_BROKER_PORT_PQC        8883
#define PQC_BROKER_PORT_RSA        8884
#define PQC_BROKER_PORT_ECDSA      8885
#define PQC_BROKER_URI_PQC         "ssl://localhost:8883"
#define PQC_BROKER_URI_RSA         "ssl://localhost:8884"
#define PQC_BROKER_URI_ECDSA       "ssl://localhost:8885"

/* --- MQTT defaults --- */
#define PQC_MQTT_QOS_0         0
#define PQC_MQTT_QOS_1         1
#define PQC_MQTT_QOS_2         2
#define PQC_MQTT_KEEPALIVE_SEC 60
#define PQC_MQTT_TIMEOUT_MS    10000

/* --- Topics --- */
#define PQC_TOPIC_SENSOR       "iot/sensor/data"
#define PQC_TOPIC_BENCHMARK    "iot/benchmark/ping"
#define PQC_TOPIC_STATUS       "iot/status"

/* --- Benchmark defaults --- */
#define BENCH_DEFAULT_ITERATIONS  100
#define BENCH_DEFAULT_MSG_SIZE    256
#define BENCH_WARMUP_ITERATIONS   5

/* --- Return codes --- */
#define PQC_OK                 0
#define PQC_ERR_TLS            1
#define PQC_ERR_CONNECT        2
#define PQC_ERR_PUBLISH        3
#define PQC_ERR_SUBSCRIBE      4
#define PQC_ERR_ARGS           5

/* --- Logging macros --- */
#include <stdio.h>
#include <time.h>

#define PQC_LOG(level, fmt, ...) \
    do { \
        time_t _t = time(NULL); \
        struct tm *_tm = localtime(&_t); \
        char _ts[20]; \
        strftime(_ts, sizeof(_ts), "%Y-%m-%d %H:%M:%S", _tm); \
        fprintf(stderr, "[%s] [%s] " fmt "\n", _ts, level, ##__VA_ARGS__); \
    } while (0)

#define LOG_INFO(fmt, ...)  PQC_LOG("INFO",  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  PQC_LOG("WARN",  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) PQC_LOG("ERROR", fmt, ##__VA_ARGS__)

#endif /* PQC_MQTT_COMMON_H */
