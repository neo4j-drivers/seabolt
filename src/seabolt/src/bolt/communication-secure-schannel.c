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

#include <security.h>
#include <sspi.h>
#include <schnlsp.h>

#ifndef SP_PROT_TLS1_2_CLIENT
#define SP_PROT_TLS1_2_CLIENT   0x00000800
#endif

#ifndef SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3_CLIENT   0x00002000
#endif

#ifndef SCH_SEND_AUX_RECORD
#define SCH_SEND_AUX_RECORD     0x00200000
#endif

#ifndef SCH_USE_STRONG_CRYPTO
#define SCH_USE_STRONG_CRYPTO   0x00400000
#endif

#ifndef SECURITY_FLAG_IGNORE_CERT_CN_INVALID
#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID    0x00001000
#endif

#define PEM_MARKER "-----BEGIN"

typedef struct BoltSecurityContext {
    const BoltLog* log;
    CredHandle* cred_handle;
    HCERTCHAINENGINE* cert_engine;
    HCERTSTORE* root_store;
    HCERTSTORE* trust_store;
} BoltSecurityContext;

typedef struct SChannelContext {
    int owns_sec_ctx;
    BoltSecurityContext* sec_ctx;

    char* id;
    char* hostname;

    CtxtHandle* context_handle;
    SecPkgContext_StreamSizes* stream_sizes;
    char* send_buffer;
    int send_buffer_length;

    char* recv_buffer;
    DWORD recv_buffer_length;
    DWORD recv_buffer_pos;

    char* hs_buffer;
    DWORD hs_buffer_length;
    DWORD hs_buffer_pos;

    char* pt_pending_buffer;
    DWORD pt_pending_buffer_length;
    DWORD pt_pending_buffer_pos;

    char* ct_pending_buffer;
    DWORD ct_pending_buffer_length;

    BoltTrust* trust;
    BoltCommunication* plain_comm;
} SChannelContext;

typedef void (* logger_func)(const struct BoltLog* log, const char* format, ...);

const char* secure_schannel_trust_error(DWORD status)
{
    switch (status) {
    case TRUST_E_CERT_SIGNATURE:
        return "The signature of the certificate cannot be verified.";
    case CRYPT_E_REVOKED:
        return "The certificate or signature has been revoked.";
    case CERT_E_UNTRUSTEDROOT:
        return "A certification chain processed correctly but terminated in a root certificate that is not trusted by the trust provider.";
    case CERT_E_UNTRUSTEDTESTROOT:
        return "The root certificate is a testing certificate, and policy settings disallow test certificates.";
    case CERT_E_CHAINING:
        return "A chain of certificates was not correctly created.";
    case CERT_E_WRONG_USAGE:
        return "The certificate is not valid for the requested usage.";
    case CERT_E_EXPIRED:
        return "A required certificate is not within its validity period.";
    case CERT_E_INVALID_NAME:
        return "The certificate has an invalid name. Either the name is not included in the permitted list, or it is explicitly excluded.";
    case CERT_E_INVALID_POLICY:
        return "The certificate has an invalid policy.";
    case TRUST_E_BASIC_CONSTRAINTS:
        return "The basic constraints of the certificate are not valid, or they are missing.";
    case CERT_E_CRITICAL:
        return "The certificate is being used for a purpose other than the purpose specified by its CA.";
    case CERT_E_VALIDITYPERIODNESTING:
        return "The validity periods of the certification chain do not nest correctly.";
    case CRYPT_E_NO_REVOCATION_CHECK:
        return "The revocation function was unable to check revocation for the certificate.";
    case CRYPT_E_REVOCATION_OFFLINE:
        return "The revocation function was unable to check revocation because the revocation server was offline. ";
    case CERT_E_PURPOSE:
        return "The certificate is being used for a purpose other than one specified by the issuing CA.";
    case CERT_E_REVOKED:
        return "The certificate has been explicitly revoked by the issuer.";
    case CERT_E_REVOCATION_FAILURE:
        return "The revocation process could not continue, and the certificate could not be checked. ";
    case CERT_E_CN_NO_MATCH:
        return "The certificate's CN name does not match the passed value.";
    case CERT_E_ROLE:
        return "A certificate that can only be used as an end-entity is being used as a CA or vice versa. ";
    default:
        return "An unknown error.";
    }
}

const char* secure_schannel_status_error(DWORD status)
{
    switch (status) {
    case SEC_E_INSUFFICIENT_MEMORY:
        return "There is not enough memory available to complete the requested action.";
    case SEC_E_INTERNAL_ERROR:
        return "An error occurred that did not map to an SSPI error code.";
    case SEC_E_INVALID_HANDLE:
        return "The handle passed to the function is not valid.";
    case SEC_E_INVALID_TOKEN:
        return "The error is due to a malformed input token, such as a token corrupted in transit, a token of incorrect size, or a token passed into the wrong security package. Passing a token to the wrong package can happen if the client and server did not negotiate the proper security package.";
    case SEC_E_LOGON_DENIED:
        return "The logon failed.";
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        return "No authority could be contacted for authentication. The domain name of the authenticating party could be wrong, the domain could be unreachable, or there might have been a trust relationship failure.";
    case SEC_E_NO_CREDENTIALS:
        return "No credentials are available in the security package.";
    case SEC_E_TARGET_UNKNOWN:
        return "The target was not recognized.";
    case SEC_E_WRONG_PRINCIPAL:
        return "The principal that received the authentication request is not the same as the one passed into the pszTargetName parameter. This indicates a failure in mutual authentication. ";
    case SEC_E_NOT_OWNER:
        return "The caller of the function does not have the necessary credentials.";
    case SEC_E_SECPKG_NOT_FOUND:
        return "The requested security package does not exist.";
    case SEC_E_UNKNOWN_CREDENTIALS:
        return "The credentials supplied to the package were not recognized. ";
    case SEC_E_UNSUPPORTED_FUNCTION:
        return "The function requested is not supported.";
    case SEC_E_BUFFER_TOO_SMALL:
        return "The message buffer is too small.";
    case SEC_E_CRYPTO_SYSTEM_INVALID:
        return "The cipher chosen for the security context is not supported.";
    case SEC_E_INCOMPLETE_MESSAGE:
        return "The data in the input buffer is incomplete. The application needs to read more data from the server and call DecryptMessage (Digest) again.";
    case SEC_E_MESSAGE_ALTERED:
        return "The message has been altered. Used with the Digest SSP.";
    case SEC_E_OUT_OF_SEQUENCE:
        return "The message was not received in the correct sequence.";
    case SEC_E_QOP_NOT_SUPPORTED:
        return "Neither confidentiality nor integrity are supported by the security context.";
    case SEC_E_CONTEXT_EXPIRED:
        return "The application is referencing a context that has already been closed. A properly written application should not receive this error.";
    default:
        return "An unclassified security status is returned.";
    }
}

void
log_with_last_error(const struct BoltLog* log, logger_func func, const DWORD error_code, const char* format_msg,
        const char* id)
{
    LPSTR error_msg = NULL;
    DWORD error_msg_length = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &error_msg, 0, NULL);
    if (error_msg_length==0) {
        error_msg = "Unable retrieve OS-specific error message";
    }

    func(log, format_msg, id, error_code, error_msg);

    if (error_msg!=NULL) {
        LocalFree(error_msg);
    }
}

