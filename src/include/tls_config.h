/* ============================================================================
 * tls_config.h — TLS/SSL configuration helpers for PQC and classical modes
 * ============================================================================ */

#ifndef TLS_CONFIG_H
#define TLS_CONFIG_H

#include "MQTTClient.h"

/* --- TLS Mode enum --- */
typedef enum {
    TLS_MODE_PQC,        /* ML-DSA certs + ML-KEM key exchange */
    TLS_MODE_RSA,        /* RSA-2048 baseline */
    TLS_MODE_ECDSA       /* ECDSA P-256 baseline */
} tls_mode_t;

/* --- Certificate paths structure --- */
typedef struct {
    const char *ca_cert;
    const char *client_cert;
    const char *client_key;
} tls_cert_paths_t;

/**
 * Populate an MQTTClient_SSLOptions struct for the given TLS mode.
 *
 * @param ssl_opts   Pointer to the SSL options struct to populate.
 * @param mode       Which TLS mode to configure.
 * @param certs      Paths to CA, client cert, and client key files.
 * @return           0 on success, non-zero on error.
 */
int tls_config_init(MQTTClient_SSLOptions *ssl_opts,
                    tls_mode_t mode,
                    const tls_cert_paths_t *certs);

/**
 * Get the broker URI for the given TLS mode.
 */
const char *tls_config_get_broker_uri(tls_mode_t mode);

/**
 * Parse a string ("pqc", "rsa", "ecdsa") into a tls_mode_t.
 * Returns -1 on invalid input.
 */
int tls_config_parse_mode(const char *mode_str, tls_mode_t *out_mode);

#endif /* TLS_CONFIG_H */
