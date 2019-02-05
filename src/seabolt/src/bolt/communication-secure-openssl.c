/*
 * Copyright (c) 2002-2019 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
 *
 * This file is part of Neo4j.
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

#include "bolt-private.h"
#include "communication-secure.h"
#include "config-private.h"
#include "log-private.h"
#include "status-private.h"
#include "mem.h"
#include "communication-plain.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

typedef struct BoltSecurityContext {
    SSL_CTX* ssl_ctx;
} BoltSecurityContext;

typedef struct OpenSSLContext {
    int owns_sec_ctx;
    BoltSecurityContext* sec_ctx;

    char* id;
    char* hostname;
    SSL* ssl;

    BoltTrust* trust;
    BoltCommunication* plain_comm;
} OpenSSLContext;

int SSL_CTX_TRUST_INDEX = -1;
int SSL_CTX_LOG_INDEX = -1;
int SSL_ID_INDEX = -1;

int verify_callback(int preverify_ok, X509_STORE_CTX* ctx)
{
    // get related BoltTrust and BoltLog instances
    SSL* ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    SSL_CTX* context = SSL_get_SSL_CTX(ssl);
    struct BoltTrust* trust = (struct BoltTrust*) SSL_CTX_get_ex_data(context, SSL_CTX_TRUST_INDEX);
    struct BoltLog* log = (struct BoltLog*) SSL_CTX_get_ex_data(context, SSL_CTX_LOG_INDEX);
    char* id = (char*) SSL_get_ex_data(ssl, SSL_ID_INDEX);
    //char* id = (char*) SSL_CTX_get_ex_data(context, SSL_CTX_ID_INDEX);

    // check if preverify is successful or not
    if (!preverify_ok) {
        // get the error code, bear in mind that we'll have this callback called twice if both X509
        // and hostname verification failed
        int error = X509_STORE_CTX_get_error(ctx);

        switch (error) {
        case X509_V_ERR_HOSTNAME_MISMATCH:
            if (trust!=NULL && trust->skip_verify_hostname) {
                BoltLog_warning(log,
                        "[%s]: Openssl reported failure of hostname verification due to a mismatch, but resuming handshake since hostname verification is set to be skipped",
                        id);
                return 1;
            }
            else {
                BoltLog_debug(log,
                        "[%s]: Openssl reported failure of hostname verification due to a mismatch, aborting handshake",
                        id);
                return 0;
            }
        default:
            if (trust!=NULL && trust->skip_verify) {
                BoltLog_warning(log,
                        "[%s]: Openssl reported error '%s' with code '%d' when establishing trust, but resuming handshake since trust verification is set to be skipped",
                        id, X509_verify_cert_error_string(error), error);
                return 1;
            }
            else {
                BoltLog_debug(log,
                        "[%s]: Openssl reported error '%s' with code '%d' when establishing trust, aborting handshake",
                        id, X509_verify_cert_error_string(error), error);
                return 0;
            }
        }
    }
    else {
        BoltLog_debug(log, "[%s]: Openssl established trust", id);
    }

    return 1;
}

BoltSecurityContext*
BoltSecurityContext_create(struct BoltTrust* trust, const char* hostname, const struct BoltLog* log, const char* id)
{
    UNUSED(id);

    SSL_CTX* context = NULL;
    X509_STORE* store = NULL;
    const SSL_METHOD* ctx_init_method = NULL;

#if OPENSSL_VERSION_NUMBER<0x10100000L
    ctx_init_method = TLSv1_2_client_method();
#else
    ctx_init_method = TLS_client_method();
#endif

    // create a new SSL_CTX with TLS1.2 enabled
    context = SSL_CTX_new(ctx_init_method);
    if (context==NULL) {
        return NULL;
    }

    // load trusted certificates
    int status = 1;
    if (trust!=NULL && trust->certs!=NULL && trust->certs_len!=0) {
        // create a new x509 certificate store
        store = X509_STORE_new();
        if (store!=NULL) {
            // add default trust certs to the store, which are pointed by
            // SSL_CERT_DIR and SSL_CERT_FILE, see more on OPENSSL docs
            status = X509_STORE_set_default_paths(store);
        }

        if (status) {
            // if an explicit PEM encoded certs are provided as part of config
            if (trust->certs!=NULL && trust->certs_len!=0) {
                // load the buffer into a BIO memory reader
                BIO* trusted_certs_bio = BIO_new(BIO_s_mem());
                BIO_write(trusted_certs_bio, trust->certs, (int32_t) trust->certs_len);

                // read all X509_AUX objects encoded (_AUX suffix tells that these
                // certificates are to be treated as trusted
                X509* trusted_cert = PEM_read_bio_X509_AUX(trusted_certs_bio, NULL, NULL, NULL);
                while (trusted_cert!=NULL) {
                    X509_STORE_add_cert(store, trusted_cert);

                    trusted_cert = PEM_read_bio_X509_AUX(trusted_certs_bio, NULL, NULL, NULL);
                }

                // free the BIO reader
                BIO_free(trusted_certs_bio);
            }
        }

        // set trusted certificate store on the context
        status = SSL_CTX_set1_verify_cert_store(context, store);
    }
    else {
        // set trusted certificates from the defaults, which are pointed by
        // SSL_CERT_DIR and SSL_CERT_FILE, see more on OPENSSL docs
        status = SSL_CTX_set_default_verify_paths(context);
    }

    // Store BoltTrust and BoltLog objects into the context to be used in verification callback
    SSL_CTX_set_ex_data(context, SSL_CTX_TRUST_INDEX, trust);
    SSL_CTX_set_ex_data(context, SSL_CTX_LOG_INDEX, (void*) log);

    // Enable hostname verification
    X509_VERIFY_PARAM* param = SSL_CTX_get0_param(context);
    X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    X509_VERIFY_PARAM_set1_host(param, hostname, strlen(hostname));

    // Enable verification and set verify callback
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);

    // cleanup context if anything went wrong
    if (!status && context!=NULL) {
        SSL_CTX_free(context);
        context = NULL;
    }

    // cleanup store if anything went wrong
    if (!status && store!=NULL) {
        X509_STORE_free(store);
        store = NULL;
    }

    if (context!=NULL) {
        BoltSecurityContext* secContext = BoltMem_allocate(sizeof(BoltSecurityContext));
        secContext->ssl_ctx = context;
        return secContext;
    }

    return NULL;
}

void BoltSecurityContext_destroy(BoltSecurityContext* context)
{
    SSL_CTX_free(context->ssl_ctx);
    BoltMem_deallocate(context, sizeof(BoltSecurityContext));
}

int BoltSecurityContext_startup()
{
#if OPENSSL_VERSION_NUMBER<0x10100000L
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif
    // load error strings and have cryto initialized
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // register two ex_data indexes that we'll use to store
    // BoltTrust and BoltLog instances
    SSL_CTX_TRUST_INDEX = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    SSL_CTX_LOG_INDEX = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    SSL_ID_INDEX = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);

    return BOLT_SUCCESS;
}

int BoltSecurityContext_shutdown()
{
    return BOLT_SUCCESS;
}

int secure_openssl_last_error(BoltCommunication* comm, int ssl_ret, int* ssl_error_code, int* last_error)
{
    OpenSSLContext* ctx = comm->context;

    // On windows, SSL_get_error resets WSAGetLastError so we're left without an error code after
    // asking error code - so we're saving it here in case.
    int last_error_saved = ctx->plain_comm->last_error(ctx->plain_comm);
    *ssl_error_code = SSL_get_error(ctx->ssl, ssl_ret);
    switch (*ssl_error_code) {
    case SSL_ERROR_NONE:
        return BOLT_SUCCESS;
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE: {
        *last_error = ctx->plain_comm->last_error(ctx->plain_comm);
        if (*last_error==0) {
            *last_error = last_error_saved;
        }
        return ctx->plain_comm->transform_error(ctx->plain_comm, *last_error);
    }
    default:
        return BOLT_TLS_ERROR;
    }
}

int secure_openssl_open(BoltCommunication* comm, const struct sockaddr_storage* address)
{
    OpenSSLContext* ctx = comm->context;
    PlainCommunicationContext* ctx_plain = ctx->plain_comm->context;

    int status = ctx->plain_comm->open(ctx->plain_comm, address);
    if (status!=BOLT_SUCCESS) {
        return status;
    }

    if (ctx->sec_ctx==NULL) {
        ctx->sec_ctx = BoltSecurityContext_create(ctx->trust, ctx->hostname, comm->log);
        ctx->owns_sec_ctx = 1;
    }

    ctx->ssl = SSL_new(ctx->sec_ctx->ssl_ctx);
    SSL_set_ex_data(ctx->ssl, SSL_ID_INDEX, (void*) ctx->id);

    status = 1;
    // Link to underlying socket
    if (status) {
        status = SSL_set_fd(ctx->ssl, ctx_plain->fd_socket);

        if (!status) {
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_openssl_open(%s:%d), SSL_set_fd returned: %d", __FILE__, __LINE__, status);
        }
    }

    // Enable SNI
    if (status) {
        status = SSL_set_tlsext_host_name(ctx->ssl, ctx->hostname);

        if (!status) {
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_openssl_open(%s:%d), SSL_set_tlsext_host_name returned: %d", __FILE__, __LINE__, status);

        }
    }

    if (status) {
        status = SSL_connect(ctx->ssl);
        if (status==1) {
            return BOLT_SUCCESS;
        }

        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_openssl_open(%s:%d), SSL_connect returned: %d", __FILE__, __LINE__, status);
    }

    SSL_free(ctx->ssl);
    if (ctx->owns_sec_ctx) {
        BoltSecurityContext_destroy(ctx->sec_ctx);
    }
    ctx->owns_sec_ctx = 0;
    ctx->sec_ctx = NULL;
    ctx->ssl = NULL;

    return comm->status->error;
}

int secure_openssl_close(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;

    if (ctx->ssl!=NULL) {
        SSL_shutdown(ctx->ssl);
        SSL_free(ctx->ssl);
        ctx->ssl = NULL;
    }
    if (ctx->sec_ctx!=NULL && ctx->owns_sec_ctx) {
        BoltSecurityContext_destroy(ctx->sec_ctx);
        ctx->sec_ctx = NULL;
        ctx->owns_sec_ctx = 0;
    }

    return ctx->plain_comm->close(ctx->plain_comm);
}

int secure_openssl_send(BoltCommunication* comm, char* buffer, int length, int* sent)
{
    OpenSSLContext* ctx = comm->context;

    int bytes = SSL_write(ctx->ssl, buffer, length);
    if (bytes<0) {
        int last_error = 0, ssl_error = 0;
        int last_error_transformed = secure_openssl_last_error(comm, bytes, &ssl_error, &last_error);
        BoltStatus_set_error_with_ctx(comm->status, last_error_transformed,
                "secure_openssl_send(%s:%d), SSL_write error code: %d, underlying error code: %d", __FILE__, __LINE__,
                ssl_error, last_error);
        return BOLT_STATUS_SET;
    }

    *sent = bytes;

    return BOLT_SUCCESS;
}

int secure_openssl_recv(BoltCommunication* comm, char* buffer, int length, int* received)
{
    OpenSSLContext* ctx = comm->context;

    int bytes = SSL_read(ctx->ssl, buffer, length);
    if (bytes<=0) {
        int last_error = 0, ssl_error = 0;
        int last_error_transformed = secure_openssl_last_error(comm, bytes, &ssl_error, &last_error);
        BoltStatus_set_error_with_ctx(comm->status, last_error_transformed,
                "secure_openssl_recv(%s:%d), SSL_read error code: %d, underlying error code: %d", __FILE__, __LINE__,
                ssl_error, last_error);
        return BOLT_STATUS_SET;
    }

    *received = bytes;

    return BOLT_SUCCESS;
}

int secure_openssl_destroy(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;

    if (ctx!=NULL) {
        BoltCommunication_destroy(ctx->plain_comm);

        BoltMem_deallocate(ctx->hostname, strlen(ctx->hostname));
        BoltMem_deallocate(ctx->id, strlen(ctx->id));

        BoltMem_deallocate(ctx, sizeof(OpenSSLContext));
        comm->context = NULL;
    }

    return BOLT_SUCCESS;
}

BoltAddress* secure_openssl_local_endpoint(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;
    return ctx->plain_comm->get_local_endpoint(ctx->plain_comm);
}

BoltAddress* secure_openssl_remote_endpoint(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;
    return ctx->plain_comm->get_remote_endpoint(ctx->plain_comm);
}

int secure_openssl_ignore_sigpipe(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;
    return ctx->plain_comm->ignore_sigpipe(ctx->plain_comm);
}

int secure_openssl_restore_sigpipe(BoltCommunication* comm)
{
    OpenSSLContext* ctx = comm->context;
    return ctx->plain_comm->restore_sigpipe(ctx->plain_comm);
}

BoltCommunication* BoltCommunication_create_secure(BoltSecurityContext* sec_ctx, BoltTrust* trust,
        BoltSocketOptions* socket_options, BoltLog* log, const char* hostname, const char* id)
{
    BoltCommunication* plain_comm = BoltCommunication_create_plain(socket_options, log);

    BoltCommunication* comm = BoltMem_allocate(sizeof(BoltCommunication));
    comm->open = &secure_openssl_open;
    comm->close = &secure_openssl_close;
    comm->send = &secure_openssl_send;
    comm->recv = &secure_openssl_recv;
    comm->destroy = &secure_openssl_destroy;

    comm->get_local_endpoint = &secure_openssl_local_endpoint;
    comm->get_remote_endpoint = &secure_openssl_remote_endpoint;

    comm->ignore_sigpipe = &secure_openssl_ignore_sigpipe;
    comm->restore_sigpipe = &secure_openssl_restore_sigpipe;

    comm->status_owned = 0;
    comm->status = plain_comm->status;
    comm->sock_opts_owned = 0;
    comm->sock_opts = plain_comm->sock_opts;
    comm->log = BoltLog_clone(log);

    OpenSSLContext* context = BoltMem_allocate(sizeof(OpenSSLContext));
    context->sec_ctx = sec_ctx;
    context->owns_sec_ctx = sec_ctx==NULL;
    context->ssl = NULL;
    context->trust = trust;
    context->plain_comm = plain_comm;
    context->id = BoltMem_duplicate(id, strlen(id)+1);
    context->hostname = BoltMem_duplicate(hostname, strlen(hostname)+1);
    comm->context = context;

    return comm;
}
