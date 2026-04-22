/* ============================================================================
 * stress_test.c — Concurrent client stress test
 *
 * Spawns N threads, each performing a TLS handshake simultaneously.
 * Measures broker CPU/memory impact under concurrent PQC handshake load.
 *
 * Usage: ./stress_test --mode pqc|rsa|ecdsa [--clients 50]
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "MQTTClient.h"
#include "pqc_mqtt_common.h"
#include "tls_config.h"
#include "timing_utils.h"
#include "csv_logger.h"

/* --- Per-thread context --- */
typedef struct {
    int          thread_id;
    tls_mode_t   mode;
    const char  *broker_uri;
    const char  *ca_cert;
    const char  *client_cert;
    const char  *client_key;
    uint64_t     latency_ns;     /* output */
    int          success;        /* output */
} thread_ctx_t;

static void *client_thread(void *arg)
{
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    ctx->success    = 0;
    ctx->latency_ns = 0;

    char client_id[64];
    snprintf(client_id, sizeof(client_id), "stress-%d", ctx->thread_id);

    MQTTClient client;
    int rc = MQTTClient_create(&client, ctx->broker_uri, client_id,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) return NULL;

    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
    tls_cert_paths_t certs = {
        ctx->ca_cert, ctx->client_cert, ctx->client_key
    };
    tls_config_init(&ssl_opts, ctx->mode, &certs);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = PQC_MQTT_KEEPALIVE_SEC;
    conn_opts.cleansession      = 1;
    conn_opts.ssl               = &ssl_opts;

    pqc_timer_t timer;
    timer_start(&timer);
    rc = MQTTClient_connect(client, &conn_opts);
    timer_stop(&timer);

    if (rc == MQTTCLIENT_SUCCESS) {
        ctx->latency_ns = timer_elapsed_ns(&timer);
        ctx->success    = 1;
        MQTTClient_disconnect(client, 1000);
    }

    MQTTClient_destroy(&client);
    return NULL;
}

int main(int argc, char *argv[])
{
    tls_mode_t mode = TLS_MODE_PQC;
    int num_clients  = 50;
    const char *output_file = "eval/results/raw/stress_test.csv";
    const char *ca_cert = NULL, *client_cert = NULL, *client_key = NULL;

    int mode_set = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s --mode <pqc|rsa|ecdsa> [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --mode <mode>      TLS mode (pqc, rsa, ecdsa)\n");
            printf("  --clients <N>      Number of concurrent clients\n");
            printf("  --output <file>    CSV output file path\n");
            printf("  --ca <file>        CA certificate path\n");
            printf("  --cert <file>      Client certificate path\n");
            printf("  --key <file>       Client private key path\n");
            exit(0);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &mode) != 0) return 1;
            mode_set = 1;
        } else if (strcmp(argv[i], "--clients") == 0 && i + 1 < argc) {
            num_clients = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--ca") == 0 && i + 1 < argc) {
            ca_cert = argv[++i];
        } else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) {
            client_cert = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
            client_key = argv[++i];
        }
    }

    if (!mode_set) {
        fprintf(stderr, "Usage: %s --mode <pqc|rsa|ecdsa> [--clients N]\n", argv[0]);
        return 1;
    }

    const char *mode_str   = (mode == TLS_MODE_PQC) ? "pqc"
                           : (mode == TLS_MODE_RSA) ? "rsa" : "ecdsa";
    const char *broker_uri = tls_config_get_broker_uri(mode);

    /* Default cert paths */
    if (!ca_cert) ca_cert = (mode == TLS_MODE_PQC) ? "pki/output/pqc/ca.crt"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/ca.crt"
        : "pki/output/classical/ecdsa/ca.crt";
    if (!client_cert) client_cert = (mode == TLS_MODE_PQC) ? "pki/output/pqc/client.crt"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.crt"
        : "pki/output/classical/ecdsa/client.crt";
    if (!client_key) client_key = (mode == TLS_MODE_PQC) ? "pki/output/pqc/client.key"
        : (mode == TLS_MODE_RSA) ? "pki/output/classical/rsa/client.key"
        : "pki/output/classical/ecdsa/client.key";

    LOG_INFO("=== Stress Test ===");
    LOG_INFO("Mode:    %s", mode_str);
    LOG_INFO("Clients: %d (simultaneous)", num_clients);

    /* --- Allocate thread contexts --- */
    pthread_t    *threads = calloc(num_clients, sizeof(pthread_t));
    thread_ctx_t *ctxs    = calloc(num_clients, sizeof(thread_ctx_t));

    for (int i = 0; i < num_clients; i++) {
        ctxs[i].thread_id  = i;
        ctxs[i].mode       = mode;
        ctxs[i].broker_uri = broker_uri;
        ctxs[i].ca_cert    = ca_cert;
        ctxs[i].client_cert = client_cert;
        ctxs[i].client_key  = client_key;
    }

    /* --- Launch all threads simultaneously --- */
    pqc_timer_t total_timer;
    timer_start(&total_timer);

    for (int i = 0; i < num_clients; i++) {
        pthread_create(&threads[i], NULL, client_thread, &ctxs[i]);
    }

    /* --- Wait for all threads --- */
    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    timer_stop(&total_timer);

    /* --- Collect results --- */
    int    success_count = 0;
    uint64_t sum_ns = 0, min_ns = UINT64_MAX, max_ns = 0;

    csv_logger_t logger;
    csv_logger_open(&logger, output_file,
                    "client_id,mode,num_clients,latency_ns,latency_ms,success");

    for (int i = 0; i < num_clients; i++) {
        if (ctxs[i].success) {
            success_count++;
            sum_ns += ctxs[i].latency_ns;
            if (ctxs[i].latency_ns < min_ns) min_ns = ctxs[i].latency_ns;
            if (ctxs[i].latency_ns > max_ns) max_ns = ctxs[i].latency_ns;
        }
        csv_logger_write(&logger, "%d,%s,%d,%lu,%.6f,%d",
                         i, mode_str, num_clients,
                         (unsigned long)ctxs[i].latency_ns,
                         (double)ctxs[i].latency_ns / 1e6,
                         ctxs[i].success);
    }

    csv_logger_close(&logger);

    /* --- Summary --- */
    LOG_INFO("");
    LOG_INFO("=== Stress Test Results (%s, %d clients) ===", mode_str, num_clients);
    LOG_INFO("Success:     %d / %d", success_count, num_clients);
    LOG_INFO("Total time:  %.3f ms", timer_elapsed_ms(&total_timer));

    if (success_count > 0) {
        LOG_INFO("Avg latency: %.3f ms", (double)sum_ns / success_count / 1e6);
        LOG_INFO("Min latency: %.3f ms", (double)min_ns / 1e6);
        LOG_INFO("Max latency: %.3f ms", (double)max_ns / 1e6);
    }

    free(threads);
    free(ctxs);

    LOG_INFO("Data written to: %s", output_file);
    return PQC_OK;
}