void
log_with_sec_stat(const struct BoltLog* log, logger_func func, const SECURITY_STATUS status,
        const char* format_msg, const char* id)
{
    func(log, format_msg, id, status, secure_schannel_status_error(status));
}

void
log_with_trust_error(const struct BoltLog* log, logger_func func, const DWORD trust_status, const char* format_msg,
        const char* id)
{
    func(log, format_msg, id, trust_status, secure_schannel_trust_error(trust_status));
}

/**
 * Loads certificates from a sequence of PEM-encoded certificates. Returns BOLT_SUCCESS on success and BOLT_TLS_ERROR
 * on failure.
 *
 * Self-signed certificates are placed into an in-memory store named root_store and other certificates are placed into
 * trust_store. Returned store handles should be closed by the user.
 *
 * @param trust
 * @param log
 * @param id
 * @param root_store
 * @param trust_store
 * @return
 */
int secure_schannel_load_certs(struct BoltTrust* trust, const struct BoltLog* log, const char* id,
        HCERTSTORE* root_store, HCERTSTORE* trust_store)
{
    int result = BOLT_SUCCESS;

    BYTE* binary = NULL;
    DWORD binary_size = 0;
    PCCERT_CONTEXT cert = NULL;

    *root_store = NULL;
    *trust_store = NULL;

    // Create a certificate store to hold trusted root certificates
    *root_store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, NULL);
    if (*root_store==NULL) {
        log_with_last_error(log, &BoltLog_error, GetLastError(),
                "[%s]: Unable to create an in-memory certificate store: CertOpenStore returned 0x%x: '%s'", id);
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // Create a certificate store to hold other trusted peer or intermediate CA certificates.
    *trust_store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, NULL);
    if (*trust_store==NULL) {
        log_with_last_error(log, &BoltLog_error, GetLastError(),
                "[%s]: Unable to create an in-memory certificate store: CertOpenStore returned 0x%x: '%s'", id);
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // Find the first instance of PEM encoded block
    char* pem = strstr(trust->certs, PEM_MARKER);
    while (pem!=NULL) {
        DWORD pem_length = (DWORD) (trust->certs_len-(pem-trust->certs));

        // Decode PEM-encoded string into its binary form
        if (!CryptStringToBinary(pem, pem_length, CRYPT_STRING_ANY, NULL, &binary_size, NULL, NULL)) {
            log_with_last_error(log, &BoltLog_error, GetLastError(),
                    "[%s]: Unable to decode PEM encoded string: CryptStringToBinary returned 0x%x: '%s", id);
            result = BOLT_TLS_ERROR;
            goto cleanup;
        }

        binary = BoltMem_allocate(binary_size);
        if (!CryptStringToBinary(pem, pem_length, CRYPT_STRING_ANY, binary, &binary_size, NULL, NULL)) {
            log_with_last_error(log, &BoltLog_error, GetLastError(),
                    "[%s]: Unable to decode PEM encoded string: CryptStringToBinary returned 0x%x: '%s'", id);
            result = BOLT_TLS_ERROR;
            goto cleanup;
        }

        // Parse the PEM-encoded certificate
        cert = CertCreateCertificateContext(X509_ASN_ENCODING, binary, binary_size);
        if (cert==NULL) {
            log_with_last_error(log, &BoltLog_error, GetLastError(),
                    "[%s]: Unable to create certificate context: CertCreateCertificateContext returned 0x%x: '%s'", id);
            result = BOLT_TLS_ERROR;
            goto cleanup;
        }

        // Decide which store should the certificate to be placed
        HCERTSTORE* target_store = NULL;
        if (CertCompareCertificateName(X509_ASN_ENCODING, &cert->pCertInfo->Subject, &cert->pCertInfo->Issuer)) {
            target_store = root_store;
        }
        else {
            target_store = trust_store;
        }

        // Add it to the target store
        if (!CertAddCertificateContextToStore(*target_store, cert, CERT_STORE_ADD_ALWAYS, NULL)) {
            log_with_last_error(log, &BoltLog_error, GetLastError(),
                    "[%s]: Unable to add certificate to store: CertAddCertificateContextToStore returned 0x%x: '%s",
                    id);
            result = BOLT_TLS_ERROR;
            goto cleanup;
        }

        // Free allocated certificate
        CertFreeCertificateContext(cert);
        cert = NULL;

        // Free buffer
        BoltMem_deallocate(binary, binary_size);
        binary = NULL;
        binary_size = 0;

        // Find the next instance of PEM encoded block
        pem = strstr(pem+1, PEM_MARKER);
    }

    cleanup:
    if (cert!=NULL) {
        CertFreeCertificateContext(cert);
    }

    if (binary!=NULL) {
        BoltMem_deallocate(binary, binary_size);
    }

    if (result!=BOLT_SUCCESS) {
        if (*root_store!=NULL) {
            CertCloseStore(*root_store, CERT_CLOSE_STORE_FORCE_FLAG);
        }

        if (*trust_store!=NULL) {
            CertCloseStore(*trust_store, CERT_CLOSE_STORE_FORCE_FLAG);
        }
    }

    return result;
}

BoltSecurityContext*
BoltSecurityContext_create(struct BoltTrust* trust, const char* hostname, const struct BoltLog* log, const char* id)
{
    UNUSED(trust);
    UNUSED(hostname);

    CredHandle* handle = BoltMem_allocate(sizeof(CredHandle));
    TimeStamp lifetime;

    SCHANNEL_CRED credData;
    ZeroMemory(&credData, sizeof(credData));
    credData.dwVersion = SCHANNEL_CRED_VERSION;
    credData.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT;
    credData.dwFlags = SCH_SEND_AUX_RECORD | SCH_USE_STRONG_CRYPTO;

    // we explicitly disable automatic certificate validation
    credData.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

    SECURITY_STATUS status = AcquireCredentialsHandle(NULL, UNISP_NAME, SECPKG_CRED_OUTBOUND, NULL, &credData, NULL,
            NULL, handle, &lifetime);
    if (status!=SEC_E_OK) {
        log_with_sec_stat(log, &BoltLog_error, status,
                "[%s]: Unable to initialise security context: AcquireCredentialsHandle returned 0x%x: '%s'", id);
        return NULL;
    }

    HCERTSTORE root_store = NULL, trust_store = NULL;
    HCERTCHAINENGINE cert_engine = 0;
    if (trust!=NULL && trust->certs!=NULL && trust->certs_len>0) {
        int result = secure_schannel_load_certs(trust, log, id, &root_store, &trust_store);
        if (result!=BOLT_SUCCESS) {
            return NULL;
        }

        CERT_CHAIN_ENGINE_CONFIG cert_engine_config;
        memset(&cert_engine_config, 0, sizeof(cert_engine_config));
        cert_engine_config.cbSize = sizeof(CERT_CHAIN_ENGINE_CONFIG);
        cert_engine_config.hRestrictedRoot = NULL;
        cert_engine_config.hRestrictedTrust = NULL;
        cert_engine_config.hRestrictedOther = NULL;
        cert_engine_config.cAdditionalStore = 0;
        cert_engine_config.rghAdditionalStore = NULL;
        cert_engine_config.dwFlags = 0;
        cert_engine_config.dwUrlRetrievalTimeout = 0;
        cert_engine_config.MaximumCachedCertificates = 0;
        cert_engine_config.CycleDetectionModulus = 0;
        cert_engine_config.hExclusiveRoot = root_store;
        cert_engine_config.hExclusiveTrustedPeople = trust_store;

        if (!CertCreateCertificateChainEngine(&cert_engine_config, &cert_engine)) {
            log_with_last_error(log, &BoltLog_error, GetLastError(),
                    "Unable to create chain engine: CertCreateCertificateEngine returned 0x%x: '%s'", id);
            return NULL;
        }
    }

    BoltSecurityContext* context = BoltMem_allocate(sizeof(BoltSecurityContext));
    context->log = log;
    context->cred_handle = handle;
    context->cert_engine = cert_engine;
    context->root_store = root_store;
    context->trust_store = trust_store;
    return context;
}

