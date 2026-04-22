/* ============================================================================
 * payload_sweep_bench.c — Throughput across varying payload sizes
 *
 * Measures message throughput at different payload sizes (64B → 16KB)
 * to profile the overhead curve of PQC-TLS encrypted MQTT transport.
 *
 * Usage: ./payload_sweep_bench --mode pqc|rsa|ecdsa [--output <file>]
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

#define SWEEP_CLIENT_ID "bench-payload-sweep"

/* Payload sizes to test (bytes) */
static const int PAYLOAD_SIZES[] = {
    64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
};
#define NUM_SIZES (sizeof(PAYLOAD_SIZES) / sizeof(PAYLOAD_SIZES[0]))

#define MSGS_PER_SIZE 200

int main(int argc, char *argv[])
{
    tls_mode_t mode = TLS_MODE_PQC;
    const char *output_file = "eval/results/raw/payload_sweep.csv";
    const char *ca = NULL, *cert = NULL, *key = NULL;
    int mode_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s --mode <pqc|rsa|ecdsa> [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mode <mode>      TLS mode (pqc, rsa, ecdsa)\n");
            printf("  --output <file>    CSV output file path\n");
            printf("  --ca <file>        CA certificate path\n");
            printf("  --cert <file>      Client certificate path\n");
            printf("  --key <file>       Client private key path\n");
            exit(0);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &mode) != 0) return 1;
            mode_set = 1;
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

    LOG_INFO("=== Payload Sweep Benchmark ===");
    LOG_INFO("Mode: %s | Msgs/size: %d | Sizes: %zu", mode_str, MSGS_PER_SIZE, NUM_SIZES);

    /* Connect */
    MQTTClient client;
    MQTTClient_create(&client, broker_uri, SWEEP_CLIENT_ID,
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
    LOG_INFO("✓ Connected to %s", broker_uri);

    /* CSV logger */
    csv_logger_t logger;
    csv_logger_open(&logger, output_file,
                    "mode,payload_bytes,msg_count,duration_sec,msgs_per_sec,mbps");

    /* Sweep */
    for (size_t s = 0; s < NUM_SIZES; s++) {
        int psize = PAYLOAD_SIZES[s];
        char *payload = (char *)malloc(psize);
        memset(payload, 'X', psize);

        pqc_timer_t timer;
        int success = 0;

        timer_start(&timer);
        for (int i = 0; i < MSGS_PER_SIZE; i++) {
            MQTTClient_message msg = MQTTClient_message_initializer;
            msg.payload    = payload;
            msg.payloadlen = psize;
            msg.qos        = 1;
            msg.retained   = 0;

            MQTTClient_deliveryToken token;
            rc = MQTTClient_publishMessage(client, PQC_TOPIC_BENCHMARK, &msg, &token);
            if (rc == MQTTCLIENT_SUCCESS) {
                MQTTClient_waitForCompletion(client, token, PQC_MQTT_TIMEOUT_MS);
                success++;
            }
        }
        timer_stop(&timer);

        double elapsed_sec = timer_elapsed_ms(&timer) / 1000.0;
        double msgs_sec    = (elapsed_sec > 0) ? success / elapsed_sec : 0;
        double mbps        = (elapsed_sec > 0)
                           ? (double)success * psize * 8.0 / elapsed_sec / 1e6 : 0;

        LOG_INFO("[%5d B] %d msgs in %.3f s → %.1f msg/s, %.3f Mbps",
                 psize, success, elapsed_sec, msgs_sec, mbps);

        csv_logger_write(&logger, "%s,%d,%d,%.6f,%.2f,%.6f",
                         mode_str, psize, success, elapsed_sec, msgs_sec, mbps);

        free(payload);
    }

    csv_logger_close(&logger);
    MQTTClient_disconnect(client, PQC_MQTT_TIMEOUT_MS);
    MQTTClient_destroy(&client);

    LOG_INFO("=== Payload sweep complete ===");
    return PQC_OK;
}
