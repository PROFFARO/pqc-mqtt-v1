/* ============================================================================
 * pqc_subscriber.c — MQTT subscriber with PQC-TLS support
 *
 * Connects to the Mosquitto broker over PQC-TLS, subscribes to a topic,
 * and logs received messages + handshake latency.
 *
 * Usage: ./pqc_subscriber --mode pqc|rsa|ecdsa [--topic <t>]
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "MQTTClient.h"
#include "pqc_mqtt_common.h"
#include "tls_config.h"
#include "timing_utils.h"
#include "csv_logger.h"

#define DEFAULT_CLIENT_ID  "pqc-subscriber-01"

static volatile int running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    LOG_INFO("Interrupt received, shutting down ...");
    running = 0;
}

/* --- Callback: message arrived --- */
static int on_message(void *context, char *topic, int topic_len,
                      MQTTClient_message *message)
{
    (void)context;
    (void)topic_len;

    char *payload = (char *)malloc(message->payloadlen + 1);
    if (payload) {
        memcpy(payload, message->payload, message->payloadlen);
        payload[message->payloadlen] = '\0';
        LOG_INFO("[RECV] Topic: %s | Payload: %s", topic, payload);
        free(payload);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic);
    return 1;
}

/* --- Callback: connection lost --- */
static void on_connection_lost(void *context, char *cause)
{
    (void)context;
    LOG_WARN("Connection lost: %s", cause ? cause : "unknown");
    running = 0;
}

/* --- Command-line options --- */
typedef struct {
    tls_mode_t    mode;
    const char   *topic;
    const char   *ca_cert;
    const char   *client_cert;
    const char   *client_key;
    const char   *log_file;
} sub_config_t;

static int parse_args(int argc, char *argv[], sub_config_t *cfg)
{
    cfg->topic       = PQC_TOPIC_SENSOR;
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
        } else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc) {
            cfg->ca_cert = argv[++i];
        } else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) {
            cfg->client_cert = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            cfg->client_key = argv[++i];
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            cfg->log_file = argv[++i];
        }
    }

    if (!mode_set) {
        LOG_ERROR("--mode is required. Use: pqc, rsa, ecdsa");
        return -1;
    }

    /* Default cert paths */
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
    sub_config_t cfg;
    if (parse_args(argc, argv, &cfg) != 0)
        return PQC_ERR_ARGS;

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    const char *broker_uri = tls_config_get_broker_uri(cfg.mode);

    LOG_INFO("=== PQC-MQTT Subscriber ===");
    LOG_INFO("Broker:  %s", broker_uri);
    LOG_INFO("Topic:   %s", cfg.topic);

    /* --- Create MQTT client --- */
    MQTTClient client;
    int rc = MQTTClient_create(&client, broker_uri, DEFAULT_CLIENT_ID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("MQTTClient_create failed: %d", rc);
        return PQC_ERR_CONNECT;
    }

    /* Set callbacks */
    MQTTClient_setCallbacks(client, NULL, on_connection_lost, on_message, NULL);

    /* --- Configure TLS --- */
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    tls_cert_paths_t certs = {
        .ca_cert     = cfg.ca_cert,
        .client_cert = cfg.client_cert,
        .client_key  = cfg.client_key
    };

    if (tls_config_init(&ssl_opts, cfg.mode, &certs) != 0) {
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

    /* --- Subscribe --- */
    rc = MQTTClient_subscribe(client, cfg.topic, PQC_MQTT_QOS_1);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Subscribe failed: %d", rc);
        MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
        MQTTClient_destroy(&client);
        return PQC_ERR_SUBSCRIBE;
    }

    LOG_INFO("✓ Subscribed to: %s", cfg.topic);
    LOG_INFO("Waiting for messages ... (Ctrl+C to stop)");

    /* --- Message loop --- */
    while (running) {
        usleep(100000); /* 100ms sleep to avoid busy-wait */
    }

    /* --- Cleanup --- */
    MQTTClient_unsubscribe(client, cfg.topic);
    MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
    MQTTClient_destroy(&client);

    LOG_INFO("=== Subscriber finished ===");
    return PQC_OK;
}
