
#ifndef SEABOLT_EXPORT_H
#define SEABOLT_EXPORT_H

#ifdef SEABOLT_STATIC_DEFINE
#  define SEABOLT_EXPORT
#  define SEABOLT_NO_EXPORT
#else
#  ifndef SEABOLT_EXPORT
#    ifdef seabolt_EXPORTS
        /* We are building this library */
#      define SEABOLT_EXPORT 
#    else
        /* We are using this library */
#      define SEABOLT_EXPORT 
#    endif
#  endif

#  ifndef SEABOLT_NO_EXPORT
#    define SEABOLT_NO_EXPORT 
#  endif
#endif

#ifndef SEABOLT_DEPRECATED
#  define SEABOLT_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef SEABOLT_DEPRECATED_EXPORT
#  define SEABOLT_DEPRECATED_EXPORT SEABOLT_EXPORT SEABOLT_DEPRECATED
#endif

#ifndef SEABOLT_DEPRECATED_NO_EXPORT
#  define SEABOLT_DEPRECATED_NO_EXPORT SEABOLT_NO_EXPORT SEABOLT_DEPRECATED
#endif

#if 1 /* DEFINE_NO_DEPRECATED */
#  ifndef SEABOLT_NO_DEPRECATED
#    define SEABOLT_NO_DEPRECATED
#  endif
#endif

#endif /* SEABOLT_EXPORT_H */
