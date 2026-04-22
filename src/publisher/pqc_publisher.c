/* ============================================================================
 * pqc_publisher.c — MQTT publisher with PQC-TLS support
 *
 * Connects to the Mosquitto broker over ML-DSA/ML-KEM-secured TLS 1.3,
 * publishes simulated IoT sensor data, and logs handshake latency.
 *
 * Usage: ./pqc_publisher --mode pqc|rsa|ecdsa [--topic <t>] [--count <n>]
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MQTTClient.h"
#include "pqc_mqtt_common.h"
#include "tls_config.h"
#include "timing_utils.h"
#include "csv_logger.h"

/* --- Default configuration --- */
#define DEFAULT_CLIENT_ID  "pqc-publisher-01"
#define DEFAULT_MSG_COUNT  10
#define DEFAULT_INTERVAL_MS 1000

/* --- Command-line options --- */
typedef struct {
    tls_mode_t    mode;
    const char   *topic;
    int           msg_count;
    int           interval_ms;
    const char   *ca_cert;
    const char   *client_cert;
    const char   *client_key;
    const char   *log_file;
} pub_config_t;

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s --mode <pqc|rsa|ecdsa> [options]\n"
        "\n"
        "Options:\n"
        "  --mode <mode>       TLS mode: pqc, rsa, ecdsa (required)\n"
        "  --topic <topic>     MQTT topic (default: %s)\n"
        "  --count <n>         Number of messages to publish (default: %d)\n"
        "  --interval <ms>     Delay between messages in ms (default: %d)\n"
        "  --ca <path>         CA certificate file\n"
        "  --cert <path>       Client certificate file\n"
        "  --key <path>        Client private key file\n"
        "  --log <path>        CSV log file for timing data\n"
        "\n", prog, PQC_TOPIC_SENSOR, DEFAULT_MSG_COUNT, DEFAULT_INTERVAL_MS);
}

