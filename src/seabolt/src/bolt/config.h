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
#ifndef SEABOLT_CONFIG_H
#define SEABOLT_CONFIG_H

#include "bolt-public.h"
#include "address-resolver.h"
#include "values.h"

/**
 * The operating mode of the connector.
 */
typedef int32_t BoltScheme;
/**
 * Use BOLT_SCHEME_DIRECT to establish direct connections towards a single server
 */
#define BOLT_SCHEME_DIRECT    0

#ifndef SEABOLT_NO_DEPRECATED
/**
 * Use BOLT_SCHEME_ROUTING to establish routing connections towards a casual cluster.
 * This is deprecated, please use BOLT_SCHEME_NEO4J instead.
 */
#define BOLT_SCHEME_ROUTING  1
#endif
/**
 * Use BOLT_SCHEME_NEO4J to establish routing first connections towards a neo4j server.
 */
#define BOLT_SCHEME_NEO4J     1

/**
 * The transport to use for established connections.
 */
typedef int32_t BoltTransport;
/**
 * Use BOLT_TRANSPORT_PLAINTEXT to establish clear text connections
 */
#define BOLT_TRANSPORT_PLAINTEXT    0
/**
 * Use BOLT_TRANSPORT_ENCRYPTED to establish encrypted connections that're protected with TLS 1.2.
 */
#define BOLT_TRANSPORT_ENCRYPTED    1

/**
 * The type that holds available configuration options applicable to underlying sockets.
 *
 * An instance needs to be created with \ref BoltSocketOptions_create and destroyed with \ref BoltSocketOptions_destroy
 * either after \ref BoltConfig_set_socket_options is called if using \ref BoltConnector, or after all
 * \ref BoltConnection instances explicitly opened with \ref BoltConnection_open are closed.
 */
typedef struct BoltSocketOptions BoltSocketOptions;

/**
 * Creates a new instance of \ref BoltSocketOptions.
 *
 * @returns the pointer to the newly allocated \ref BoltSocketOptions instance.
 */
SEABOLT_EXPORT BoltSocketOptions* BoltSocketOptions_create();

/**
 * Destroys the passed \ref BoltSocketOptions instance.
 *
 * @param socket_options the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltSocketOptions_destroy(BoltSocketOptions* socket_options);

/**
 * Gets the configured connect timeout.
 *
 * @param socket_options the socket options instance to query.
 * @return the configured connect timeout.
 */
SEABOLT_EXPORT int32_t BoltSocketOptions_get_connect_timeout(BoltSocketOptions* socket_options);

/**
 * Sets the configured connect timeout.
 *
 * @param socket_options the socket options instance to modify.
 * @param connect_timeout the connect timeout to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t
BoltSocketOptions_set_connect_timeout(BoltSocketOptions* socket_options, int32_t connect_timeout);

/**
 * Gets whether keep alive is enabled or not.
 *
 * @param socket_options the socket options instance to query.
 * @return the configured keep alive, 1 if enabled or 0 if disabled.
 */
SEABOLT_EXPORT int32_t BoltSocketOptions_get_keep_alive(BoltSocketOptions* socket_options);

/**
 * Sets whether keep alive is enabled or not
 *
 * @param socket_options the socket options instance to modify.
 * @param keep_alive the keep alive mode to set, 1 to enable and 0 to disable.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltSocketOptions_set_keep_alive(BoltSocketOptions* socket_options, int32_t keep_alive);

/**
 * The type that holds available configuration options applicable to encrypted connections.
 *
 * An instance needs to be created with \ref BoltTrust_create and destroyed with \ref BoltTrust_destroy either
 * after \ref BoltConfig_set_trust is called if using \ref BoltConnector, or after all \ref BoltConnection instances
 * explicitly opened with \ref BoltConnection_open are closed.
 */
typedef struct BoltTrust BoltTrust;

/**
 * Creates a new instance of \ref BoltTrust.
 *
 * @returns the pointer to the newly allocated \ref BoltTrust instance.
 */
SEABOLT_EXPORT BoltTrust* BoltTrust_create();