void BoltSecurityContext_destroy(BoltSecurityContext* context)
{
    if (context!=NULL) {
        if (context->cred_handle!=NULL) {
            SECURITY_STATUS status = FreeCredentialsHandle(context->cred_handle);
            if (status!=SEC_E_OK) {
                BoltLog_warning(context->log, "Unable to destroy security context: FreeCredentialsHandle returned %d",
                        status);
            }
            BoltMem_deallocate(context->cred_handle, sizeof(CredHandle));
            context->cred_handle = NULL;
        }

        if (context->cert_engine!=NULL) {
            CertFreeCertificateChainEngine(context->cert_engine);
            context->cert_engine = NULL;
        }

        if (context->root_store!=NULL) {
            if (!CertCloseStore(context->root_store, CERT_CLOSE_STORE_CHECK_FLAG)) {
                BoltLog_warning(context->log, "Unable to close custom root store: CertCloseStore returned: 0x%x",
                        GetLastError());
            }
            context->root_store = NULL;
        }

        if (context->trust_store!=NULL) {
            if (!CertCloseStore(context->trust_store, CERT_CLOSE_STORE_CHECK_FLAG)) {
                BoltLog_warning(context->log, "Unable to close custom trust store: CertCloseStore returned: 0x%x",
                        GetLastError());
            }
            context->trust_store = NULL;
        }

        BoltMem_deallocate(context, sizeof(BoltSecurityContext));
    }
}

int BoltSecurityContext_startup()
{
    return BOLT_SUCCESS;
}

int BoltSecurityContext_shutdown()
{
    return BOLT_SUCCESS;
}

int secure_schannel_disconnect(BoltCommunication* comm);

int secure_schannel_handshake(BoltCommunication* comm);

int secure_schannel_handshake_loop(BoltCommunication* comm, int do_initial_read);

void secure_schannel_log_cert(BoltCommunication* comm, PCCERT_CONTEXT cert, int remote)
{
    SChannelContext* ctx = comm->context;

    char name[1000];

    // display leaf name
    if (!CertNameToStr(cert->dwCertEncodingType,
            &cert->pCertInfo->Subject,
            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
            name, sizeof(name))) {
        log_with_last_error(comm->log, &BoltLog_warning, GetLastError(),
                "[%s]: Unable to extract subject name: CertNameToStr returned 0x%x: '%s'", ctx->id);
    }
    BoltLog_debug(comm->log, "[%s]: %s subject name: %s", ctx->id, remote ? "Server" : "Client", name);

    if (!CertNameToStr(cert->dwCertEncodingType,
            &cert->pCertInfo->Issuer,
            CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
            name, sizeof(name))) {
        log_with_last_error(comm->log, &BoltLog_warning, GetLastError(),
                "[%s]: Unable to extract issuer name: CertNameToStr returned 0x%x: '%s'", ctx->id);
    }
    BoltLog_debug(comm->log, "[%s]: %s issuer name: %s", ctx->id, remote ? "Server" : "Client", name);

    // display certificate chain
    PCCERT_CONTEXT current_cert = cert;
    PCCERT_CONTEXT issuer_cert = NULL;
    int level = 0;
    while (1) {
        DWORD verification_flags = 0;
        issuer_cert = CertGetIssuerCertificateFromStore(cert->hCertStore, current_cert, NULL, &verification_flags);
        if (issuer_cert==NULL) {
            if (current_cert!=cert) {
                CertFreeCertificateContext(current_cert);
            }
            break;
        }

        if (!CertNameToStr(issuer_cert->dwCertEncodingType,
                &issuer_cert->pCertInfo->Subject,
                CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                name, sizeof(name))) {
            log_with_last_error(comm->log, &BoltLog_warning, GetLastError(),
                    "[%s]: Unable to extract CA subject name: CertNameToStr returned 0x%x: '%s'", ctx->id);
        }
        BoltLog_debug(comm->log, "[%s]: CA[%d] subject name: %s", ctx->id, level, name);

        if (!CertNameToStr(issuer_cert->dwCertEncodingType,
                &issuer_cert->pCertInfo->Issuer,
                CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                name, sizeof(name))) {
            log_with_last_error(comm->log, &BoltLog_warning, GetLastError(),
                    "[%s]: Unable to extract CA issuer name: CertNameToStr returned 0x%x: '%s'", ctx->id);
        }
        BoltLog_debug(comm->log, "[%s]: CA[%d] issuer name: %s", ctx->id, level, name);

        if (current_cert!=cert) {
            CertFreeCertificateContext(current_cert);
        }
        current_cert = issuer_cert;
        level += 1;
    }

    if (level==0 && remote) {
        BoltLog_warning(comm->log, "[%s]: Server did not provide its certificate chain.", ctx->id);
    }
}

