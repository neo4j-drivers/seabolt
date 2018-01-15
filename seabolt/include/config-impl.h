#ifndef SEABOLT_CONFIG_IMPL
#define SEABOLT_CONFIG_IMPL
#include "config.h"

#if USE_POSIXSOCK
#include <sys/socket.h>
#include <arpa/inet.h>
#endif // USE_POSIXSOCK

#if USE_WINSOCK
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif // USE_WINSOCK

#if USE_WINSSPI

#endif // USE_WINSSPI

#if USE_OPENSSL

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#endif // USE_OPENSSL

#endif // SEABOLT_CONFIG_IMPL
