/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * Source file for all wolfSSL specific code for the TLS/SSL layer. No code
 * but vtls.c should ever call or use these functions.
 *
 */

#include "curl_setup.h"

#ifdef USE_WOLFSSL

#define WOLFSSL_OPTIONS_IGNORE_SYS
#include <wolfssl/version.h>
#include <wolfssl/options.h>

/* To determine what functions are available we rely on one or both of:
   - the user's options.h generated by wolfSSL
   - the symbols detected by curl's configure
   Since they are markedly different from one another, and one or the other may
   not be available, we do some checking below to bring things in sync. */

/* HAVE_ALPN is wolfSSL's build time symbol for enabling ALPN in options.h. */
#ifndef HAVE_ALPN
#ifdef HAVE_WOLFSSL_USEALPN
#define HAVE_ALPN
#endif
#endif

/* WOLFSSL_ALLOW_SSLV3 is wolfSSL's build time symbol for enabling SSLv3 in
   options.h, but is only seen in >= 3.6.6 since that's when they started
   disabling SSLv3 by default. */
#ifndef WOLFSSL_ALLOW_SSLV3
#if (LIBWOLFSSL_VERSION_HEX < 0x03006006) || \
  defined(HAVE_WOLFSSLV3_CLIENT_METHOD)
#define WOLFSSL_ALLOW_SSLV3
#endif
#endif

#include <limits.h>

#include "urldata.h"
#include "sendf.h"
#include "inet_pton.h"
#include "vtls.h"
#include "keylog.h"
#include "parsedate.h"
#include "connect.h" /* for the connect timeout */
#include "select.h"
#include "strcase.h"
#include "x509asn1.h"
#include "curl_printf.h"
#include "multiif.h"

#include <wolfssl/openssl/ssl.h>
#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include "wolfssl.h"

/* The last #include files should be: */
#include "curl_memory.h"
#include "memdebug.h"

/* KEEP_PEER_CERT is a product of the presence of build time symbol
   OPENSSL_EXTRA without NO_CERTS, depending on the version. KEEP_PEER_CERT is
   in wolfSSL's settings.h, and the latter two are build time symbols in
   options.h. */
#ifndef KEEP_PEER_CERT
#if defined(HAVE_WOLFSSL_GET_PEER_CERTIFICATE) || \
    (defined(OPENSSL_EXTRA) && !defined(NO_CERTS))
#define KEEP_PEER_CERT
#endif
#endif

struct ssl_backend_data {
  SSL_CTX* ctx;
  SSL*     handle;
};

static Curl_recv wolfssl_recv;
static Curl_send wolfssl_send;

#ifdef OPENSSL_EXTRA
/*
 * Availability note:
 * The TLS 1.3 secret callback (wolfSSL_set_tls13_secret_cb) was added in
 * WolfSSL 4.4.0, but requires the -DHAVE_SECRET_CALLBACK build option. If that
 * option is not set, then TLS 1.3 will not be logged.
 * For TLS 1.2 and before, we use wolfSSL_get_keys().
 * SSL_get_client_random and wolfSSL_get_keys require OPENSSL_EXTRA
 * (--enable-opensslextra or --enable-all).
 */
#if defined(HAVE_SECRET_CALLBACK) && defined(WOLFSSL_TLS13)
static int
wolfssl_tls13_secret_callback(SSL *ssl, int id, const unsigned char *secret,
                              int secretSz, void *ctx)
{
  const char *label;
  unsigned char client_random[SSL3_RANDOM_SIZE];
  (void)ctx;

  if(!ssl || !Curl_tls_keylog_enabled()) {
    return 0;
  }

  switch(id) {
  case CLIENT_EARLY_TRAFFIC_SECRET:
    label = "CLIENT_EARLY_TRAFFIC_SECRET";
    break;
  case CLIENT_HANDSHAKE_TRAFFIC_SECRET:
    label = "CLIENT_HANDSHAKE_TRAFFIC_SECRET";
    break;
  case SERVER_HANDSHAKE_TRAFFIC_SECRET:
    label = "SERVER_HANDSHAKE_TRAFFIC_SECRET";
    break;
  case CLIENT_TRAFFIC_SECRET:
    label = "CLIENT_TRAFFIC_SECRET_0";
    break;
  case SERVER_TRAFFIC_SECRET:
    label = "SERVER_TRAFFIC_SECRET_0";
    break;
  case EARLY_EXPORTER_SECRET:
    label = "EARLY_EXPORTER_SECRET";
    break;
  case EXPORTER_SECRET:
    label = "EXPORTER_SECRET";
    break;
  default:
    return 0;
  }

  if(SSL_get_client_random(ssl, client_random, SSL3_RANDOM_SIZE) == 0) {
    /* Should never happen as wolfSSL_KeepArrays() was called before. */
    return 0;
  }

  Curl_tls_keylog_write(label, client_random, secret, secretSz);
  return 0;
}
#endif /* defined(HAVE_SECRET_CALLBACK) && defined(WOLFSSL_TLS13) */

