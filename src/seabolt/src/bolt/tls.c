/*
 * Copyright (c) 2002-2018 "Neo4j,"
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
#include "config-private.h"
#include "log-private.h"

int verify_callback(int preverify_ok, X509_STORE_CTX* ctx)
{
    // get related BoltTrust and BoltLog instances
    SSL* ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    SSL_CTX* context = SSL_get_SSL_CTX(ssl);
    struct BoltTrust* trust = (struct BoltTrust*) SSL_CTX_get_ex_data(context, SSL_CTX_TRUST_INDEX);
    struct BoltLog* log = (struct BoltLog*) SSL_CTX_get_ex_data(context, SSL_CTX_LOG_INDEX);
    char* id = (char*) SSL_CTX_get_ex_data(context, SSL_CTX_ID_INDEX);

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

SSL_CTX* create_ssl_ctx(struct BoltTrust* trust, const char* hostname, const struct BoltLog* log, const char* id)
{
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
                BIO_write(trusted_certs_bio, trust->certs, (int32_t)trust->certs_len);

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
    SSL_CTX_set_ex_data(context, SSL_CTX_ID_INDEX, (void*) id);

    // Enable hostname verification
    X509_VERIFY_PARAM* param = SSL_CTX_get0_param(context);
    X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    X509_VERIFY_PARAM_set1_host(param, hostname, strlen(hostname));

    // Enable verification and set verify callback
    SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);

    // cleanup context if anything went wrong
    if (!status && context!=NULL) {
        SSL_CTX_free(context);
    }

    // cleanup store if anything went wrong
    if (!status && store!=NULL) {
        X509_STORE_free(store);
    }

    return context;
}

void free_ssl_context(SSL_CTX* ctx)
{
    SSL_CTX_free(ctx);
}

#if USE_WINSOCK && defined(SEABOLT_STATIC_DEFINE) && defined(_MSC_VER)
FILE * __cdecl __iob_func(void)
{
	FILE* _iob = (FILE*)malloc(3 * sizeof(FILE));
	_iob[0] = *stdin;
	_iob[1] = *stdout;
	_iob[2] = *stderr;
	return _iob;
}
#endif
