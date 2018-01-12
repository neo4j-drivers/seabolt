#ifndef SEABOLT_CONFIG
#define SEABOLT_CONFIG
#include "config-options.h"

#define BOLT_PUBLIC_API

#ifdef WIN32
typedef unsigned short in_port_t;
#define BOLT_PUBLIC_API __declspec(dllexport)
#endif // USE_WINSOCK

#endif // SEABOLT_CONFIG
