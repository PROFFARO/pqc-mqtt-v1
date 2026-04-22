/* ============================================================================
 * handshake_bench.c — TLS handshake latency benchmark
 *
 * Performs N connect/disconnect cycles and records each handshake latency
 * to a CSV file. This is the primary benchmark for comparing PQC vs classical
 * TLS performance.
 *
 * Usage: ./handshake_bench --mode pqc|rsa|ecdsa [--iterations 100] [--output <file>]
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

#define BENCH_CLIENT_ID  "bench-handshake"

typedef struct {
    tls_mode_t  mode;
    int         iterations;
    int         warmup;
    const char *output_file;
    const char *ca_cert;
    const char *client_cert;
    const char *client_key;
} bench_config_t;

static int parse_args(int argc, char *argv[], bench_config_t *cfg)
{
    cfg->iterations  = BENCH_DEFAULT_ITERATIONS;
    cfg->warmup      = BENCH_WARMUP_ITERATIONS;
    cfg->output_file = "eval/results/raw/handshake_latency.csv";
    cfg->ca_cert     = NULL;
    cfg->client_cert = NULL;
    cfg->client_key  = NULL;

    int mode_set = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (tls_config_parse_mode(argv[++i], &cfg->mode) != 0) return -1;
            mode_set = 1;
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            cfg->iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            cfg->warmup = atoi(argv[++i]);
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
        fprintf(stderr, "Usage: %s --mode <pqc|rsa|ecdsa> [--iterations N]\n", argv[0]);
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
    bench_config_t cfg;
    if (parse_args(argc, argv, &cfg) != 0)
        return PQC_ERR_ARGS;

    const char *mode_str = (cfg.mode == TLS_MODE_PQC) ? "pqc"
                         : (cfg.mode == TLS_MODE_RSA) ? "rsa" : "ecdsa";
    const char *broker_uri = tls_config_get_broker_uri(cfg.mode);

    LOG_INFO("=== Handshake Latency Benchmark ===");
    LOG_INFO("Mode:       %s", mode_str);
    LOG_INFO("Broker:     %s", broker_uri);
    LOG_INFO("Iterations: %d (+ %d warmup)", cfg.iterations, cfg.warmup);
    LOG_INFO("Output:     %s", cfg.output_file);

    /* --- Prepare CSV logger --- */
    csv_logger_t logger;
    if (csv_logger_open(&logger, cfg.output_file,
                        "iteration,mode,latency_ns,latency_ms") != 0) {
        LOG_ERROR("Cannot open output file: %s", cfg.output_file);
        return PQC_ERR_ARGS;
    }

    /* --- TLS config (reused each iteration) --- */
    tls_cert_paths_t certs = {
        .ca_cert     = cfg.ca_cert,
        .client_cert = cfg.client_cert,
        .client_key  = cfg.client_key
    };

    int total = cfg.warmup + cfg.iterations;
    uint64_t sum_ns = 0;
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    int success_count = 0;

    for (int i = 0; i < total; i++) {
        int is_warmup = (i < cfg.warmup);

        /* Create a fresh client each iteration */
        MQTTClient client;
        char client_id[64];
        snprintf(client_id, sizeof(client_id), "%s-%d", BENCH_CLIENT_ID, i);

        int rc = MQTTClient_create(&client, broker_uri, client_id,
                                   MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (rc != MQTTCLIENT_SUCCESS) {
            LOG_WARN("[%d] Client create failed: %d", i, rc);
            continue;
        }

        MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
        tls_config_init(&ssl_opts, cfg.mode, &certs);

        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        conn_opts.keepAliveInterval = PQC_MQTT_KEEPALIVE_SEC;
        conn_opts.cleansession      = 1;
        conn_opts.ssl               = &ssl_opts;

        /* --- Timed connect --- */
        pqc_timer_t timer;
        timer_start(&timer);
        rc = MQTTClient_connect(client, &conn_opts);
        timer_stop(&timer);

        if (rc == MQTTCLIENT_SUCCESS) {
            uint64_t elapsed_ns = timer_elapsed_ns(&timer);
            double   elapsed_ms = timer_elapsed_ms(&timer);

            if (!is_warmup) {
                csv_logger_write(&logger, "%d,%s,%lu,%.6f",
                                 i - cfg.warmup, mode_str,
                                 (unsigned long)elapsed_ns, elapsed_ms);
                sum_ns += elapsed_ns;
                if (elapsed_ns < min_ns) min_ns = elapsed_ns;
                if (elapsed_ns > max_ns) max_ns = elapsed_ns;
                success_count++;
            }

            if (i % 10 == 0 || is_warmup) {
                LOG_INFO("[%s%d] Handshake: %.3f ms",
                         is_warmup ? "W" : "", i, elapsed_ms);
            }

            MQTTClient_disconnect(client, 1000);
        } else {
            LOG_WARN("[%d] Connect failed: %d", i, rc);
        }

        MQTTClient_destroy(&client);

        /* Small delay between iterations to avoid overwhelming the broker */
        usleep(50000); /* 50ms */
    }

    csv_logger_close(&logger);

    /* --- Summary statistics --- */
    LOG_INFO("");
    LOG_INFO("=== Benchmark Results (%s) ===", mode_str);
    LOG_INFO("Successful:  %d / %d", success_count, cfg.iterations);

    if (success_count > 0) {
        double avg_ms = (double)sum_ns / success_count / 1e6;
        double min_ms = (double)min_ns / 1e6;
        double max_ms = (double)max_ns / 1e6;

        LOG_INFO("Average:     %.3f ms", avg_ms);
        LOG_INFO("Min:         %.3f ms", min_ms);
        LOG_INFO("Max:         %.3f ms", max_ms);
    }

    LOG_INFO("Data written to: %s", cfg.output_file);
    LOG_INFO("=== Benchmark complete ===");

    return PQC_OK;
}
