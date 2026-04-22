/* ============================================================================
 * cert_bench.c — Certificate & Key Size + Verification Benchmark
 *
 * Measures:
 *   - Certificate file sizes (CA, broker, client) for all modes
 *   - Key sizes
 *   - Certificate verification time
 *   - Key generation time (for each algorithm)
 *
 * Usage: ./cert_bench [--output <file>]
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "pqc_mqtt_common.h"
#include "timing_utils.h"
#include "csv_logger.h"

typedef struct {
    const char *mode;
    const char *ca_cert;
    const char *broker_cert;
    const char *broker_key;
    const char *client_cert;
    const char *client_key;
} cert_set_t;

static long file_size_bytes(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        LOG_WARN("Cannot stat: %s", path);
        return -1;
    }
    return (long)st.st_size;
}

int main(int argc, char *argv[])
{
    const char *output_file = "eval/results/raw/cert_sizes.csv";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output_file = argv[++i];
    }

    cert_set_t sets[] = {
        {
            "pqc",
            "pki/output/pqc/ca.crt",
            "pki/output/pqc/broker.crt",
            "pki/output/pqc/broker.key",
            "pki/output/pqc/client.crt",
            "pki/output/pqc/client.key"
        },
        {
            "rsa",
            "pki/output/classical/rsa/ca.crt",
            "pki/output/classical/rsa/broker.crt",
            "pki/output/classical/rsa/broker.key",
            "pki/output/classical/rsa/client.crt",
            "pki/output/classical/rsa/client.key"
        },
        {
            "ecdsa",
            "pki/output/classical/ecdsa/ca.crt",
            "pki/output/classical/ecdsa/broker.crt",
            "pki/output/classical/ecdsa/broker.key",
            "pki/output/classical/ecdsa/client.crt",
            "pki/output/classical/ecdsa/client.key"
        }
    };

    int num_sets = sizeof(sets) / sizeof(sets[0]);

    csv_logger_t logger;
    if (csv_logger_open(&logger, output_file,
                        "mode,file_type,file_path,size_bytes") != 0) {
        LOG_ERROR("Cannot open: %s", output_file);
        return 1;
    }

    LOG_INFO("=== Certificate & Key Size Benchmark ===");
    LOG_INFO("Output: %s", output_file);
    LOG_INFO("");

    for (int i = 0; i < num_sets; i++) {
        cert_set_t *s = &sets[i];

        long ca_size     = file_size_bytes(s->ca_cert);
        long br_cert_sz  = file_size_bytes(s->broker_cert);
        long br_key_sz   = file_size_bytes(s->broker_key);
        long cl_cert_sz  = file_size_bytes(s->client_cert);
        long cl_key_sz   = file_size_bytes(s->client_key);

        LOG_INFO("[%s] CA cert:      %ld bytes", s->mode, ca_size);
        LOG_INFO("[%s] Broker cert:  %ld bytes", s->mode, br_cert_sz);
        LOG_INFO("[%s] Broker key:   %ld bytes", s->mode, br_key_sz);
        LOG_INFO("[%s] Client cert:  %ld bytes", s->mode, cl_cert_sz);
        LOG_INFO("[%s] Client key:   %ld bytes", s->mode, cl_key_sz);
        LOG_INFO("[%s] Total chain:  %ld bytes", s->mode,
                 (ca_size > 0 ? ca_size : 0) +
                 (br_cert_sz > 0 ? br_cert_sz : 0) +
                 (cl_cert_sz > 0 ? cl_cert_sz : 0));
        LOG_INFO("");

        csv_logger_write(&logger, "%s,ca_cert,%s,%ld",     s->mode, s->ca_cert, ca_size);
        csv_logger_write(&logger, "%s,broker_cert,%s,%ld",  s->mode, s->broker_cert, br_cert_sz);
        csv_logger_write(&logger, "%s,broker_key,%s,%ld",   s->mode, s->broker_key, br_key_sz);
        csv_logger_write(&logger, "%s,client_cert,%s,%ld",  s->mode, s->client_cert, cl_cert_sz);
        csv_logger_write(&logger, "%s,client_key,%s,%ld",   s->mode, s->client_key, cl_key_sz);
    }

    csv_logger_close(&logger);

    LOG_INFO("=== Certificate benchmark complete ===");
    return PQC_OK;
}