int secure_schannel_verify_chain(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;

    if (ctx->context_handle==NULL) {
        return BOLT_TLS_ERROR;
    }

    int result;
    int server_name_length = 0;
    wchar_t* server_name = NULL;
    PCCERT_CHAIN_CONTEXT chain_context = NULL;

    // Retrive certificate presented by the remote server
    PCCERT_CONTEXT server_cert = NULL;
    SECURITY_STATUS status = QueryContextAttributes(ctx->context_handle, SECPKG_ATTR_REMOTE_CERT_CONTEXT,
            (void*) &server_cert);
    if (status!=SEC_E_OK) {
        log_with_sec_stat(comm->log, &BoltLog_error, status,
                "[%s]: Unable to retrieve server certificate: QueryContextAttributes returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_verify_chain(%s:%d), QueryContextAttributes error code: 0x%x", __FILE__, __LINE__,
                status);
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // Log server certificate
    secure_schannel_log_cert(comm, server_cert, 1);

    // Windows API requires the hostname given for hostname verification to be in UTF-8
    server_name_length = MultiByteToWideChar(CP_ACP, 0, ctx->hostname, -1, NULL, 0);
    server_name = malloc(sizeof(wchar_t)*server_name_length);
    if (server_name==NULL) {
        result = BOLT_OUT_OF_MEMORY;
        goto cleanup;
    }
    server_name_length = MultiByteToWideChar(CP_ACP, 0, ctx->hostname, -1, server_name, server_name_length);
    if (server_name_length==0) {
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // Set chain building parameters
    char* requested_usages[] = {szOID_PKIX_KP_SERVER_AUTH,
                                szOID_SERVER_GATED_CRYPTO,
                                szOID_SGC_NETSCAPE};
    DWORD requested_usages_length = sizeof(requested_usages)/sizeof(char*);

    CERT_CHAIN_PARA chain_param;
    memset(&chain_param, 0, sizeof(chain_param));
    chain_param.cbSize = sizeof(chain_param);
    chain_param.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    chain_param.RequestedUsage.Usage.cUsageIdentifier = requested_usages_length;
    chain_param.RequestedUsage.Usage.rgpszUsageIdentifier = requested_usages;

    // Build a chain for verification, include server presented chain for building
    if (!CertGetCertificateChain(ctx->sec_ctx->cert_engine, server_cert, NULL, server_cert->hCertStore, &chain_param, 0,
            NULL, &chain_context)) {
        DWORD error_code = GetLastError();
        log_with_last_error(comm->log, &BoltLog_error, error_code,
                "[%s]: Unable to build certificate chain: CertGetCertificateChain returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_verify_chain(%s:%d), CertGetCertificateChain error code: 0x%x", __FILE__, __LINE__,
                error_code);
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // Set chain verification parameters
    HTTPSPolicyCallbackData https_policy;
    CERT_CHAIN_POLICY_PARA policy_param;
    CERT_CHAIN_POLICY_STATUS policy_stat;

    memset(&https_policy, 0, sizeof(https_policy));
    https_policy.cbStruct = sizeof(HTTPSPolicyCallbackData);
    https_policy.dwAuthType = AUTHTYPE_SERVER;
    https_policy.fdwChecks = 0;
    https_policy.pwszServerName = server_name;

    memset(&policy_param, 0, sizeof(policy_param));
    policy_param.cbSize = sizeof(policy_param);
    policy_param.pvExtraPolicyPara = &https_policy;

    memset(&policy_stat, 0, sizeof(policy_stat));
    policy_stat.cbSize = sizeof(policy_stat);

    if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chain_context, &policy_param, &policy_stat)) {
        DWORD error_code = GetLastError();
        log_with_last_error(comm->log, &BoltLog_error, error_code,
                "[%s]: Unable to verify certificate chain: CertVerifyCertificateChainPolicy returned 0x%x: '%s'",
                ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_verify_chain(%s:%d), CertVerifyCertificateChainPolicy error code: 0x%x", __FILE__,
                __LINE__, error_code);
        result = BOLT_TLS_ERROR;
        goto cleanup;
    }

    // CertVerifyCertificateChainPolicy only returns 1 error, check if it returned CERT_E_CN_NO_MATCH
    // If the reported error is not a hostname mismatch, let's re-issue another CertVerifyCertificateChainPolicy call to
    // surface possible hostname mismatch condition.
    DWORD stored_error = 0;
    if (policy_stat.dwError!=0 && policy_stat.dwError!=(DWORD) CERT_E_CN_NO_MATCH) {
        stored_error = policy_stat.dwError;

        // once more excluding X509 related checks
        https_policy.fdwChecks = 0x00000080 |  // SECURITY_FLAG_IGNORE_REVOCATION
                0x00000100 |  // SECURITY_FLAG_IGNORE_UNKNOWN_CA
                0x00002000 |  // SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                0x00000200; // SECURITY_FLAG_IGNORE_WRONG_USAGE
        policy_param.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS |
                CERT_CHAIN_POLICY_IGNORE_INVALID_BASIC_CONSTRAINTS_FLAG |
                CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG |
                CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG |
                CERT_CHAIN_POLICY_IGNORE_INVALID_NAME_FLAG |
                CERT_CHAIN_POLICY_IGNORE_INVALID_POLICY_FLAG |
                CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS |
                CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
                CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG |
                CERT_CHAIN_POLICY_IGNORE_NOT_SUPPORTED_CRITICAL_EXT_FLAG |
                CERT_CHAIN_POLICY_IGNORE_PEER_TRUST_FLAG;

        if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, chain_context, &policy_param, &policy_stat)) {
            DWORD error_code = GetLastError();
            log_with_last_error(comm->log, &BoltLog_error, error_code,
                    "[%s]: Unable to verify certificate chain: CertVerifyCertificateChainPolicy returned 0x%x: '%s'",
                    ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_schannel_verify_chain(%s:%d), CertVerifyCertificateChainPolicy error code: 0x%x", __FILE__,
                    __LINE__, error_code);
            result = BOLT_STATUS_SET;
            goto cleanup;
        }
    }

    int fail = 0;
    if (policy_stat.dwError && policy_stat.dwError==(DWORD) CERT_E_CN_NO_MATCH) {
        if (ctx->trust!=NULL && ctx->trust->skip_verify_hostname) {
            BoltLog_warning(comm->log,
                    "[%s]: Hostname verification failed due to a mismatch, but resuming handshake since hostname verification is set to be skipped",
                    ctx->id);
            fail = fail!=0;
        }
        else {
            BoltLog_error(comm->log,
                    "[%s]: Hostname verification failed due to a mismatch, aborting handshake",
                    ctx->id);
            fail = 1;
        }
    }

    if (stored_error!=0) {
        if (ctx->trust!=NULL && ctx->trust->skip_verify) {
            BoltLog_warning(comm->log,
                    "[%s]: Unable to establish trust due to '%s' (code '0x%x'), but resuming handshake since trust verification is set to be skipped",
                    ctx->id, secure_schannel_trust_error(stored_error), stored_error);
            fail = fail!=0;
        }
        else {
            log_with_trust_error(comm->log, &BoltLog_error, stored_error,
                    "[%s]: Unable to establish trust due to 0x%x: '%s', aborting handshake", ctx->id);
            fail = 1;
        }
    }

    result = fail ? BOLT_TLS_ERROR : BOLT_SUCCESS;

    cleanup:
    if (chain_context!=NULL) {
        CertFreeCertificateChain(chain_context);
    }

    if (server_name!=NULL) {
        free(server_name);
    }

    return result;
}

int secure_schannel_open(BoltCommunication* comm, const struct sockaddr_storage* address)
{
    SChannelContext* ctx = comm->context;

    int status = ctx->plain_comm->open(ctx->plain_comm, address);
    if (status!=BOLT_SUCCESS) {
        return status;
    }

    if (ctx->sec_ctx==NULL) {
        ctx->sec_ctx = BoltSecurityContext_create(ctx->trust, ctx->hostname, comm->log, ctx->id);
        ctx->owns_sec_ctx = 1;
    }

    ctx->context_handle = BoltMem_allocate(sizeof(CtxtHandle));
    status = secure_schannel_handshake(comm);
    if (status!=BOLT_SUCCESS) {
        return status;
    }

    status = secure_schannel_verify_chain(comm);
    if (status!=BOLT_SUCCESS) {
        return status;
    }

    ctx->stream_sizes = BoltMem_allocate(sizeof(SecPkgContext_StreamSizes));
    SECURITY_STATUS size_status = QueryContextAttributes(ctx->context_handle, SECPKG_ATTR_STREAM_SIZES,
            ctx->stream_sizes);
    if (size_status!=SEC_E_OK) {
        log_with_sec_stat(comm->log, &BoltLog_error, size_status,
                "[%s]: Unable to query TLS stream sizes: QueryContextAttributes returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_open(%s:%d), QueryContextAttributes error code: 0x%x", __FILE__,
                __LINE__, size_status);
        return BOLT_STATUS_SET;
    }

    ctx->send_buffer_length = (int) (ctx->stream_sizes->cbHeader+ctx->stream_sizes->cbMaximumMessage
            +ctx->stream_sizes->cbTrailer);
    ctx->send_buffer = BoltMem_allocate(ctx->send_buffer_length);

    ctx->recv_buffer_length = (int) (ctx->stream_sizes->cbHeader+ctx->stream_sizes->cbMaximumMessage
            +ctx->stream_sizes->cbTrailer);
    ctx->recv_buffer_pos = 0;
    ctx->recv_buffer = BoltMem_allocate(ctx->recv_buffer_length);

    return BOLT_SUCCESS;
}