/**
 * Destroys the passed \ref BoltTrust instance.
 *
 * @param trust the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltTrust_destroy(BoltTrust* trust);

/**
 * Gets the configured trusted certificate byte stream (sequence of PEM encoded X509 certificates).
 *
 * @param trust the trust instance to query.
 * @param size the size of the returned byte stream, set to 0 if certs stream is NULL
 * @return the configured trusted certificate byte stream.
 */
SEABOLT_EXPORT const char* BoltTrust_get_certs(BoltTrust* trust, uint64_t* size);

/**
 * Sets the configured trusted certificate byte stream (sequence of PEM encoded X509 certificates).
 *
 * @param trust the trust instance to query.
 * @param certs_pem the trusted certificate byte stream to set.
 * @param certs_pem_size the size of the trusted certificate byte stream.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltTrust_set_certs(BoltTrust* trust, const char* certs_pem, uint64_t certs_pem_size);

/**
 * Gets the configured verification mode.
 *
 * @param trust the trust instance to query.
 * @return the configured verification mode, 1 if verification is in place or 0 if it will be skipped.
 */
SEABOLT_EXPORT int32_t BoltTrust_get_skip_verify(BoltTrust* trust);

/**
 * Sets the configured verification mode.
 *
 * @param trust the trust instance to modify.
 * @param skip_verify the verification mode to set, 1 to enable verification or 0 to disable.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltTrust_set_skip_verify(BoltTrust* trust, int32_t skip_verify);

/**
 * Gets the configured hostname verification mode.
 *
 * @param trust the trust instance to query.
 * @return the configured hostname verification mode, 1 if hostname verification is in place or 0 if it will be skipped.
 */
SEABOLT_EXPORT int32_t BoltTrust_get_skip_verify_hostname(BoltTrust* trust);

/**
 * Sets the configured hostname verification mode.
 *
 * @param trust the trust instance to modify.
 * @param skip_verify_hostname the hostname verification mode to set, 1 to enable verification or 0 to disable.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltTrust_set_skip_verify_hostname(BoltTrust* trust, int32_t skip_verify_hostname);

/**
 * The type that holds available configuration options to be provided to \ref BoltConnector.
 *
 * An instance needs to be created with \ref BoltConfig_create and destroyed with \ref BoltConfig_destroy after
 * \ref BoltConnector_create is called.
 */
typedef struct BoltConfig BoltConfig;

/**
 * Creates a new instance of \ref BoltConfig.
 *
 * @returns the pointer to the newly allocated \ref BoltConfig instance.
 */
SEABOLT_EXPORT BoltConfig* BoltConfig_create();

/**
 * Destroys the passed \ref BoltConfig instance.
 *
 * @param config the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltConfig_destroy(BoltConfig* config);

/**
 * Gets the configured \ref BoltScheme "scheme".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltScheme "scheme".
 */
SEABOLT_EXPORT BoltScheme BoltConfig_get_scheme(BoltConfig* config);

/**
 * Sets the configured \ref BoltScheme "scheme".
 *
 * @param config the config instance to modify.
 * @param mode the \ref BoltScheme "scheme" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_scheme(BoltConfig* config, BoltScheme scheme);

/**
 * Gets the configured \ref BoltTransport "transport".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltTransport "transport".
 */
SEABOLT_EXPORT BoltTransport BoltConfig_get_transport(BoltConfig* config);

/**
 * Sets the configured \ref BoltTransport "transport".
 *
 * @param config the config instance to modify.
 * @param transport the \ref BoltTransport "transport" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_transport(BoltConfig* config, BoltTransport transport);

/**
 * Gets the configured \ref BoltTrust "trust settings".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltTrust "trust settings".
 */
SEABOLT_EXPORT BoltTrust* BoltConfig_get_trust(BoltConfig* config);

/**
 * Sets the configured \ref BoltTrust "trust settings".
 *
 * @param config the config instance to modify.
 * @param trust the \ref BoltTrust "trust settings" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_trust(BoltConfig* config, BoltTrust* trust);

/**
 * Gets the configured user agent that will be presented to the server.
 *
 * @param config the config instance to query.
 * @return the configured user agent.
 */
SEABOLT_EXPORT const char* BoltConfig_get_user_agent(BoltConfig* config);

