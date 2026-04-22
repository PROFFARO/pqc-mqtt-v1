/* ============================================================================
 * throughput_bench.c — MQTT message throughput benchmark
 *
 * Measures messages/second over PQC-TLS vs classical TLS at QoS 0, 1, 2.
 *
 * Usage: ./throughput_bench --mode pqc|rsa|ecdsa [--count 1000] [--qos 1]
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

#define TP_CLIENT_ID  "bench-throughput"

typedef struct {
    tls_mode_t  mode;
    int         msg_count;
    int         qos;
    int         payload_size;
    const char *output_file;
    const char *ca_cert;
    const char *client_cert;
    const char *client_key;
} tp_config_t;

static int parse_args(int argc, char *argv[], tp_config_t *cfg)
{
    cfg->msg_count    = 1000;
    cfg->qos          = PQC_MQTT_QOS_1;
    cfg->payload_size = BENCH_DEFAULT_MSG_SIZE;
    cfg->output_file  = "eval/results/raw/throughput.csv";
    cfg->ca_cert      = NULL;
    cfg->client_cert  = NULL;
    cfg->client_key   = NULL;

    int mode_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s --mode <pqc|rsa|ecdsa> [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mode <mode>      TLS mode (pqc, rsa, ecdsa)\n");
            printf("  --count <N>        Number of messages\n");
            printf("  --qos <0|1|2>      QoS level\n");
            printf("  --payload-size <N> Payload size in bytes\n");
            printf("  --output <file>    CSV output file path\n");
            printf("  --ca <file>        CA certificate path\n");
            printf("  --cert <file>      Client certificate path\n");
            printf("  --key <file>       Client private key path\n");
            exit(0);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &cfg->mode) != 0) return -1;
            mode_set = 1;
        } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            cfg->msg_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--qos") == 0 && i + 1 < argc) {
            cfg->qos = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--payload-size") == 0 && i + 1 < argc) {
            cfg->payload_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            cfg->output_file = argv[++i];
        } else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc) {
            cfg->ca_cert = argv[++i];
        } else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) {
            cfg->client_cert = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            cfg->client_key = argv[++i];
        }
    }

    if (!mode_set) {
        fprintf(stderr, "Usage: %s --mode <pqc|rsa|ecdsa> [--count N] [--qos 0|1|2]\n", argv[0]);
        return -1;
    }

    /* Default cert paths */
    if (!cfg->ca_cert) {
        cfg->ca_cert = (cfg->mode == TLS_MODE_PQC) ? "pki/output/pqc/ca.crt"
            : (cfg->mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/ca.crt"
            : "pki/output/classical/ecdsa/ca.crt";
    }
    if (!cfg->client_cert) {
        cfg->client_cert = (cfg->mode == TLS_MODE_PQC) ? "pki/output/pqc/client.crt"
            : (cfg->mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.crt"
            : "pki/output/classical/ecdsa/client.crt";
    }
    if (!cfg->client_key) {
        cfg->client_key = (cfg->mode == TLS_MODE_PQC) ? "pki/output/pqc/client.key"
            : (cfg->mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.key"
            : "pki/output/classical/ecdsa/client.key";
    }

    return 0;
}

int main(int argc, char *argv[])
{
    tp_config_t cfg;
    if (parse_args(argc, argv, &cfg) != 0)
        return PQC_ERR_ARGS;

    const char *mode_str = (cfg.mode == TLS_MODE_PQC) ? "pqc"
                         : (cfg.mode == TLS_MODE_RSA) ? "rsa" : "ecdsa";
    const char *broker_uri = tls_config_get_broker_uri(cfg.mode);

    LOG_INFO("=== Throughput Benchmark ===");
    LOG_INFO("Mode:         %s", mode_str);
    LOG_INFO("Messages:     %d", cfg.msg_count);
    LOG_INFO("QoS:          %d", cfg.qos);
    LOG_INFO("Payload size: %d bytes", cfg.payload_size);

    /* --- Connect --- */
    MQTTClient client;
    MQTTClient_create(&client, broker_uri, TP_CLIENT_ID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    tls_cert_paths_t certs = { cfg.ca_cert, cfg.client_cert, cfg.client_key };
    tls_config_init(&ssl_opts, cfg.mode, &certs);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = PQC_MQTT_KEEPALIVE_SEC;
    conn_opts.cleansession      = 1;
    conn_opts.ssl               = &ssl_opts;

    int rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        LOG_ERROR("Connection failed: %d", rc);
        MQTTClient_destroy(&client);
        return PQC_ERR_CONNECT;
    }
    LOG_INFO("✓ Connected to %s", broker_uri);

    /* --- Prepare payload --- */
    char *payload = (char *)malloc(cfg.payload_size);
    memset(payload, 'A', cfg.payload_size);
    payload[cfg.payload_size - 1] = '\0';

    /* --- Timed publish burst --- */
    pqc_timer_t burst_timer;
    int success = 0;

    timer_start(&burst_timer);

    for (int i = 0; i < cfg.msg_count; i++) {
        MQTTClient_message msg = MQTTClient_message_initializer;
        msg.payload    = payload;
        msg.payloadlen = cfg.payload_size;
        msg.qos        = cfg.qos;
        msg.retained   = 0;

        MQTTClient_deliveryToken token;
        rc = MQTTClient_publishMessage(client, PQC_TOPIC_BENCHMARK, &msg, &token);
        if (rc == MQTTCLIENT_SUCCESS) {
            if (cfg.qos > 0)
                MQTTClient_waitForCompletion(client, token, PQC_MQTT_TIMEOUT_MS);
            success++;
        }
    }

    timer_stop(&burst_timer);

    double elapsed_sec = timer_elapsed_ms(&burst_timer) / 1000.0;
    double msgs_per_sec = (elapsed_sec > 0) ? success / elapsed_sec : 0;

    LOG_INFO("");
    LOG_INFO("=== Results (%s, QoS %d) ===", mode_str, cfg.qos);
    LOG_INFO("Sent:        %d / %d", success, cfg.msg_count);
    LOG_INFO("Duration:    %.3f sec", elapsed_sec);
    LOG_INFO("Throughput:  %.1f msgs/sec", msgs_per_sec);

    /* --- Log to CSV --- */
    csv_logger_t logger;
    if (csv_logger_open(&logger, cfg.output_file,
                        "mode,qos,payload_bytes,msg_count,duration_sec,msgs_per_sec") == 0) {
        csv_logger_write(&logger, "%s,%d,%d,%d,%.6f,%.2f",
                         mode_str, cfg.qos, cfg.payload_size,
                         success, elapsed_sec, msgs_per_sec);
        csv_logger_close(&logger);
    }

    /* --- Cleanup --- */
    free(payload);
    MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
    MQTTClient_destroy(&client);

    LOG_INFO("=== Throughput benchmark complete ===");
    return PQC_OK;
}
