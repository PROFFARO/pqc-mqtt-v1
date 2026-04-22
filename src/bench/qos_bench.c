/* ============================================================================
 * qos_bench.c — QoS-level comparison benchmark
 *
 * Measures throughput and per-message latency at QoS 0, 1, and 2
 * for each TLS mode to quantify the combined overhead of PQC + QoS.
 *
 * Usage: ./qos_bench --mode pqc|rsa|ecdsa [--count 500] [--output <file>]
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

#define QOS_CLIENT_ID "bench-qos"
#define DEFAULT_COUNT 500

int main(int argc, char *argv[])
{
    tls_mode_t mode = TLS_MODE_PQC;
    int msg_count = DEFAULT_COUNT;
    const char *output_file = "eval/results/raw/qos_comparison.csv";
    const char *ca = NULL, *cert = NULL, *key = NULL;
    int mode_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s --mode <pqc|rsa|ecdsa> [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mode <mode>      TLS mode (pqc, rsa, ecdsa)\n");
            printf("  --count <N>        Number of messages per QoS level\n");
            printf("  --output <file>    CSV output file path\n");
            printf("  --ca <file>        CA certificate path\n");
            printf("  --cert <file>      Client certificate path\n");
            printf("  --key <file>       Client private key path\n");
            exit(0);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &mode) != 0) return 1;
            mode_set = 1;
        } else if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            msg_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc) {
            ca = argv[++i];
        } else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) {
            cert = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            key = argv[++i];
        }
    }

    if (!mode_set) {
        fprintf(stderr, "Usage: %s --mode <pqc|rsa|ecdsa>\n", argv[0]);
        return 1;
    }

    const char *mode_str   = (mode == TLS_MODE_PQC) ? "pqc"
                           : (mode == TLS_MODE_RSA) ? "rsa" : "ecdsa";
    const char *broker_uri = tls_config_get_broker_uri(mode);

    /* Default cert paths */
    if (!ca) ca = (mode == TLS_MODE_PQC) ? "pki/output/pqc/ca.crt"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/ca.crt"
        : "pki/output/classical/ecdsa/ca.crt";
    if (!cert) cert = (mode == TLS_MODE_PQC) ? "pki/output/pqc/client.crt"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.crt"
        : "pki/output/classical/ecdsa/client.crt";
    if (!key) key = (mode == TLS_MODE_PQC) ? "pki/output/pqc/client.key"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.key"
        : "pki/output/classical/ecdsa/client.key";

    LOG_INFO("=== QoS Comparison Benchmark ===");
    LOG_INFO("Mode: %s | Count: %d per QoS level", mode_str, msg_count);

    /* Connect */
    MQTTClient client;
    MQTTClient_create(&client, broker_uri, QOS_CLIENT_ID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    tls_cert_paths_t certs = { ca, cert, key };
    tls_config_init(&ssl_opts, mode, &certs);

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

    csv_logger_t logger;
    csv_logger_open(&logger, output_file,
                    "mode,qos,msg_count,duration_sec,msgs_per_sec,avg_latency_ms");

    char payload[256];
    memset(payload, 'Q', sizeof(payload));

    for (int qos = 0; qos <= 2; qos++) {
        pqc_timer_t burst_timer;
        uint64_t total_msg_ns = 0;
        int success = 0;

        LOG_INFO("[QoS %d] Sending %d messages ...", qos, msg_count);

        timer_start(&burst_timer);
        for (int i = 0; i < msg_count; i++) {
            pqc_timer_t msg_timer;
            timer_start(&msg_timer);

            MQTTClient_message msg = MQTTClient_message_initializer;
            msg.payload    = payload;
            msg.payloadlen = sizeof(payload);
            msg.qos        = qos;
            msg.retained   = 0;

            MQTTClient_deliveryToken token;
            rc = MQTTClient_publishMessage(client, PQC_TOPIC_BENCHMARK, &msg, &token);
            if (rc == MQTTCLIENT_SUCCESS) {
                if (qos > 0)
                    MQTTClient_waitForCompletion(client, token, PQC_MQTT_TIMEOUT_MS);
                timer_stop(&msg_timer);
                total_msg_ns += timer_elapsed_ns(&msg_timer);
                success++;
            }
        }
        timer_stop(&burst_timer);

        double elapsed_sec   = timer_elapsed_ms(&burst_timer) / 1000.0;
        double msgs_sec      = (elapsed_sec > 0) ? success / elapsed_sec : 0;
        double avg_latency   = (success > 0) ? (double)total_msg_ns / success / 1e6 : 0;

        LOG_INFO("[QoS %d] %d msgs, %.1f msg/s, avg %.3f ms/msg",
                 qos, success, msgs_sec, avg_latency);

        csv_logger_write(&logger, "%s,%d,%d,%.6f,%.2f,%.6f",
                         mode_str, qos, success, elapsed_sec, msgs_sec, avg_latency);
    }

    csv_logger_close(&logger);
    MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
    MQTTClient_destroy(&client);

    LOG_INFO("=== QoS benchmark complete ===");
    return PQC_OK;
}