static int parse_args(int argc, char *argv[], pub_config_t *cfg)
{
    /* Defaults */
    cfg->topic       = PQC_TOPIC_SENSOR;
    cfg->msg_count   = DEFAULT_MSG_COUNT;
    cfg->interval_ms = DEFAULT_INTERVAL_MS;
    cfg->ca_cert     = NULL;
    cfg->client_cert = NULL;
    cfg->client_key  = NULL;
    cfg->log_file    = NULL;

    int mode_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &cfg->mode) != 0) return -1;
            mode_set = 1;
        } else if (strcmp(argv[i], "--topic") == 0 && i + 1 < argc) {
            cfg->topic = argv[++i];
        } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            cfg->msg_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            cfg->interval_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc) {
            cfg->ca_cert = argv[++i];
        } else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) {
            cfg->client_cert = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            cfg->client_key = argv[++i];
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            cfg->log_file = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
    }

    if (!mode_set) {
        LOG_ERROR("--mode is required");
        print_usage(argv[0]);
        return -1;
    }

    /* Set default cert paths based on mode if not overridden */
    if (!cfg->ca_cert) {
        cfg->ca_cert = (cfg->mode == TLS_MODE_PQC)
            ? "pki/output/pqc/ca.crt"
            : (cfg->mode == TLS_MODE_RSA)
                ? "pki/output/classical/rsa/ca.crt"
                : "pki/output/classical/ecdsa/ca.crt";
    }
    if (!cfg->client_cert) {
        cfg->client_cert = (cfg->mode == TLS_MODE_PQC)
            ? "pki/output/pqc/client.crt"
            : (cfg->mode == TLS_MODE_RSA)
                ? "pki/output/classical/rsa/client.crt"
                : "pki/output/classical/ecdsa/client.crt";
    }
    if (!cfg->client_key) {
        cfg->client_key = (cfg->mode == TLS_MODE_PQC)
            ? "pki/output/pqc/client.key"
            : (cfg->mode == TLS_MODE_RSA)
                ? "pki/output/classical/rsa/client.key"
                : "pki/output/classical/ecdsa/client.key";
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pub_config_t cfg;
    if (parse_args(argc, argv, &cfg) != 0)
        return PQC_ERR_ARGS;

    const char *broker_uri = tls_config_get_broker_uri(cfg.mode);

    LOG_INFO("=== PQC-MQTT Publisher ===");
    LOG_INFO("Broker:  %s", broker_uri);
    LOG_INFO("Topic:   %s", cfg.topic);
    LOG_INFO("Count:   %d messages", cfg.msg_count);

    /* --- Create MQTT client --- */
    MQTTClient client;
    int rc = MQTTClient_create(&client, broker_uri, DEFAULT_CLIENT_ID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("MQTTClient_create failed: %d", rc);
        return PQC_ERR_CONNECT;
    }

    /* --- Configure TLS --- */
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    tls_cert_paths_t certs = {
        .ca_cert     = cfg.ca_cert,
        .client_cert = cfg.client_cert,
        .client_key  = cfg.client_key
    };

    if (tls_config_init(&ssl_opts, cfg.mode, &certs) != 0) {
        LOG_ERROR("TLS configuration failed");
        MQTTClient_destroy(&client);
        return PQC_ERR_TLS;
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = PQC_MQTT_KEEPALIVE_SEC;
    conn_opts.cleansession      = 1;
    conn_opts.ssl               = &ssl_opts;

    /* --- Connect (with timing) --- */
    pqc_timer_t handshake_timer;

    LOG_INFO("Connecting to broker ...");
    timer_start(&handshake_timer);
    rc = MQTTClient_connect(client, &conn_opts);
    timer_stop(&handshake_timer);

    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Connection failed: %d", rc);
        MQTTClient_destroy(&client);
        return PQC_ERR_CONNECT;
    }

    double handshake_ms = timer_elapsed_ms(&handshake_timer);
    LOG_INFO("✓ Connected! Handshake latency: %.3f ms", handshake_ms);

    /* --- Optional: Log handshake to CSV --- */
    csv_logger_t logger;
    if (cfg.log_file) {
        csv_logger_open(&logger, cfg.log_file,
                        "timestamp,mode,handshake_ms");
        csv_logger_write(&logger, "%ld,%s,%.6f",
                         (long)time(NULL),
                         (cfg.mode == TLS_MODE_PQC) ? "pqc" :
                         (cfg.mode == TLS_MODE_RSA) ? "rsa" : "ecdsa",
                         handshake_ms);
    }

    /* --- Publish messages --- */
    for (int i = 0; i < cfg.msg_count; i++) {
        char payload[512];
        snprintf(payload, sizeof(payload),
                 "{\"sensor_id\":\"pub-01\",\"seq\":%d,"
                 "\"temperature\":%.1f,\"humidity\":%.1f,"
                 "\"timestamp\":%ld}",
                 i,
                 20.0 + (rand() % 150) / 10.0,   /* 20.0 - 35.0 °C */
                 40.0 + (rand() % 400) / 10.0,    /* 40.0 - 80.0 %  */
                 (long)time(NULL));

        pqc_timer_t pub_timer;
        timer_start(&pub_timer);

        MQTTClient_message msg = MQTTClient_message_initializer;
        msg.payload    = payload;
        msg.payloadlen = (int)strlen(payload);
        msg.qos        = PQC_MQTT_QOS_1;
        msg.retained   = 0;

        MQTTClient_deliveryToken token;
        rc = MQTTClient_publishMessage(client, cfg.topic, &msg, &token);

        if (rc == MQTTCLIENT_SUCCESS) {
            MQTTClient_waitForCompletion(client, token, PQC_MQTT_TIMEOUT_MS);
            timer_stop(&pub_timer);
            LOG_INFO("[%d/%d] Published (%.3f ms): %s",
                     i + 1, cfg.msg_count, timer_elapsed_ms(&pub_timer), payload);
        } else {
            LOG_ERROR("[%d/%d] Publish failed: %d", i + 1, cfg.msg_count, rc);
        }

        if (cfg.interval_ms > 0)
            usleep(cfg.interval_ms * 1000);
    }

    /* --- Cleanup --- */
    if (cfg.log_file)
        csv_logger_close(&logger);

    MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
    MQTTClient_destroy(&client);

    LOG_INFO("=== Publisher finished ===");
    return PQC_OK;
}
