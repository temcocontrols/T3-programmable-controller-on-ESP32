/**
 * SMTP email client
 *
 * Adapted from the `ssl_mail_client` example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include <mbedtls/base64.h>
#include <sys/param.h>

#include "lwip/sockets.h"


#include "driver/uart.h"
extern char debug_array[100];

#if 0
/* Constants that are configurable in menuconfig */
#define MAIL_SERVER         "192.168.0.7"//CONFIG_SMTP_SERVER ��192.168.0.7��
#define MAIL_PORT           "25"//CONFIG_SMTP_PORT_NUMBER
#define SENDER_MAIL         "chelsea@temcocontrols.com"//CONFIG_SMTP_SENDER_MAIL ��chelsea@temcocontrols.com��
#define SENDER_PASSWORD     "GhyR$d@!@" //CONFIG_SMTP_SENDER_PASSWORD
#define RECIPIENT_MAIL      "chelsea@temcocontrols.com"//CONFIG_SMTP_RECIPIENT_MAIL
#endif


#define MAIL_SERVER1         "mail.smtp2go.com"//CONFIG_SMTP_SERVER ��192.168.0.7��
#define MAIL_PORT1           "2525"//CONFIG_SMTP_PORT_NUMBER
#define SENDER_MAIL1         "T3@temcocontrols.com"//CONFIG_SMTP_SENDER_MAIL ��chelsea@temcocontrols.com��
#define SENDER_PASSWORD1     "aHRwODV4eW1qcHcw" //CONFIG_SMTP_SENDER_PASSWORD
#define RECIPIENT_MAIL1      "chelsea@temcocontrols.com"//CONFIG_SMTP_RECIPIENT_MAIL


#define SERVER_USES_STARTSSL 	1

static const char *TAG = "smtp_example";

#define TASK_STACK_SIZE     (8 * 1024)
#define BUF_SIZE            512

extern unsigned int Test[50];
void debug_info(char *string);
#define VALIDATE_MBEDTLS_RETURN(ret, min_valid_ret, max_valid_ret, goto_label)  \
    do {                                                                        \
        if (ret < min_valid_ret || ret > max_valid_ret) {                       \
            goto goto_label;                                                    \
        }                                                                       \
    } while (0)                                                                 \

/**
 * Root cert for smtp.googlemail.com, taken from server_root_cert.pem
 *
 * The PEM file was extracted from the output of this command:
 * openssl s_client -showcerts -connect smtp.googlemail.com:587 -starttls smtp
 *
 * The CA root cert is the last cert given in the chain of certs.
 *
 * To embed it in the app binary, the PEM file is named
 * in the component.mk COMPONENT_EMBED_TXTFILES variable.
 */

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

//extern const uint8_t esp_logo_png_start[] asm("_binary_esp_logo_png_start");
//extern const uint8_t esp_logo_png_end[]   asm("_binary_esp_logo_png_end");
#if 1
static int write_and_get_response(mbedtls_net_context *sock_fd, unsigned char *buf, size_t len)
{
    int ret;
    const size_t DATA_SIZE = 128;
    unsigned char data[DATA_SIZE];
    char code[4];
    size_t i, idx = 0;

    if (len) {
        ESP_LOGD(TAG, "%s", buf);
    }

    if (len && (ret = mbedtls_net_send(sock_fd, buf, len)) <= 0) {
        ESP_LOGE(TAG, "mbedtls_net_send failed with error -0x%x", -ret);
        return ret;
    }

    do {
        len = DATA_SIZE - 1;
        ret = mbedtls_net_recv(sock_fd, data, len);

        if (ret <= 0) {
            ESP_LOGE(TAG, "mbedtls_net_recv failed with error -0x%x", -ret);
            goto exit;
        }

        data[len] = '\0';
        printf("\n%s", data);
        len = ret;
        for (i = 0; i < len; i++) {
            if (data[i] != '\n') {
                if (idx < 4) {
                    code[idx++] = data[i];
                }
                continue;
            }

            if (idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ') {
                code[3] = '\0';
                ret = atoi(code);
                goto exit;
            }

            idx = 0;
        }
    } while (1);

exit:
    return ret;
}

