#ifndef SEABOLT_CONFIG_IMPL
#define SEABOLT_CONFIG_IMPL
#include "config.h"

#if USE_POSIXSOCK
#include <netdb.h>
#endif // USE_POSIXSOCK

#ifdef USE_WINSOCK
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif // USE_WINSOCK

#ifdef USE_WINSSPI

#endif // USE_WINSSPI

#ifdef USE_OPENSSL

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#endif // USE_OPENSSL

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#endif // SEABOLT_CONFIG_IMPL
