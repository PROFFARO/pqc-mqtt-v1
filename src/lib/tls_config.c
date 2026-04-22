/* ============================================================================
 * tls_config.c — TLS/SSL configuration implementation
 * ============================================================================ */

#include "tls_config.h"
#include "pqc_mqtt_common.h"
#include <string.h>

int tls_config_init(MQTTClient_SSLOptions *ssl_opts,
                    tls_mode_t mode,
                    const tls_cert_paths_t *certs)
{
    if (!ssl_opts || !certs) {
        LOG_ERROR("tls_config_init: NULL argument");
        return -1;
    }

    /* Start from the Paho default initializer */
    MQTTClient_SSLOptions default_opts = MQTTClient_SSLOptions_initializer;
    *ssl_opts = default_opts;

    /* Enable modern TLS version selection (struct_version >= 1) */
    ssl_opts->struct_version = 1;

    /* Set certificate paths */
    ssl_opts->trustStore  = certs->ca_cert;
    ssl_opts->keyStore    = certs->client_cert;
    ssl_opts->privateKey  = certs->client_key;

    /* Verify server certificate */
    ssl_opts->enableServerCertAuth = 1;

    /*
     * sslVersion = MQTT_SSL_VERSION_DEFAULT lets OpenSSL negotiate
     * the highest version (TLS 1.3) automatically.
     */
    ssl_opts->sslVersion = MQTT_SSL_VERSION_DEFAULT;

    switch (mode) {
    case TLS_MODE_PQC:
        LOG_INFO("TLS mode: PQC (ML-DSA certs, ML-KEM-768 key exchange)");
        /*
         * With oqs-provider loaded via OPENSSL_CONF, OpenSSL will
         * automatically advertise ML-KEM groups and accept ML-DSA certs.
         * No explicit cipher suite override needed.
         */
        break;

    case TLS_MODE_RSA:
        LOG_INFO("TLS mode: Classical RSA-2048");
        break;

    case TLS_MODE_ECDSA:
        LOG_INFO("TLS mode: Classical ECDSA P-256");
        break;

    default:
        LOG_ERROR("Unknown TLS mode: %d", (int)mode);
        return -1;
    }

    return 0;
}

const char *tls_config_get_broker_uri(tls_mode_t mode)
{
    switch (mode) {
    case TLS_MODE_PQC:
        return PQC_BROKER_URI_PQC;
    case TLS_MODE_RSA:
        return PQC_BROKER_URI_RSA;
    case TLS_MODE_ECDSA:
        return PQC_BROKER_URI_ECDSA;
    default:
        return PQC_BROKER_URI_PQC;
    }
}

int tls_config_parse_mode(const char *mode_str, tls_mode_t *out_mode)
{
    if (!mode_str || !out_mode) return -1;

    if (strcmp(mode_str, "pqc") == 0) {
        *out_mode = TLS_MODE_PQC;
        return 0;
    } else if (strcmp(mode_str, "rsa") == 0) {
        *out_mode = TLS_MODE_RSA;
        return 0;
    } else if (strcmp(mode_str, "ecdsa") == 0) {
        *out_mode = TLS_MODE_ECDSA;
        return 0;
    }

    LOG_ERROR("Invalid TLS mode: '%s'. Use: pqc, rsa, ecdsa", mode_str);
    return -1;
}