static int write_ssl_and_get_response(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
    int ret;
    const size_t DATA_SIZE = 128;
    unsigned char data[DATA_SIZE];
    char code[4];
    size_t i, idx = 0;
    if (len) {
        ESP_LOGD(TAG, "%s", buf);
        printf("%s",buf);
    }

    while (len && (ret = mbedtls_ssl_write(ssl, buf, len)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        	debug_info("mbedtls_ssl_write failed with error");
            goto exit;
        }
    }

    do {
        len = DATA_SIZE - 1;
        ret = mbedtls_ssl_read(ssl, data, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue;
        }

        if (ret <= 0) {Test[12]++;
            goto exit;
        }

        ESP_LOGD(TAG, "%s", data);

        len = ret;
        for (i = 0; i < len; i++) {
            if (data[i] != '\n') {
                if (idx < 4) {
                    code[idx++] = data[i];
                }
                continue;
            }

            if (idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ') {
                code[3] = '\0';
                ret = atoi(code);
                goto exit;
            }

            idx = 0;
        }
    } while (1);

exit:
	Test[14]++;
	Test[15] = ret;
	debug_info("write_ssl_and_get_response end");
    return ret;
}

static int write_ssl_data(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
    int ret;

    if (len) {
        ESP_LOGD(TAG, "%s", buf);
    }

    while (len && (ret = mbedtls_ssl_write(ssl, buf, len)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "mbedtls_ssl_write failed with error -0x%x", -ret);
            return ret;
        }
    }

    return 0;
}

static int perform_tls_handshake(mbedtls_ssl_context *ssl)
{
    int ret = -1;
    uint32_t flags;
    char *buf = NULL;
    buf = (char *) calloc(1, BUF_SIZE);
    if (buf == NULL) {
    	debug_info( "calloc failed for size ");
        goto exit;
    }

    debug_info("Performing the SSL/TLS handshake...");

    fflush(stdout);
    while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        	debug_info("mbedtls_ssl_handshake returned");
            goto exit;
        }
    }
    mbedtls_ssl_handshake(ssl);

    debug_info("Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(ssl)) != 0) {
        /* In real life, we probably want to close connection if ret != 0 */
    	debug_info("Failed to verify peer certificate!");
        mbedtls_x509_crt_verify_info(buf, BUF_SIZE, "  ! ", flags);
        debug_info("verification info");
    } else {
    	debug_info("Certificate verified.");
    }

    ESP_LOGI(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(ssl));
    ret = 0; /* No error */

exit:
    if (buf) {
        free(buf);
    }
    return ret;
}

#if 1
void debug_info2(char *str)
{
    //sprintf(debug_array,str/*"mbedtls_ssl_init\r\n"*/);
    //uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
}
#endif

void smtp_client_task_ssl(void)
//static void smtp_client_task(void *pvParameters)
{
    char *buf = NULL;
    unsigned char base64_buffer[128];
    int ret, len;
    size_t base64_len;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    //debug_info2("Seeding the random number generator\r\n");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL, 0)) != 0) {
    	debug_info2("mbedtls_ctr_drbg_seed returned\r\n");
        goto exit;
    }

    //debug_info2("Loading the CA root certificate...\r\n");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end - server_root_cert_pem_start);

    if (ret < 0) {
    	debug_info2("mbedtls_x509_crt_parse returned\r\n");
        goto exit;
    }

    //debug_info2("Setting hostname for TLS session...\r\n");

    /* Hostname set here should match CN in server certificate */
    if ((ret = mbedtls_ssl_set_hostname(&ssl, MAIL_SERVER1)) != 0) {
    	debug_info2("mbedtls_ssl_set_hostname returned\r\n");
        goto exit;
    }

    //debug_info2("Setting up the SSL/TLS structure...\r\n");

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    	debug_info2("mbedtls_ssl_config_defaults returned\r\n");
        goto exit;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    	debug_info2("mbedtls_ssl_setup returned\r\n");
        goto exit;
    }

    mbedtls_net_init(&server_fd);

    debug_info2("Connecting to ...\r\n");

    if ((ret = mbedtls_net_connect(&server_fd, MAIL_SERVER1,
         MAIL_PORT1, MBEDTLS_NET_PROTO_TCP)) != 0) {
    	debug_info2("mbedtls_net_connect returned");
        goto exit;
    }

    debug_info2("Connected.\r\n");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

   //debug_info2("mbedtls_ssl_set_bio.\r\n");

    buf = (char *) calloc(1, BUF_SIZE);
    if (buf == NULL) {
    	debug_info2("calloc failed for size");
        goto exit;
    }