static void
wolfssl_log_tls12_secret(SSL *ssl)
{
  unsigned char *ms, *sr, *cr;
  unsigned int msLen, srLen, crLen, i, x = 0;

#if LIBWOLFSSL_VERSION_HEX >= 0x0300d000 /* >= 3.13.0 */
  /* wolfSSL_GetVersion is available since 3.13, we use it instead of
   * SSL_version since the latter relies on OPENSSL_ALL (--enable-opensslall or
   * --enable-all). Failing to perform this check could result in an unusable
   * key log line when TLS 1.3 is actually negotiated. */
  switch(wolfSSL_GetVersion(ssl)) {
  case WOLFSSL_SSLV3:
  case WOLFSSL_TLSV1:
  case WOLFSSL_TLSV1_1:
  case WOLFSSL_TLSV1_2:
    break;
  default:
    /* TLS 1.3 does not use this mechanism, the "master secret" returned below
     * is not directly usable. */
    return;
  }
#endif

  if(SSL_get_keys(ssl, &ms, &msLen, &sr, &srLen, &cr, &crLen) != SSL_SUCCESS) {
    return;
  }

  /* Check for a missing master secret and skip logging. That can happen if
   * curl rejects the server certificate and aborts the handshake.
   */
  for(i = 0; i < msLen; i++) {
    x |= ms[i];
  }
  if(x == 0) {
    return;
  }

  Curl_tls_keylog_write("CLIENT_RANDOM", cr, ms, msLen);
}
#endif /* OPENSSL_EXTRA */

static int do_file_type(const char *type)
{
  if(!type || !type[0])
    return SSL_FILETYPE_PEM;
  if(strcasecompare(type, "PEM"))
    return SSL_FILETYPE_PEM;
  if(strcasecompare(type, "DER"))
    return SSL_FILETYPE_ASN1;
  return -1;
}

/*
 * This function loads all the client/CA certificates and CRLs. Setup the TLS
 * layer and do all necessary magic.
 */
static CURLcode
wolfssl_connect_step1(struct connectdata *conn,
                     int sockindex)
{
  char *ciphers;
  struct Curl_easy *data = conn->data;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  SSL_METHOD* req_method = NULL;
  curl_socket_t sockfd = conn->sock[sockindex];
#ifdef HAVE_SNI
  bool sni = FALSE;
#define use_sni(x)  sni = (x)
#else
#define use_sni(x)  Curl_nop_stmt
#endif

  if(connssl->state == ssl_connection_complete)
    return CURLE_OK;

  if(SSL_CONN_CONFIG(version_max) != CURL_SSLVERSION_MAX_NONE) {
    failf(data, "wolfSSL does not support to set maximum SSL/TLS version");
    return CURLE_SSL_CONNECT_ERROR;
  }

  /* check to see if we've been told to use an explicit SSL/TLS version */
  switch(SSL_CONN_CONFIG(version)) {
  case CURL_SSLVERSION_DEFAULT:
  case CURL_SSLVERSION_TLSv1:
#if LIBWOLFSSL_VERSION_HEX >= 0x03003000 /* >= 3.3.0 */
    /* minimum protocol version is set later after the CTX object is created */
    req_method = SSLv23_client_method();
#else
    infof(data, "wolfSSL <3.3.0 cannot be configured to use TLS 1.0-1.2, "
          "TLS 1.0 is used exclusively\n");
    req_method = TLSv1_client_method();
#endif
    use_sni(TRUE);
    break;
  case CURL_SSLVERSION_TLSv1_0:
#ifdef WOLFSSL_ALLOW_TLSV10
    req_method = TLSv1_client_method();
    use_sni(TRUE);
#else
    failf(data, "wolfSSL does not support TLS 1.0");
    return CURLE_NOT_BUILT_IN;
#endif
    break;
  case CURL_SSLVERSION_TLSv1_1:
    req_method = TLSv1_1_client_method();
    use_sni(TRUE);
    break;
  case CURL_SSLVERSION_TLSv1_2:
    req_method = TLSv1_2_client_method();
    use_sni(TRUE);
    break;
  case CURL_SSLVERSION_TLSv1_3:
#ifdef WOLFSSL_TLS13
    req_method = wolfTLSv1_3_client_method();
    use_sni(TRUE);
    break;
#else
    failf(data, "wolfSSL: TLS 1.3 is not yet supported");
    return CURLE_SSL_CONNECT_ERROR;
#endif
  case CURL_SSLVERSION_SSLv3:
#ifdef WOLFSSL_ALLOW_SSLV3
    req_method = SSLv3_client_method();
    use_sni(FALSE);
#else
    failf(data, "wolfSSL does not support SSLv3");
    return CURLE_NOT_BUILT_IN;
#endif
    break;
  case CURL_SSLVERSION_SSLv2:
    failf(data, "wolfSSL does not support SSLv2");
    return CURLE_SSL_CONNECT_ERROR;
  default:
    failf(data, "Unrecognized parameter passed via CURLOPT_SSLVERSION");
    return CURLE_SSL_CONNECT_ERROR;
  }

  if(!req_method) {
    failf(data, "SSL: couldn't create a method!");
    return CURLE_OUT_OF_MEMORY;
  }

  if(backend->ctx)
    SSL_CTX_free(backend->ctx);
  backend->ctx = SSL_CTX_new(req_method);

  if(!backend->ctx) {
    failf(data, "SSL: couldn't create a context!");
    return CURLE_OUT_OF_MEMORY;
  }

  switch(SSL_CONN_CONFIG(version)) {
  case CURL_SSLVERSION_DEFAULT:
  case CURL_SSLVERSION_TLSv1:
#if LIBWOLFSSL_VERSION_HEX > 0x03004006 /* > 3.4.6 */
    /* Versions 3.3.0 to 3.4.6 we know the minimum protocol version is
     * whatever minimum version of TLS was built in and at least TLS 1.0. For
     * later library versions that could change (eg TLS 1.0 built in but
     * defaults to TLS 1.1) so we have this short circuit evaluation to find
     * the minimum supported TLS version.
    */
    if((wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1) != 1) &&
       (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_1) != 1) &&
       (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_2) != 1)
