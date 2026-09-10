#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_ERR_SSL_WANT_READ -0x6900
#define MBEDTLS_ERR_SSL_WANT_WRITE -0x6880

#define MBEDTLS_SSL_IS_CLIENT 0
#define MBEDTLS_SSL_TRANSPORT_STREAM 0
#define MBEDTLS_SSL_PRESET_DEFAULT 0
#define MBEDTLS_SSL_VERIFY_REQUIRED 2

typedef struct {
    int dummy;
} mbedtls_ssl_context;

typedef struct {
    int dummy;
} mbedtls_ssl_config;

typedef struct {
    int dummy;
} mbedtls_x509_crt;

void mbedtls_ssl_init(mbedtls_ssl_context *ssl);
void mbedtls_x509_crt_init(mbedtls_x509_crt *crt);
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);

int mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname);
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf, int endpoint, int transport, int preset);
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode);
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain, void *ca_crl);
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);
int mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf);
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl, void *p_bio,
                         int (*f_send)(void *, const unsigned char *, size_t),
                         int (*f_recv)(void *, unsigned char *, size_t),
                         int (*f_recv_timeout)(void *, unsigned char *, size_t, uint32_t));

int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl);
uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl);
int mbedtls_x509_crt_verify_info(char *buf, size_t size, const char *prefix, uint32_t flags);
const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl);
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len);
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len);
int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl);

void mbedtls_x509_crt_free(mbedtls_x509_crt *crt);
void mbedtls_ssl_free(mbedtls_ssl_context *ssl);
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);

#endif // MBEDTLS_SSL_H