#if SERVER_USES_STARTSSL
    //debug_info2("SERVER_USES_STARTSSL...");
    /* Get response */
    ret = write_and_get_response(&server_fd, (unsigned char *) buf, 0);
    Test[40] = 1;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

    //debug_info2("Writing EHLO to server...");
    len = snprintf((char *) buf, BUF_SIZE, "EHLO %s\r\n", "ESP32");
    ret = write_and_get_response(&server_fd, (unsigned char *) buf, len);
    Test[40] = 2;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

    debug_info2("Writing STARTTLS to server...");
    len = snprintf((char *) buf, BUF_SIZE, "STARTTLS\r\n");
    ret = write_and_get_response(&server_fd, (unsigned char *) buf, len);
    Test[40] = 3;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

    debug_info2("perform_tls_handshake");
    ret = perform_tls_handshake(&ssl);
    Test[40] = 3;
    if (ret != 0) {
        goto exit;
    }

#else /* SERVER_USES_STARTSSL */
    ret = perform_tls_handshake(&ssl);
    Test[40] = 4;
    if (ret != 0) {debug_info2("Error perform_tls_handshake");
        goto exit;
    }

    debug_info2("perform_tls_handshake ok");

    /* Get response */
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, 0);
    Test[40] = 5;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);
    debug_info2("Writing EHLO to server...");

    len = snprintf((char *) buf, BUF_SIZE, "EHLO %s\r\n", "ESP32");
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 6;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