int secure_schannel_close(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;

    if (ctx->context_handle!=NULL) {
        int status = secure_schannel_disconnect(comm);
        if (status!=BOLT_SUCCESS) {
            // TODO: warn
        }

        DeleteSecurityContext(ctx->context_handle);
        BoltMem_deallocate(ctx->context_handle, sizeof(CtxtHandle));
        ctx->context_handle = NULL;
    }

    if (ctx->stream_sizes!=NULL) {
        BoltMem_deallocate(ctx->stream_sizes, sizeof(SecPkgContext_StreamSizes));
        ctx->stream_sizes = NULL;
    }

    if (ctx->send_buffer!=NULL) {
        BoltMem_deallocate(ctx->send_buffer, ctx->send_buffer_length);
        ctx->send_buffer_length = 0;
        ctx->send_buffer = NULL;
    }

    if (ctx->recv_buffer!=NULL) {
        BoltMem_deallocate(ctx->recv_buffer, ctx->recv_buffer_length);
        ctx->recv_buffer_length = 0;
        ctx->recv_buffer_pos = 0;
        ctx->recv_buffer = NULL;
    }

    ctx->pt_pending_buffer = NULL;
    ctx->pt_pending_buffer_pos = 0;
    ctx->pt_pending_buffer_length = 0;

    ctx->ct_pending_buffer = NULL;
    ctx->ct_pending_buffer_length = 0;

    if (ctx->sec_ctx!=NULL && ctx->owns_sec_ctx) {
        BoltSecurityContext_destroy(ctx->sec_ctx);
        ctx->sec_ctx = NULL;
        ctx->owns_sec_ctx = 0;
    }

    return ctx->plain_comm->close(ctx->plain_comm);
}

int secure_schannel_send(BoltCommunication* comm, char* buffer, int length, int* sent)
{
    SChannelContext* ctx = comm->context;

    SecBufferDesc msg;
    SecBuffer msg_bufs[4];

    char* msg_buffer = ctx->send_buffer+ctx->stream_sizes->cbHeader;
    int msg_buffer_length = 0;
    int msg_buffer_sent = 0;
    int remaining = length;
    int total_sent = 0;
    int result = BOLT_SUCCESS;
    while (total_sent<length) {
        int current_length =
                remaining<(int) ctx->stream_sizes->cbMaximumMessage ? remaining
                                                                    : (int) ctx->stream_sizes->cbMaximumMessage;
        char* current = buffer+total_sent;
        MoveMemory(msg_buffer, current, (size_t) current_length);

        msg_bufs[0].pvBuffer = ctx->send_buffer;
        msg_bufs[0].cbBuffer = ctx->stream_sizes->cbHeader;
        msg_bufs[0].BufferType = SECBUFFER_STREAM_HEADER;

        msg_bufs[1].pvBuffer = msg_buffer;
        msg_bufs[1].cbBuffer = (unsigned long) current_length;
        msg_bufs[1].BufferType = SECBUFFER_DATA;

        msg_bufs[2].pvBuffer = msg_buffer+current_length;
        msg_bufs[2].cbBuffer = ctx->stream_sizes->cbTrailer;
        msg_bufs[2].BufferType = SECBUFFER_STREAM_TRAILER;

        msg_bufs[3].BufferType = SECBUFFER_EMPTY;

        msg.ulVersion = SECBUFFER_VERSION;
        msg.cBuffers = 4;
        msg.pBuffers = msg_bufs;

        SECURITY_STATUS status = EncryptMessage(ctx->context_handle, 0, &msg, 0);
        if (FAILED(status)) {
            log_with_sec_stat(comm->log, &BoltLog_error, status,
                    "[%s]: Unable to encrypt outgoing message: EncryptMessage returned 0x%x: '%s'", ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_schannel_send(%s:%d), EncryptMessage error code: 0x%x", __FILE__,
                    __LINE__, status);
            result = BOLT_STATUS_SET;
            break;
        }

        msg_buffer_length = (int) (msg_bufs[0].cbBuffer+msg_bufs[1].cbBuffer+msg_bufs[2].cbBuffer);
        msg_buffer_sent = 0;
        while (msg_buffer_sent<msg_buffer_length) {
            int sent_current = 0;
            result = ctx->plain_comm->send(ctx->plain_comm, ctx->send_buffer+msg_buffer_sent,
                    msg_buffer_length-msg_buffer_sent, &sent_current);
            if (result!=BOLT_SUCCESS) {
                goto cleanup;
            }

            msg_buffer_sent += sent_current;
        }

        remaining -= current_length;
        total_sent += current_length;
    }

    cleanup:
    if (result==BOLT_SUCCESS) {
        *sent = total_sent;
    }

    return result;
}