/**
 * Sets the configured user agent that will be presented to the server.
 *
 * @param config the config instance to modify.
 * @param user_agent the user agent to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_user_agent(BoltConfig* config, const char* user_agent);

/**
 * Gets the configured \ref BoltValue "routing context".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltValue "routing context".
 */
SEABOLT_EXPORT BoltValue* BoltConfig_get_routing_context(BoltConfig* config);

/**
 * Sets the configured \ref BoltValue "routing context". The routing context is a \ref BoltValue of type Dictionary
 * which consists of string key=value pairs that will be passed on to the routing procedure as explained in
 * <a href="https://neo4j.com/docs/developer-manual/current/drivers/client-applications/#_routing_drivers_with_routing_context">Routing drivers with routing context</a>
 *
 * @param config the config instance to modify.
 * @param routing_context the \ref BoltValue "routing context" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_routing_context(BoltConfig* config, BoltValue* routing_context);

/**
 * Gets the configured \ref BoltAddressResolver "address resolver".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltAddressResolver "address resolver".
 */
SEABOLT_EXPORT BoltAddressResolver* BoltConfig_get_address_resolver(BoltConfig* config);

/**
 * Sets the configured \ref BoltAddressResolver "address resolver".
 *
 * @param config the config instance to modify.
 * @param address_resolver the \ref BoltAddressResolver "address resolver" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_address_resolver(BoltConfig* config, BoltAddressResolver* address_resolver);

/**
 * Gets the configured \ref BoltLog "logger".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltLog "logger".
 */
SEABOLT_EXPORT BoltLog* BoltConfig_get_log(BoltConfig* config);

/**
 * Sets the configured \ref BoltLog "logger".
 *
 * @param config the config instance to modify.
 * @param log the \ref BoltLog "logger" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_log(BoltConfig* config, BoltLog* log);

/**
 * Gets the configured maximum connection pool size.
 *
 * @param config the config instance to query.
 * @return the configured maximum connection pool size.
 */
SEABOLT_EXPORT int32_t BoltConfig_get_max_pool_size(BoltConfig* config);

/**
 * Sets the configured maximum connection pool size.
 *
 * @param config the config instance to modify.
 * @param max_pool_size the maximum connection pool size to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_max_pool_size(BoltConfig* config, int32_t max_pool_size);

/**
 * Gets the configured maximum connection life time.
 *
 * @param config the config instance to query.
 * @return the configured maximum connection life time.
 */
SEABOLT_EXPORT int32_t BoltConfig_get_max_connection_life_time(BoltConfig* config);

/**
 * Sets the configured maximum connection life time.
 *
 * @param config the config instance to modify.
 * @param max_connection_life_time the maximum connection life time to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_max_connection_life_time(BoltConfig* config, int32_t max_connection_life_time);

/**
 * Gets the configured maximum connection acquisition time.
 *
 * @param config the config instance to query.
 * @return the configured maximum connection acquisition time.
 */
SEABOLT_EXPORT int32_t BoltConfig_get_max_connection_acquisition_time(BoltConfig* config);

/**
 * Sets the configured maximum connection acquisition time.
 *
 * @param config the config instance to modify.
 * @param max_connection_acquisition_time the maximum connection acquisition time to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t
BoltConfig_set_max_connection_acquisition_time(BoltConfig* config, int32_t max_connection_acquisition_time);

/**
 * Gets the configured \ref BoltSocketOptions "socket options".
 *
 * @param config the config instance to query.
 * @return the configured \ref BoltSocketOptions "socket options".
 */
SEABOLT_EXPORT BoltSocketOptions* BoltConfig_get_socket_options(BoltConfig* config);

/**
 * Sets the configured \ref BoltSocketOptions "socket options".
 *
 * @param config the config instance to modify.
 * @param socket_options the \ref BoltSocketOptions "socket options" to set.
 * @returns \ref BOLT_SUCCESS when the operation is successful, or another positive error code identifying the reason.
 */
SEABOLT_EXPORT int32_t BoltConfig_set_socket_options(BoltConfig* config, BoltSocketOptions* socket_options);

#endif //SEABOLT_CONFIG_H