#endif /* SERVER_USES_STARTSSL */

    /* Authentication */
    debug_info2("Authentication...");

    ESP_LOGI(TAG, "Write AUTH LOGIN");
    len = snprintf( (char *) buf, BUF_SIZE, "AUTH LOGIN\r\n" );
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 7;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 399, exit);

    debug_info2("Write USER NAME");
    ret = mbedtls_base64_encode((unsigned char *) base64_buffer, sizeof(base64_buffer),
                                &base64_len, (unsigned char *) SENDER_MAIL1, strlen(SENDER_MAIL1));
    Test[40] = 8;
    if (ret != 0) {
        ESP_LOGE(TAG, "Error in mbedtls encode! ret = -0x%x", -ret);
        goto exit;
    }
    len = snprintf((char *) buf, BUF_SIZE, "%s\r\n", base64_buffer);
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 9;
    VALIDATE_MBEDTLS_RETURN(ret, 300, 399, exit);

    debug_info2("Write PASSWORD");
    ret = mbedtls_base64_encode((unsigned char *) base64_buffer, sizeof(base64_buffer),
                                &base64_len, (unsigned char *) SENDER_PASSWORD1, strlen(SENDER_PASSWORD1));
    Test[40] = 10;
    if (ret != 0) {
        ESP_LOGE(TAG, "Error in mbedtls encode! ret = -0x%x", -ret);
        goto exit;
    }
    len = snprintf((char *) buf, BUF_SIZE, "%s\r\n", base64_buffer);
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 11;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 399, exit);

    /* Compose email */
    debug_info2("Write MAIL FROM");
    len = snprintf((char *) buf, BUF_SIZE, "MAIL FROM:<%s>\r\n", SENDER_MAIL1);
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 12;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

    debug_info2("Write RCPT");
    len = snprintf((char *) buf, BUF_SIZE, "RCPT TO:<%s>\r\n", RECIPIENT_MAIL1);
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 13;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);

    debug_info2("Write DATA");
    len = snprintf((char *) buf, BUF_SIZE, "DATA\r\n");
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 14;
    VALIDATE_MBEDTLS_RETURN(ret, 300, 399, exit);

    debug_info2("Write Content");
    /* We do not take action if message sending is partly failed. */
    len = snprintf((char *) buf, BUF_SIZE,
                   "From: %s\r\nSubject: mbed TLS Test mail\r\n"
                   "To: %s\r\n"
                   "MIME-Version: 1.0 (mime-construct 1.9)\n",
                   "ESP32 SMTP Client", RECIPIENT_MAIL1);

    /**
     * Note: We are not validating return for some ssl_writes.
     * If by chance, it's failed; at worst email will be incomplete!
     */
    ret = write_ssl_data(&ssl, (unsigned char *) buf, len);
    Test[40] = 15;
    /* Multipart boundary */
    len = snprintf((char *) buf, BUF_SIZE,
                   "Content-Type: multipart/mixed;boundary=XYZabcd1234\n"
                   "--XYZabcd1234\n");
    ret = write_ssl_data(&ssl, (unsigned char *) buf, len);

    /* Text */
    len = snprintf((char *) buf, BUF_SIZE,
                   "Content-Type: text/plain\n"
                   "This is a simple test mail from the SMTP client example.\r\n"
                   "\r\n"
                   "Enjoy!\n\n--XYZabcd1234\n");
    ret = write_ssl_data(&ssl, (unsigned char *) buf, len);
    Test[40] = 16;
    /* Attachment */
    len = snprintf((char *) buf, BUF_SIZE,
                   "Content-Type: image/png;name=esp_logo.png\n"
                   "Content-Transfer-Encoding: base64\n"
                   "Content-Disposition:attachment;filename=\"esp_logo.png\"\r\n\n");
    ret = write_ssl_data(&ssl, (unsigned char *) buf, len);
    Test[40] = 17;
#if 0
    /* Image contents... */
    const uint8_t *offset = esp_logo_png_start;
    while (offset < esp_logo_png_end - 1) {
        int read_bytes = MIN(((sizeof (base64_buffer) - 1) / 4) * 3, esp_logo_png_end - offset - 1);
        ret = mbedtls_base64_encode((unsigned char *) base64_buffer, sizeof(base64_buffer),
                                    &base64_len, (unsigned char *) offset, read_bytes);
        if (ret != 0) {
            ESP_LOGE(TAG, "Error in mbedtls encode! ret = -0x%x", -ret);
            goto exit;
        }
        offset += read_bytes;
        len = snprintf((char *) buf, BUF_SIZE, "%s\r\n", base64_buffer);
        ret = write_ssl_data(&ssl, (unsigned char *) buf, len);
    }
#endif

    len = snprintf((char *) buf, BUF_SIZE, "\n--XYZabcd1234\n");
    ret = write_ssl_data(&ssl, (unsigned char *) buf, len);
    Test[40] = 18;
    len = snprintf((char *) buf, BUF_SIZE, "\r\n.\r\n");
    ret = write_ssl_and_get_response(&ssl, (unsigned char *) buf, len);
    Test[40] = 19;
    VALIDATE_MBEDTLS_RETURN(ret, 200, 299, exit);
    debug_info2("Email sent!");

    /* Close connection */
    mbedtls_ssl_close_notify(&ssl);
    ret = 0; /* No errors */

exit:
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