int secure_schannel_recv(BoltCommunication* comm, char* buffer, int length, int* received)
{
    SChannelContext* ctx = comm->context;

    // First check if we have any pending plain text that was previously decrypted but not returned due to size
    // restrictions
    if (ctx->pt_pending_buffer!=NULL) {
        DWORD data_size = ctx->pt_pending_buffer_length-ctx->pt_pending_buffer_pos;
        DWORD copy_length = length<(int) data_size ? (DWORD) length : data_size;

        MoveMemory(buffer, ctx->pt_pending_buffer+ctx->pt_pending_buffer_pos, copy_length);
        *received = (int) copy_length;

        ctx->pt_pending_buffer_pos += copy_length;

        if (ctx->pt_pending_buffer_pos==ctx->pt_pending_buffer_length) {
            ctx->pt_pending_buffer = NULL;
            ctx->pt_pending_buffer_length = 0;
            ctx->pt_pending_buffer_pos = 0;
        }

        return BOLT_SUCCESS;
    }

    // Then check if we have any pending cipher text that was previously received from the socket but not yet
    // processed
    if (ctx->ct_pending_buffer!=NULL && ctx->ct_pending_buffer_length>0) {
        MoveMemory(ctx->recv_buffer, ctx->ct_pending_buffer, ctx->ct_pending_buffer_length);
        ctx->recv_buffer_pos = ctx->ct_pending_buffer_length;

        ctx->ct_pending_buffer = NULL;
        ctx->ct_pending_buffer_length = 0;
    }

    SecBufferDesc msg;
    SecBuffer msg_bufs[4];

    SecBuffer* data_buffer;
    SecBuffer* extra_buffer;

    SECURITY_STATUS status = SEC_E_OK;
    int result = BOLT_SUCCESS;

    memset(msg_bufs, 0, sizeof(msg_bufs));

    while (1) {
        if (ctx->recv_buffer_pos==0 || status==SEC_E_INCOMPLETE_MESSAGE) {
            int now_received = 0;
            result = ctx->plain_comm->recv(ctx->plain_comm, ctx->recv_buffer+ctx->recv_buffer_pos,
                    (int) (ctx->recv_buffer_length-ctx->recv_buffer_pos), &now_received);
            if (result!=BOLT_SUCCESS) {
                break;
            }

            ctx->recv_buffer_pos += now_received;
        }

        msg_bufs[0].pvBuffer = ctx->recv_buffer;
        msg_bufs[0].cbBuffer = (unsigned long) ctx->recv_buffer_pos;
        msg_bufs[0].BufferType = SECBUFFER_DATA;

        msg_bufs[1].BufferType = SECBUFFER_EMPTY;
        msg_bufs[2].BufferType = SECBUFFER_EMPTY;
        msg_bufs[3].BufferType = SECBUFFER_EMPTY;

        msg.ulVersion = SECBUFFER_VERSION;
        msg.cBuffers = 4;
        msg.pBuffers = msg_bufs;

        status = DecryptMessage(ctx->context_handle, &msg, 0, NULL);
        if (status==SEC_E_INCOMPLETE_MESSAGE) {
            continue;
        }

        if (status==SEC_I_CONTEXT_EXPIRED) {
            log_with_sec_stat(comm->log, &BoltLog_error, status,
                    "[%s]: Unable to decrypt incoming message: DecryptMessage returned 0x%x: '%s'", ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_END_OF_TRANSMISSION,
                    "secure_schannel_recv(%s:%d), DecryptMessage error code: 0x%x", __FILE__,
                    __LINE__, status);
            result = BOLT_STATUS_SET;
            break;
        }

        if (status!=SEC_E_OK && status!=SEC_I_RENEGOTIATE) {
            log_with_sec_stat(comm->log, &BoltLog_error, status,
                    "[%s]: Unable to decrypt incoming message: DecryptMessage returned 0x%x: '%s'", ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_schannel_recv(%s:%d), DecryptMessage error code: 0x%x", __FILE__,
                    __LINE__, status);
            result = BOLT_STATUS_SET;
            break;
        }

        data_buffer = NULL;
        extra_buffer = NULL;
        for (int i = 1; i<(int) msg.cBuffers; i++) {
            if (data_buffer==NULL && msg_bufs[i].BufferType==SECBUFFER_DATA) {
                data_buffer = &msg_bufs[i];
            }
            if (extra_buffer==NULL && msg_bufs[i].BufferType==SECBUFFER_EXTRA) {
                extra_buffer = &msg_bufs[i];
            }
        }

        if (data_buffer) {
            DWORD data_size = data_buffer->cbBuffer;
            DWORD copy_length = length<(int) data_size ? (DWORD) length : data_size;
            MoveMemory(buffer, data_buffer->pvBuffer, copy_length);
            *received = (int) copy_length;

            if (copy_length<data_size) {
                ctx->pt_pending_buffer = data_buffer->pvBuffer;
                ctx->pt_pending_buffer_length = data_size;
                ctx->pt_pending_buffer_pos = copy_length;
            }
            else {
                ctx->pt_pending_buffer = NULL;
                ctx->pt_pending_buffer_length = 0;
                ctx->pt_pending_buffer_pos = 0;
            }
        }

        if (extra_buffer) {
            ctx->ct_pending_buffer = extra_buffer->pvBuffer;
            ctx->ct_pending_buffer_length = (int) extra_buffer->cbBuffer;
        }

        if (status==SEC_I_RENEGOTIATE) {
            BoltLog_debug(comm->log, "[%s]: The server asked for a new handshake, entering handshake loop", ctx->id);
            result = secure_schannel_handshake_loop(comm, 0);
            if (result!=BOLT_SUCCESS) {
                break;
            }
        }

        if (status==SEC_E_OK) {
            result = BOLT_SUCCESS;
            ctx->recv_buffer_pos = 0;
            break;
        }
    }

    return result;
}

int secure_schannel_destroy(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;
    if (ctx==NULL) {
        return BOLT_SUCCESS;
    }

    if (ctx->context_handle!=NULL) {
        DeleteSecurityContext(ctx->context_handle);
        BoltMem_deallocate(ctx->context_handle, sizeof(CtxtHandle));
        ctx->context_handle = NULL;
    }

    if (ctx->stream_sizes!=NULL) {
        BoltMem_deallocate(ctx->stream_sizes, sizeof(SecPkgContext_StreamSizes));
        ctx->stream_sizes = NULL;
    }

    if (ctx->send_buffer!=NULL) {
        BoltMem_deallocate(ctx->send_buffer, ctx->send_buffer_length);
        ctx->send_buffer_length = 0;
        ctx->send_buffer = NULL;
    }

    if (ctx->recv_buffer!=NULL) {
        BoltMem_deallocate(ctx->recv_buffer, ctx->recv_buffer_length);
        ctx->recv_buffer_length = 0;
        ctx->recv_buffer_pos = 0;
        ctx->recv_buffer = NULL;
    }

    if (ctx->hs_buffer!=NULL) {
        BoltMem_deallocate(ctx->hs_buffer, ctx->hs_buffer_length);
        ctx->hs_buffer = NULL;
        ctx->hs_buffer_length = 0;
        ctx->hs_buffer_pos = 0;
    }

    if (ctx->sec_ctx!=NULL && ctx->owns_sec_ctx) {
        BoltSecurityContext_destroy(ctx->sec_ctx);
        ctx->sec_ctx = NULL;
        ctx->owns_sec_ctx = 0;
    }

    BoltCommunication_destroy(ctx->plain_comm);
    BoltMem_deallocate(ctx->hostname, strlen(ctx->hostname)+1);
    BoltMem_deallocate(ctx->id, strlen(ctx->id)+1);
    BoltMem_deallocate(ctx, sizeof(SChannelContext));
    comm->context = NULL;

    return BOLT_SUCCESS;
}

BoltAddress* secure_schannel_local_endpoint(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;
    return ctx->plain_comm->get_local_endpoint(ctx->plain_comm);
}

BoltAddress* secure_schannel_remote_endpoint(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;
    return ctx->plain_comm->get_remote_endpoint(ctx->plain_comm);
}

int secure_schannel_ignore_sigpipe(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;
    return ctx->plain_comm->ignore_sigpipe(ctx->plain_comm);
}

int secure_schannel_restore_sigpipe(BoltCommunication* comm)
{
    SChannelContext* ctx = comm->context;
    return ctx->plain_comm->restore_sigpipe(ctx->plain_comm);
}