#ifdef WOLFSSL_TLS13
       && (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_3) != 1)
#endif
      ) {
      failf(data, "SSL: couldn't set the minimum protocol version");
      return CURLE_SSL_CONNECT_ERROR;
    }
#endif
    break;
  }

  ciphers = SSL_CONN_CONFIG(cipher_list);
  if(ciphers) {
    if(!SSL_CTX_set_cipher_list(backend->ctx, ciphers)) {
      failf(data, "failed setting cipher list: %s", ciphers);
      return CURLE_SSL_CIPHER;
    }
    infof(data, "Cipher selection: %s\n", ciphers);
  }

#ifndef NO_FILESYSTEM
  /* load trusted cacert */
  if(SSL_CONN_CONFIG(CAfile)) {
    if(1 != SSL_CTX_load_verify_locations(backend->ctx,
                                      SSL_CONN_CONFIG(CAfile),
                                      SSL_CONN_CONFIG(CApath))) {
      if(SSL_CONN_CONFIG(verifypeer)) {
        /* Fail if we insist on successfully verifying the server. */
        failf(data, "error setting certificate verify locations:\n"
              "  CAfile: %s\n  CApath: %s",
              SSL_CONN_CONFIG(CAfile)?
              SSL_CONN_CONFIG(CAfile): "none",
              SSL_CONN_CONFIG(CApath)?
              SSL_CONN_CONFIG(CApath) : "none");
        return CURLE_SSL_CACERT_BADFILE;
      }
      else {
        /* Just continue with a warning if no strict certificate
           verification is required. */
        infof(data, "error setting certificate verify locations,"
              " continuing anyway:\n");
      }
    }
    else {
      /* Everything is fine. */
      infof(data, "successfully set certificate verify locations:\n");
    }
    infof(data,
          "  CAfile: %s\n"
          "  CApath: %s\n",
          SSL_CONN_CONFIG(CAfile) ? SSL_CONN_CONFIG(CAfile):
          "none",
          SSL_CONN_CONFIG(CApath) ? SSL_CONN_CONFIG(CApath):
          "none");
  }

  /* Load the client certificate, and private key */
  if(SSL_SET_OPTION(cert) && SSL_SET_OPTION(key)) {
    int file_type = do_file_type(SSL_SET_OPTION(cert_type));

    if(SSL_CTX_use_certificate_file(backend->ctx, SSL_SET_OPTION(cert),
                                     file_type) != 1) {
      failf(data, "unable to use client certificate (no key or wrong pass"
            " phrase?)");
      return CURLE_SSL_CONNECT_ERROR;
    }

    file_type = do_file_type(SSL_SET_OPTION(key_type));
    if(SSL_CTX_use_PrivateKey_file(backend->ctx, SSL_SET_OPTION(key),
                                    file_type) != 1) {
      failf(data, "unable to set private key");
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
#endif /* !NO_FILESYSTEM */

  /* SSL always tries to verify the peer, this only says whether it should
   * fail to connect if the verification fails, or if it should continue
   * anyway. In the latter case the result of the verification is checked with
   * SSL_get_verify_result() below. */
  SSL_CTX_set_verify(backend->ctx,
                     SSL_CONN_CONFIG(verifypeer)?SSL_VERIFY_PEER:
                                                 SSL_VERIFY_NONE,
                     NULL);

#ifdef HAVE_SNI
  if(sni) {
    struct in_addr addr4;
#ifdef ENABLE_IPV6
    struct in6_addr addr6;
#endif
#ifndef CURL_DISABLE_PROXY
    const char * const hostname = SSL_IS_PROXY() ? conn->http_proxy.host.name :
      conn->host.name;
#else
    const char * const hostname = conn->host.name;
#endif
    size_t hostname_len = strlen(hostname);
    if((hostname_len < USHRT_MAX) &&
       (0 == Curl_inet_pton(AF_INET, hostname, &addr4)) &&
#ifdef ENABLE_IPV6
       (0 == Curl_inet_pton(AF_INET6, hostname, &addr6)) &&
#endif
       (wolfSSL_CTX_UseSNI(backend->ctx, WOLFSSL_SNI_HOST_NAME, hostname,
                          (unsigned short)hostname_len) != 1)) {
      infof(data, "WARNING: failed to configure server name indication (SNI) "
            "TLS extension\n");
    }
  }
#endif

  /* give application a chance to interfere with SSL set up. */
  if(data->set.ssl.fsslctx) {
    CURLcode result = (*data->set.ssl.fsslctx)(data, backend->ctx,
                                               data->set.ssl.fsslctxp);
    if(result) {
      failf(data, "error signaled by ssl ctx callback");
      return result;
    }
  }
#ifdef NO_FILESYSTEM
  else if(SSL_CONN_CONFIG(verifypeer)) {
    failf(data, "SSL: Certificates can't be loaded because wolfSSL was built"
          " with \"no filesystem\". Either disable peer verification"
          " (insecure) or if you are building an application with libcurl you"
          " can load certificates via CURLOPT_SSL_CTX_FUNCTION.");
    return CURLE_SSL_CONNECT_ERROR;
  }
#endif

  /* Let's make an SSL structure */
  if(backend->handle)
    SSL_free(backend->handle);
  backend->handle = SSL_new(backend->ctx);
  if(!backend->handle) {
    failf(data, "SSL: couldn't create a context (handle)!");
    return CURLE_OUT_OF_MEMORY;
  }

#ifdef HAVE_ALPN
  if(conn->bits.tls_enable_alpn) {
    char protocols[128];
    *protocols = '\0';

    /* wolfSSL's ALPN protocol name list format is a comma separated string of
       protocols in descending order of preference, eg: "h2,http/1.1" */

#ifdef USE_NGHTTP2
    if(data->set.httpversion >= CURL_HTTP_VERSION_2) {
      strcpy(protocols + strlen(protocols), NGHTTP2_PROTO_VERSION_ID ",");
      infof(data, "ALPN, offering %s\n", NGHTTP2_PROTO_VERSION_ID);
    }
#endif

    strcpy(protocols + strlen(protocols), ALPN_HTTP_1_1);
    infof(data, "ALPN, offering %s\n", ALPN_HTTP_1_1);

    if(wolfSSL_UseALPN(backend->handle, protocols,
                       (unsigned)strlen(protocols),
                       WOLFSSL_ALPN_CONTINUE_ON_MISMATCH) != SSL_SUCCESS) {
      failf(data, "SSL: failed setting ALPN protocols");
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
#endif /* HAVE_ALPN */

#ifdef OPENSSL_EXTRA
  if(Curl_tls_keylog_enabled()) {
    /* Ensure the Client Random is preserved. */
    wolfSSL_KeepArrays(backend->handle);
#if defined(HAVE_SECRET_CALLBACK) && defined(WOLFSSL_TLS13)
    wolfSSL_set_tls13_secret_cb(backend->handle,
                                wolfssl_tls13_secret_callback, NULL);
#endif
  }
#endif /* OPENSSL_EXTRA */

  /* Check if there's a cached ID we can/should use here! */
  if(SSL_SET_OPTION(primary.sessionid)) {
    void *ssl_sessionid = NULL;

    Curl_ssl_sessionid_lock(conn);
    if(!Curl_ssl_getsessionid(conn, &ssl_sessionid, NULL, sockindex)) {
      /* we got a session id, use it! */
      if(!SSL_set_session(backend->handle, ssl_sessionid)) {
        char error_buffer[WOLFSSL_MAX_ERROR_SZ];
        Curl_ssl_sessionid_unlock(conn);
        failf(data, "SSL: SSL_set_session failed: %s",
              ERR_error_string(SSL_get_error(backend->handle, 0),
                               error_buffer));
        return CURLE_SSL_CONNECT_ERROR;
      }
      /* Informational message */
      infof(data, "SSL re-using session ID\n");
    }
    Curl_ssl_sessionid_unlock(conn);
  }

  /* pass the raw socket into the SSL layer */
  if(!SSL_set_fd(backend->handle, (int)sockfd)) {
    failf(data, "SSL: SSL_set_fd failed");
    return CURLE_SSL_CONNECT_ERROR;
  }

  connssl->connecting_state = ssl_connect_2;
  return CURLE_OK;
}


static CURLcode
wolfssl_connect_step2(struct connectdata *conn,
                     int sockindex)
{
  int ret = -1;
  struct Curl_easy *data = conn->data;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
#ifndef CURL_DISABLE_PROXY
  const char * const hostname = SSL_IS_PROXY() ? conn->http_proxy.host.name :
    conn->host.name;
  const char * const dispname = SSL_IS_PROXY() ?
    conn->http_proxy.host.dispname : conn->host.dispname;
  const char * const pinnedpubkey = SSL_IS_PROXY() ?
    data->set.str[STRING_SSL_PINNEDPUBLICKEY_PROXY] :
    data->set.str[STRING_SSL_PINNEDPUBLICKEY_ORIG];
#else
  const char * const hostname = conn->host.name;
  const char * const dispname = conn->host.dispname;
  const char * const pinnedpubkey =
    data->set.str[STRING_SSL_PINNEDPUBLICKEY_ORIG];
#endif

  conn->recv[sockindex] = wolfssl_recv;
  conn->send[sockindex] = wolfssl_send;

  /* Enable RFC2818 checks */
  if(SSL_CONN_CONFIG(verifyhost)) {
    ret = wolfSSL_check_domain_name(backend->handle, hostname);
    if(ret == SSL_FAILURE)
      return CURLE_OUT_OF_MEMORY;
  }

  ret = SSL_connect(backend->handle);

#ifdef OPENSSL_EXTRA
  if(Curl_tls_keylog_enabled()) {
    /* If key logging is enabled, wait for the handshake to complete and then
     * proceed with logging secrets (for TLS 1.2 or older).
     *
     * During the handshake (ret==-1), wolfSSL_want_read() is true as it waits
     * for the server response. At that point the master secret is not yet
     * available, so we must not try to read it.
     * To log the secret on completion with a handshake failure, detect
     * completion via the observation that there is nothing to read or write.
     * Note that OpenSSL SSL_want_read() is always true here. If wolfSSL ever
     * changes, the worst case is that no key is logged on error.
     */
    if(ret == SSL_SUCCESS ||
       (!wolfSSL_want_read(backend->handle) &&
        !wolfSSL_want_write(backend->handle))) {
      wolfssl_log_tls12_secret(backend->handle);
      /* Client Random and master secrets are no longer needed, erase these.
       * Ignored while the handshake is still in progress. */
      wolfSSL_FreeArrays(backend->handle);
    }
  }
#endif  /* OPENSSL_EXTRA */

  if(ret != 1) {
    char error_buffer[WOLFSSL_MAX_ERROR_SZ];
    int  detail = SSL_get_error(backend->handle, ret);

    if(SSL_ERROR_WANT_READ == detail) {
      connssl->connecting_state = ssl_connect_2_reading;
      return CURLE_OK;
    }
    else if(SSL_ERROR_WANT_WRITE == detail) {
      connssl->connecting_state = ssl_connect_2_writing;
      return CURLE_OK;
    }
    /* There is no easy way to override only the CN matching.
     * This will enable the override of both mismatching SubjectAltNames
     * as also mismatching CN fields */
    else if(DOMAIN_NAME_MISMATCH == detail) {
#if 1
      failf(data, "\tsubject alt name(s) or common name do not match \"%s\"\n",
            dispname);
      return CURLE_PEER_FAILED_VERIFICATION;
#else
      /* When the wolfssl_check_domain_name() is used and you desire to
       * continue on a DOMAIN_NAME_MISMATCH, i.e. 'conn->ssl_config.verifyhost
       * == 0', CyaSSL version 2.4.0 will fail with an INCOMPLETE_DATA
       * error. The only way to do this is currently to switch the
       * Wolfssl_check_domain_name() in and out based on the
       * 'conn->ssl_config.verifyhost' value. */
      if(SSL_CONN_CONFIG(verifyhost)) {
        failf(data,
              "\tsubject alt name(s) or common name do not match \"%s\"\n",
              dispname);
        return CURLE_PEER_FAILED_VERIFICATION;
      }
      else {
        infof(data,
              "\tsubject alt name(s) and/or common name do not match \"%s\"\n",
              dispname);
        return CURLE_OK;
      }
#endif
    }
#if LIBWOLFSSL_VERSION_HEX >= 0x02007000 /* 2.7.0 */
    else if(ASN_NO_SIGNER_E == detail) {
      if(SSL_CONN_CONFIG(verifypeer)) {
        failf(data, "\tCA signer not available for verification\n");
        return CURLE_SSL_CACERT_BADFILE;
      }
      else {
        /* Just continue with a warning if no strict certificate
           verification is required. */
        infof(data, "CA signer not available for verification, "
                    "continuing anyway\n");
      }
    }
#endif
    else {
      failf(data, "SSL_connect failed with error %d: %s", detail,
          ERR_error_string(detail, error_buffer));
      return CURLE_SSL_CONNECT_ERROR;
    }
  }

  if(pinnedpubkey) {
#ifdef KEEP_PEER_CERT
    X509 *x509;
    const char *x509_der;
    int x509_der_len;
    struct Curl_X509certificate x509_parsed;
    struct Curl_asn1Element *pubkey;
    CURLcode result;

    x509 = SSL_get_peer_certificate(backend->handle);
    if(!x509) {
      failf(data, "SSL: failed retrieving server certificate");
      return CURLE_SSL_PINNEDPUBKEYNOTMATCH;
    }

    x509_der = (const char *)wolfSSL_X509_get_der(x509, &x509_der_len);
    if(!x509_der) {
      failf(data, "SSL: failed retrieving ASN.1 server certificate");
      return CURLE_SSL_PINNEDPUBKEYNOTMATCH;
    }

    memset(&x509_parsed, 0, sizeof(x509_parsed));
    if(Curl_parseX509(&x509_parsed, x509_der, x509_der + x509_der_len))
      return CURLE_SSL_PINNEDPUBKEYNOTMATCH;

    pubkey = &x509_parsed.subjectPublicKeyInfo;
    if(!pubkey->header || pubkey->end <= pubkey->header) {
      failf(data, "SSL: failed retrieving public key from server certificate");
      return CURLE_SSL_PINNEDPUBKEYNOTMATCH;
    }

    result = Curl_pin_peer_pubkey(data,
                                  pinnedpubkey,
                                  (const unsigned char *)pubkey->header,
                                  (size_t)(pubkey->end - pubkey->header));
    if(result) {
      failf(data, "SSL: public key does not match pinned public key!");
      return result;
    }
#else
    failf(data, "Library lacks pinning support built-in");
    return CURLE_NOT_BUILT_IN;
#endif
  }

#ifdef HAVE_ALPN
  if(conn->bits.tls_enable_alpn) {
    int rc;
    char *protocol = NULL;
    unsigned short protocol_len = 0;

    rc = wolfSSL_ALPN_GetProtocol(backend->handle, &protocol, &protocol_len);

    if(rc == SSL_SUCCESS) {
      infof(data, "ALPN, server accepted to use %.*s\n", protocol_len,
            protocol);

      if(protocol_len == ALPN_HTTP_1_1_LENGTH &&
         !memcmp(protocol, ALPN_HTTP_1_1, ALPN_HTTP_1_1_LENGTH))
        conn->negnpn = CURL_HTTP_VERSION_1_1;
#ifdef USE_NGHTTP2
      else if(data->set.httpversion >= CURL_HTTP_VERSION_2 &&
              protocol_len == NGHTTP2_PROTO_VERSION_ID_LEN &&
              !memcmp(protocol, NGHTTP2_PROTO_VERSION_ID,
                      NGHTTP2_PROTO_VERSION_ID_LEN))
        conn->negnpn = CURL_HTTP_VERSION_2;
#endif
      else
        infof(data, "ALPN, unrecognized protocol %.*s\n", protocol_len,
              protocol);
      Curl_multiuse_state(conn, conn->negnpn == CURL_HTTP_VERSION_2 ?
                          BUNDLE_MULTIPLEX : BUNDLE_NO_MULTIUSE);
    }
    else if(rc == SSL_ALPN_NOT_FOUND)
      infof(data, "ALPN, server did not agree to a protocol\n");
    else {
      failf(data, "ALPN, failure getting protocol, error %d", rc);
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
#endif /* HAVE_ALPN */

  connssl->connecting_state = ssl_connect_3;
#if (LIBWOLFSSL_VERSION_HEX >= 0x03009010)
  infof(data, "SSL connection using %s / %s\n",
        wolfSSL_get_version(backend->handle),
        wolfSSL_get_cipher_name(backend->handle));
#else
  infof(data, "SSL connected\n");
#endif

  return CURLE_OK;
}


static CURLcode
wolfssl_connect_step3(struct connectdata *conn,
                     int sockindex)
{
  CURLcode result = CURLE_OK;
  struct Curl_easy *data = conn->data;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;

  DEBUGASSERT(ssl_connect_3 == connssl->connecting_state);

  if(SSL_SET_OPTION(primary.sessionid)) {
    bool incache;
    SSL_SESSION *our_ssl_sessionid;
    void *old_ssl_sessionid = NULL;

    our_ssl_sessionid = SSL_get_session(backend->handle);

    Curl_ssl_sessionid_lock(conn);
    incache = !(Curl_ssl_getsessionid(conn, &old_ssl_sessionid, NULL,
                                      sockindex));
    if(incache) {
      if(old_ssl_sessionid != our_ssl_sessionid) {
        infof(data, "old SSL session ID is stale, removing\n");
        Curl_ssl_delsessionid(conn, old_ssl_sessionid);
        incache = FALSE;
      }
    }

    if(!incache) {
      result = Curl_ssl_addsessionid(conn, our_ssl_sessionid,
                                     0 /* unknown size */, sockindex);
      if(result) {
        Curl_ssl_sessionid_unlock(conn);
        failf(data, "failed to store ssl session");
        return result;
      }
    }
    Curl_ssl_sessionid_unlock(conn);
  }

  connssl->connecting_state = ssl_connect_done;

  return result;
}


static ssize_t wolfssl_send(struct connectdata *conn,
                           int sockindex,
                           const void *mem,
                           size_t len,
                           CURLcode *curlcode)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  char error_buffer[WOLFSSL_MAX_ERROR_SZ];
  int memlen = (len > (size_t)INT_MAX) ? INT_MAX : (int)len;
  int rc = SSL_write(backend->handle, mem, memlen);

  if(rc < 0) {
    int err = SSL_get_error(backend->handle, rc);

    switch(err) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      /* there's data pending, re-invoke SSL_write() */
      *curlcode = CURLE_AGAIN;
      return -1;
    default:
      failf(conn->data, "SSL write: %s, errno %d",
            ERR_error_string(err, error_buffer),
            SOCKERRNO);
      *curlcode = CURLE_SEND_ERROR;
      return -1;
    }
  }
  return rc;
}

static void Curl_wolfssl_close(struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;

  if(backend->handle) {
    (void)SSL_shutdown(backend->handle);
    SSL_free(backend->handle);
    backend->handle = NULL;
  }
  if(backend->ctx) {
    SSL_CTX_free(backend->ctx);
    backend->ctx = NULL;
  }
}

static ssize_t wolfssl_recv(struct connectdata *conn,
                            int num,
                            char *buf,
                            size_t buffersize,
                            CURLcode *curlcode)
{
  struct ssl_connect_data *connssl = &conn->ssl[num];
  struct ssl_backend_data *backend = connssl->backend;
  char error_buffer[WOLFSSL_MAX_ERROR_SZ];
  int buffsize = (buffersize > (size_t)INT_MAX) ? INT_MAX : (int)buffersize;
  int nread = SSL_read(backend->handle, buf, buffsize);

  if(nread < 0) {
    int err = SSL_get_error(backend->handle, nread);

    switch(err) {
    case SSL_ERROR_ZERO_RETURN: /* no more data */
      break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      /* there's data pending, re-invoke SSL_read() */
      *curlcode = CURLE_AGAIN;
      return -1;
    default:
      failf(conn->data, "SSL read: %s, errno %d",
            ERR_error_string(err, error_buffer),
            SOCKERRNO);
      *curlcode = CURLE_RECV_ERROR;
      return -1;
    }
  }
  return nread;
}


static void Curl_wolfssl_session_free(void *ptr)
{
  (void)ptr;
  /* wolfSSL reuses sessions on own, no free */
}


static size_t Curl_wolfssl_version(char *buffer, size_t size)
{
#if LIBWOLFSSL_VERSION_HEX >= 0x03006000
  return msnprintf(buffer, size, "wolfSSL/%s", wolfSSL_lib_version());
#elif defined(WOLFSSL_VERSION)
  return msnprintf(buffer, size, "wolfSSL/%s", WOLFSSL_VERSION);
#endif
}


static int Curl_wolfssl_init(void)
{
#ifdef OPENSSL_EXTRA
  Curl_tls_keylog_open();
#endif
  return (wolfSSL_Init() == SSL_SUCCESS);
}


static void Curl_wolfssl_cleanup(void)
{
  wolfSSL_Cleanup();
#ifdef OPENSSL_EXTRA
  Curl_tls_keylog_close();
#endif
}


static bool Curl_wolfssl_data_pending(const struct connectdata *conn,
                                      int connindex)
{
  const struct ssl_connect_data *connssl = &conn->ssl[connindex];
  struct ssl_backend_data *backend = connssl->backend;
  if(backend->handle)   /* SSL is in use */
    return (0 != SSL_pending(backend->handle)) ? TRUE : FALSE;
  else
    return FALSE;
}


/*
 * This function is called to shut down the SSL layer but keep the
 * socket open (CCC - Clear Command Channel)
 */
static int Curl_wolfssl_shutdown(struct connectdata *conn, int sockindex)
{
  int retval = 0;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;

  if(backend->handle) {
    SSL_free(backend->handle);
    backend->handle = NULL;
  }
  return retval;
}


static CURLcode
wolfssl_connect_common(struct connectdata *conn,
                      int sockindex,
                      bool nonblocking,
                      bool *done)
{
  CURLcode result;
  struct Curl_easy *data = conn->data;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  curl_socket_t sockfd = conn->sock[sockindex];
  int what;

  /* check if the connection has already been established */
  if(ssl_connection_complete == connssl->state) {
    *done = TRUE;
    return CURLE_OK;
  }

  if(ssl_connect_1 == connssl->connecting_state) {
    /* Find out how much more time we're allowed */
    const timediff_t timeout_ms = Curl_timeleft(data, NULL, TRUE);

    if(timeout_ms < 0) {
      /* no need to continue if time already is up */
      failf(data, "SSL connection timeout");
      return CURLE_OPERATION_TIMEDOUT;
    }

    result = wolfssl_connect_step1(conn, sockindex);
    if(result)
      return result;
  }

  while(ssl_connect_2 == connssl->connecting_state ||
        ssl_connect_2_reading == connssl->connecting_state ||
        ssl_connect_2_writing == connssl->connecting_state) {

    /* check allowed time left */
    const timediff_t timeout_ms = Curl_timeleft(data, NULL, TRUE);

    if(timeout_ms < 0) {
      /* no need to continue if time already is up */
      failf(data, "SSL connection timeout");
      return CURLE_OPERATION_TIMEDOUT;
    }

    /* if ssl is expecting something, check if it's available. */
    if(connssl->connecting_state == ssl_connect_2_reading
       || connssl->connecting_state == ssl_connect_2_writing) {

      curl_socket_t writefd = ssl_connect_2_writing ==
        connssl->connecting_state?sockfd:CURL_SOCKET_BAD;
      curl_socket_t readfd = ssl_connect_2_reading ==
        connssl->connecting_state?sockfd:CURL_SOCKET_BAD;

      what = Curl_socket_check(readfd, CURL_SOCKET_BAD, writefd,
                               nonblocking?0:timeout_ms);
      if(what < 0) {
        /* fatal error */
        failf(data, "select/poll on SSL socket, errno: %d", SOCKERRNO);
        return CURLE_SSL_CONNECT_ERROR;
      }
      else if(0 == what) {
        if(nonblocking) {
          *done = FALSE;
          return CURLE_OK;
        }
        else {
          /* timeout */
          failf(data, "SSL connection timeout");
          return CURLE_OPERATION_TIMEDOUT;
        }
      }
      /* socket is readable or writable */
    }

    /* Run transaction, and return to the caller if it failed or if
     * this connection is part of a multi handle and this loop would
     * execute again. This permits the owner of a multi handle to
     * abort a connection attempt before step2 has completed while
     * ensuring that a client using select() or epoll() will always
     * have a valid fdset to wait on.
     */
    result = wolfssl_connect_step2(conn, sockindex);
    if(result || (nonblocking &&
                  (ssl_connect_2 == connssl->connecting_state ||
                   ssl_connect_2_reading == connssl->connecting_state ||
                   ssl_connect_2_writing == connssl->connecting_state)))
      return result;
  } /* repeat step2 until all transactions are done. */

  if(ssl_connect_3 == connssl->connecting_state) {
    result = wolfssl_connect_step3(conn, sockindex);
    if(result)
      return result;
  }

  if(ssl_connect_done == connssl->connecting_state) {
    connssl->state = ssl_connection_complete;
    conn->recv[sockindex] = wolfssl_recv;
    conn->send[sockindex] = wolfssl_send;
    *done = TRUE;
  }
  else
    *done = FALSE;

  /* Reset our connect state machine */
  connssl->connecting_state = ssl_connect_1;

  return CURLE_OK;
}


static CURLcode Curl_wolfssl_connect_nonblocking(struct connectdata *conn,
                                                int sockindex, bool *done)
{
  return wolfssl_connect_common(conn, sockindex, TRUE, done);
}


static CURLcode Curl_wolfssl_connect(struct connectdata *conn, int sockindex)
{
  CURLcode result;
  bool done = FALSE;

  result = wolfssl_connect_common(conn, sockindex, FALSE, &done);
  if(result)
    return result;

  DEBUGASSERT(done);

  return CURLE_OK;
}

static CURLcode Curl_wolfssl_random(struct Curl_easy *data,
                                   unsigned char *entropy, size_t length)
{
  WC_RNG rng;
  (void)data;
  if(wc_InitRng(&rng))
    return CURLE_FAILED_INIT;
  if(length > UINT_MAX)
    return CURLE_FAILED_INIT;
  if(wc_RNG_GenerateBlock(&rng, entropy, (unsigned)length))
    return CURLE_FAILED_INIT;
  if(wc_FreeRng(&rng))
    return CURLE_FAILED_INIT;
  return CURLE_OK;
}

static CURLcode Curl_wolfssl_sha256sum(const unsigned char *tmp, /* input */
                                       size_t tmplen,
                                       unsigned char *sha256sum /* output */,
                                       size_t unused)
{
  wc_Sha256 SHA256pw;
  (void)unused;
  wc_InitSha256(&SHA256pw);
  wc_Sha256Update(&SHA256pw, tmp, (word32)tmplen);
  wc_Sha256Final(&SHA256pw, sha256sum);
  return CURLE_OK;
}

static void *Curl_wolfssl_get_internals(struct ssl_connect_data *connssl,
                                        CURLINFO info UNUSED_PARAM)
{
  struct ssl_backend_data *backend = connssl->backend;
  (void)info;
  return backend->handle;
}

const struct Curl_ssl Curl_ssl_wolfssl = {
  { CURLSSLBACKEND_WOLFSSL, "WolfSSL" }, /* info */

#ifdef KEEP_PEER_CERT
  SSLSUPP_PINNEDPUBKEY |
#endif
  SSLSUPP_SSL_CTX,

  sizeof(struct ssl_backend_data),

  Curl_wolfssl_init,                /* init */
  Curl_wolfssl_cleanup,             /* cleanup */
  Curl_wolfssl_version,             /* version */
  Curl_none_check_cxn,             /* check_cxn */
  Curl_wolfssl_shutdown,            /* shutdown */
  Curl_wolfssl_data_pending,        /* data_pending */
  Curl_wolfssl_random,              /* random */
  Curl_none_cert_status_request,   /* cert_status_request */
  Curl_wolfssl_connect,             /* connect */
  Curl_wolfssl_connect_nonblocking, /* connect_nonblocking */
  Curl_wolfssl_get_internals,       /* get_internals */
  Curl_wolfssl_close,               /* close_one */
  Curl_none_close_all,             /* close_all */
  Curl_wolfssl_session_free,        /* session_free */
  Curl_none_set_engine,            /* set_engine */
  Curl_none_set_engine_default,    /* set_engine_default */
  Curl_none_engines_list,          /* engines_list */
  Curl_none_false_start,           /* false_start */
  Curl_none_md5sum,                /* md5sum */
  Curl_wolfssl_sha256sum            /* sha256sum */
};

#endif