#if 0
    sprintf(debug_array,"exit %d,%d",ret,Test[40]);
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
#endif
    if (ret != 0) {
        mbedtls_strerror(ret, buf, 100);
        debug_info2("Last error was");
    }

    putchar('\n'); /* Just a new line */
    if (buf) {
        free(buf);
    }
    //vTaskDelete(NULL);
}
#endif


#if 0
void smtp_client_task_nossl(void)
{
	char rx_buffer[200];
	int addr_family;
	int ip_protocol;
	char *buf = NULL;
	unsigned char base64_buffer[128];
	int ret, len;
	size_t base64_len;
	int err;
	int sock;
	mbedtls_net_context server_fd;

	mbedtls_net_init(&server_fd);
	buf = (char *) calloc(1, BUF_SIZE);

	debug_info("Connecting to ....");

	if ((ret = mbedtls_net_connect(&server_fd, MAIL_SERVER,
								   MAIL_PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
		debug_info("mbedtls_net_connect returned");
		goto exit;
	}

	sock = server_fd.fd;

	len = snprintf((char *) buf, BUF_SIZE, "EHLO %s\r\n", "ESP32");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = snprintf((char *) buf, BUF_SIZE, "STARTTLS\r\n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;


	len = snprintf( (char *) buf, BUF_SIZE, "AUTH LOGIN\r\n" );
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}


	ret = mbedtls_base64_encode((unsigned char *) base64_buffer, sizeof(base64_buffer),
	                                &base64_len, (unsigned char *) SENDER_MAIL, strlen(SENDER_MAIL));
	if (ret != 0) {
		goto exit;
	}
	len = snprintf((char *) buf, BUF_SIZE, "%s\r\n", base64_buffer);
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}



	ret = mbedtls_base64_encode((unsigned char *) base64_buffer, sizeof(base64_buffer),
								&base64_len, (unsigned char *) SENDER_PASSWORD, strlen(SENDER_PASSWORD));
	if (ret != 0){
		goto exit;
	}
	len = snprintf((char *) buf, BUF_SIZE, "%s\r\n", base64_buffer);
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}

	    /* Compose email */

	len = snprintf((char *) buf, BUF_SIZE, "MAIL FROM:<%s>\r\n", SENDER_MAIL);
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}

	len = snprintf((char *) buf, BUF_SIZE, "RCPT TO:<%s>\r\n", RECIPIENT_MAIL);
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}


	len = snprintf((char *) buf, BUF_SIZE, "DATA\r\n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}

	/* We do not take action if message sending is partly failed. */
	len = snprintf((char *) buf, BUF_SIZE,
				   "From: %s\r\nSubject: chelsea mail test3333\r\n"
				   "To: %s\r\n"
				   "MIME-Version: 1.0 (mime-construct 1.9)\n",
				   "ESP32 SMTP Client", RECIPIENT_MAIL);

	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	/*len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}*/

	/* Multipart boundary */
	len = snprintf((char *) buf, BUF_SIZE,
				   "Content-Type: multipart/mixed;boundary=XYZabcd1234\n"
				   "--XYZabcd1234\n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
			goto exit;
	/*len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}*/

	/* Text */
	len = snprintf((char *) buf, BUF_SIZE,
				   "--This is a test3333 by Chelsea. \n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;
	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}

	/* Attachment
	len = snprintf((char *) buf, BUF_SIZE,
				   "Content-Type: image/png;name=esp_logo.png\n"
				   "Content-Transfer-Encoding: base64\n"
				   "Content-Disposition:attachment;filename=\"esp_logo.png\"\r\n\n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}*/

	len = snprintf((char *) buf, BUF_SIZE, "\r\n.\r\n");
	debug_info(buf);
	err = send(sock, buf,len, 0);
	if (err < 0)
		goto exit;

	len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
	if(len > 0)
	{
		debug_info(rx_buffer);
	}

	exit:
	 debug_info("exit");
}
#endif

#if 0
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /**
     * This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    xTaskCreate(&smtp_client_task, "smtp_client_task", TASK_STACK_SIZE, NULL, 5, NULL);
}
#endif