BoltCommunication* BoltCommunication_create_secure(BoltSecurityContext* sec_ctx, BoltTrust* trust,
        BoltSocketOptions* socket_options, BoltLog* log, const char* hostname, const char* id)
{
    BoltCommunication* plain_comm = BoltCommunication_create_plain(socket_options, log);

    BoltCommunication* comm = BoltMem_allocate(sizeof(BoltCommunication));
    comm->open = &secure_schannel_open;
    comm->close = &secure_schannel_close;
    comm->send = &secure_schannel_send;
    comm->recv = &secure_schannel_recv;
    comm->destroy = &secure_schannel_destroy;

    comm->get_local_endpoint = &secure_schannel_local_endpoint;
    comm->get_remote_endpoint = &secure_schannel_remote_endpoint;

    comm->ignore_sigpipe = &secure_schannel_ignore_sigpipe;
    comm->restore_sigpipe = &secure_schannel_restore_sigpipe;

    comm->status_owned = 0;
    comm->status = plain_comm->status;
    comm->sock_opts_owned = 0;
    comm->sock_opts = plain_comm->sock_opts;
    comm->log = log;

    SChannelContext* context = BoltMem_allocate(sizeof(SChannelContext));
    context->sec_ctx = sec_ctx;
    context->owns_sec_ctx = sec_ctx==NULL;
    context->context_handle = NULL;
    context->stream_sizes = NULL;
    context->recv_buffer = NULL;
    context->recv_buffer_pos = 0;
    context->recv_buffer_length = 0;
    context->pt_pending_buffer = NULL;
    context->pt_pending_buffer_pos = 0;
    context->pt_pending_buffer_length = 0;
    context->ct_pending_buffer = NULL;
    context->ct_pending_buffer_length = 0;
    context->send_buffer = NULL;
    context->send_buffer_length = 0;
    context->hs_buffer_length = 16*1024;
    context->hs_buffer_pos = 0;
    context->hs_buffer = BoltMem_allocate(context->hs_buffer_length);
    context->trust = trust;
    context->plain_comm = plain_comm;
    context->id = BoltMem_duplicate(id, strlen(id)+1);
    context->hostname = BoltMem_duplicate(hostname, strlen(hostname)+1);

    comm->context = context;

    return comm;
}

int secure_schannel_disconnect(BoltCommunication* comm)
{
    SecBufferDesc out_buf;
    SecBuffer out_bufs[1];
    SECURITY_STATUS status;

    int result = BOLT_SUCCESS;

    SChannelContext* ctx = comm->context;

    // Generate a shutdown token
    int control_token = SCHANNEL_SHUTDOWN;
    out_bufs[0].pvBuffer = &control_token;
    out_bufs[0].BufferType = SECBUFFER_TOKEN;
    out_bufs[0].cbBuffer = sizeof(control_token);

    out_buf.cBuffers = 1;
    out_buf.pBuffers = out_bufs;
    out_buf.ulVersion = SECBUFFER_VERSION;

    status = ApplyControlToken(ctx->context_handle, &out_buf);
    if (status!=SEC_E_OK) {
        log_with_sec_stat(comm->log, &BoltLog_error, status,
                "[%s]: Unable to generate shutdown token: ApplyControlToken returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_disconnect(%s:%d), ApplyControlToken error code: 0x%x", __FILE__,
                __LINE__, status);
        return BOLT_STATUS_SET;
    }

    // Generate the actual message sequence
    DWORD sspi_flags_requested = ISC_REQ_SEQUENCE_DETECT |
            ISC_REQ_REPLAY_DETECT |
            ISC_REQ_CONFIDENTIALITY |
            ISC_RET_EXTENDED_ERROR |
            ISC_REQ_ALLOCATE_MEMORY |
            ISC_REQ_STREAM;
    DWORD sspi_flags_got;

    out_bufs[0].pvBuffer = NULL;
    out_bufs[0].BufferType = SECBUFFER_TOKEN;
    out_bufs[0].cbBuffer = 0;

    out_buf.cBuffers = 1;
    out_buf.pBuffers = out_bufs;
    out_buf.ulVersion = SECBUFFER_VERSION;

    status = InitializeSecurityContext(ctx->sec_ctx->cred_handle, ctx->context_handle, NULL, sspi_flags_requested, 0,
            SECURITY_NATIVE_DREP, NULL, 0, ctx->context_handle, &out_buf, &sspi_flags_got, NULL);
    if (status!=SEC_E_OK) {
        log_with_sec_stat(comm->log, &BoltLog_error, status,
                "[%s]: Unable to generate shutdown token: InitializeSecurityToken returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_disconnect(%s:%d), InitializeSecurityToken error code: 0x%x", __FILE__,
                __LINE__, status);
        return BOLT_STATUS_SET;
    }

    // Send out the shutdown token to the server
    char* msg = out_bufs[0].pvBuffer;
    DWORD msg_length = (int) out_bufs[0].cbBuffer;
    DWORD msg_sent = 0;
    while (msg_sent<msg_length) {
        int sent = 0;
        result = ctx->plain_comm->send(ctx->plain_comm, msg+msg_sent, msg_length-msg_sent, &sent);
        if (result!=BOLT_SUCCESS) {
            break;
        }

        msg_sent += sent;
    }

    if (out_bufs[0].pvBuffer!=NULL) {
        FreeContextBuffer(out_bufs[0].pvBuffer);
    }

    return result;
}

int secure_schannel_handshake(BoltCommunication* comm)
{
    SecBufferDesc out_buf;
    SecBuffer out_bufs[1];
    SECURITY_STATUS status;

    SChannelContext* ctx = comm->context;

    DWORD sspi_flags_requested = ISC_REQ_SEQUENCE_DETECT |
            ISC_REQ_REPLAY_DETECT |
            ISC_REQ_CONFIDENTIALITY |
            ISC_RET_EXTENDED_ERROR |
            ISC_REQ_ALLOCATE_MEMORY |
            ISC_REQ_STREAM;
    DWORD sspi_flags_got;

    out_bufs[0].pvBuffer = NULL;
    out_bufs[0].BufferType = SECBUFFER_TOKEN;
    out_bufs[0].cbBuffer = 0;

    out_buf.cBuffers = 1;
    out_buf.pBuffers = out_bufs;
    out_buf.ulVersion = SECBUFFER_VERSION;

    status = InitializeSecurityContext(ctx->sec_ctx->cred_handle, NULL, ctx->hostname, sspi_flags_requested, 0,
            SECURITY_NATIVE_DREP, NULL, 0, ctx->context_handle, &out_buf, &sspi_flags_got, NULL);
    if (status!=SEC_I_CONTINUE_NEEDED) {
        log_with_sec_stat(comm->log, &BoltLog_error, status,
                "[%s]: TLS handshake initialization failed: InitializeSecurityToken returned 0x%x: '%s'", ctx->id);
        BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                "secure_schannel_handshake(%s:%d), InitializeSecurityToken error code: 0x%x", __FILE__,
                __LINE__, status);
        return BOLT_STATUS_SET;
    }

    char* msg = out_bufs[0].pvBuffer;
    DWORD msg_length = out_bufs[0].cbBuffer;
    if (msg!=NULL && msg_length!=0) {
        DWORD msg_sent = 0;
        int result = BOLT_SUCCESS;
        while (msg_sent<msg_length) {
            int sent = 0;
            result = ctx->plain_comm->send(ctx->plain_comm, msg+msg_sent, (int) (msg_length-msg_sent), &sent);
            if (result!=BOLT_SUCCESS) {
                break;
            }

            msg_sent += sent;
        }

        FreeContextBuffer(out_bufs[0].pvBuffer);

        if (result!=BOLT_SUCCESS) {
            return result;
        }
    }

    return secure_schannel_handshake_loop(comm, 1);
}

int secure_schannel_handshake_loop(BoltCommunication* comm, int do_initial_read)
{
    int result = BOLT_SUCCESS;
    SChannelContext* ctx = comm->context;

    SecBufferDesc in_buf;
    SecBuffer in_bufs[2];
    SecBufferDesc out_buf;
    SecBuffer out_bufs[1];
    memset(in_bufs, 0, sizeof(in_bufs));
    memset(out_bufs, 0, sizeof(out_bufs));

    DWORD sspi_flags_requested = ISC_REQ_SEQUENCE_DETECT |
            ISC_REQ_REPLAY_DETECT |
            ISC_REQ_CONFIDENTIALITY |
            ISC_RET_EXTENDED_ERROR |
            ISC_REQ_ALLOCATE_MEMORY |
            ISC_REQ_STREAM;
    DWORD sspi_flags_got;

    int do_read = do_initial_read;
    SECURITY_STATUS status = SEC_I_CONTINUE_NEEDED;
    while (status==SEC_I_CONTINUE_NEEDED || status==SEC_E_INCOMPLETE_MESSAGE
            || status==SEC_I_INCOMPLETE_CREDENTIALS) {
        if (ctx->hs_buffer_pos==0 || status==SEC_E_INCOMPLETE_MESSAGE) {
            if (do_read) {
                int received = 0;
                result = ctx->plain_comm->recv(ctx->plain_comm, ctx->hs_buffer+ctx->hs_buffer_pos,
                        (int) (ctx->hs_buffer_length-ctx->hs_buffer_pos), &received);
                if (result!=BOLT_SUCCESS) {
                    break;
                }

                ctx->hs_buffer_pos += received;
            }
            else {
                do_read = 1;
            }
        }

        // Set up input buffers. Buffer 0 is used to pass in data received from the server. Schannel will consume
        // some or all of this. Leftover data (if any) will be placed in buffer 1 and given a buffer type of
        // SECBUFFER_EXTRA.
        in_bufs[0].pvBuffer = ctx->hs_buffer;
        in_bufs[0].cbBuffer = (unsigned long) ctx->hs_buffer_pos;
        in_bufs[0].BufferType = SECBUFFER_TOKEN;

        // Free previously allocated buffers if present
        if (in_bufs[1].pvBuffer!=NULL) {
            FreeContextBuffer(in_bufs[1].pvBuffer);
        }
        in_bufs[1].pvBuffer = NULL;
        in_bufs[1].cbBuffer = 0;
        in_bufs[1].BufferType = SECBUFFER_EMPTY;

        in_buf.cBuffers = 2;
        in_buf.pBuffers = in_bufs;
        in_buf.ulVersion = SECBUFFER_VERSION;

        // Set up output buffers. These are initialized to NULL so as to make it less likely we'll attempt to
        // free random garbage later.
        if (out_bufs[0].pvBuffer!=NULL) {
            FreeContextBuffer(out_bufs[0].pvBuffer);
        }
        out_bufs[0].pvBuffer = NULL;
        out_bufs[0].BufferType = SECBUFFER_TOKEN;
        out_bufs[0].cbBuffer = 0;

        out_buf.cBuffers = 1;
        out_buf.pBuffers = out_bufs;
        out_buf.ulVersion = SECBUFFER_VERSION;

        status = InitializeSecurityContext(ctx->sec_ctx->cred_handle, ctx->context_handle, NULL, sspi_flags_requested,
                0, SECURITY_NATIVE_DREP, &in_buf, 0, NULL, &out_buf, &sspi_flags_got, NULL);
        // If InitializeSecurityContext was successful (or if the error was
        // one of the special extended ones), send the contends of the output
        // buffer to the server.
        if (status==SEC_E_OK || status==SEC_I_CONTINUE_NEEDED
                || (FAILED(status) && (sspi_flags_got & ISC_RET_EXTENDED_ERROR))) {
            char* msg = out_bufs[0].pvBuffer;
            DWORD msg_length = out_bufs[0].cbBuffer;

            if (msg!=NULL && msg_length!=0) {
                DWORD msg_sent = 0;
                while (msg_sent<msg_length) {
                    int sent = 0;
                    result = ctx->plain_comm->send(ctx->plain_comm, msg+msg_sent, (int) (msg_length-msg_sent), &sent);
                    if (result!=BOLT_SUCCESS) {
                        goto cleanup;
                    }

                    msg_sent += sent;
                }
            }
        }

        // If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE, then we need to read more data
        // from the server and try again.
        if (status==SEC_E_INCOMPLETE_MESSAGE) {
            continue;
        }

        // If InitializeSecurityContext returned SEC_E_OK, then the handshake completed successfully.
        if (status==SEC_E_OK) {
            // If the "extra" buffer contains data, this is encrypted application protocol layer stuff. It
            // needs to be saved. The application layer will later decrypt it with DecryptMessage.
            if (in_bufs[1].BufferType==SECBUFFER_EXTRA) {
                MoveMemory(ctx->recv_buffer+ctx->recv_buffer_pos, in_bufs[1].pvBuffer, in_bufs[1].cbBuffer);
                ctx->ct_pending_buffer = ctx->recv_buffer+ctx->recv_buffer_pos;
                ctx->ct_pending_buffer_length = in_bufs[1].cbBuffer;
            }

            result = BOLT_SUCCESS;

            break;
        }

        if (FAILED(status)) {
            log_with_sec_stat(comm->log, &BoltLog_error, status,
                    "[%s]: TLS handshake loop failed: InitializeSecurityToken returned 0x%x: '%s'", ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_schannel_handshake_loop(%s:%d), InitializeSecurityToken error code: 0x%x", __FILE__,
                    __LINE__, status);
            result = BOLT_STATUS_SET;
            break;
        }

        // If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
        // then the server just requested client authentication.
        if (status==SEC_I_INCOMPLETE_CREDENTIALS) {
            BoltLog_error(comm->log, "[%s]: TLS mutual authentication is not supported.", ctx->id);
            BoltStatus_set_error_with_ctx(comm->status, BOLT_TLS_ERROR,
                    "secure_schannel_handshake_loop(%s:%d), TLS mutual authentication is not supported", __FILE__,
                    __LINE__);
            result = BOLT_STATUS_SET;
            break;
        }

        if (in_bufs[1].BufferType==SECBUFFER_EXTRA) {
            MoveMemory(ctx->hs_buffer, in_bufs[1].pvBuffer, in_bufs[1].cbBuffer);
            ctx->hs_buffer_pos = in_bufs[1].cbBuffer;
        }
        else {
            ctx->hs_buffer_pos = 0;
        }
    }

    cleanup:
    if (in_bufs[1].pvBuffer!=NULL) {
        FreeContextBuffer(in_bufs[1].pvBuffer);
    }

    if (out_bufs[0].pvBuffer!=NULL) {
        FreeContextBuffer(out_bufs[0].pvBuffer);
    }

    return result;
}
