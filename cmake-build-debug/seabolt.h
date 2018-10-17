
#ifndef message_EXPORT_H
#define message_EXPORT_H

#ifdef message_STATIC_DEFINE
#  define message_EXPORT
#  define message_NO_EXPORT
#else
#  ifndef message_EXPORT
#    ifdef seabolt_EXPORTS
        /* We are building this library */
#      define message_EXPORT 
#    else
        /* We are using this library */
#      define message_EXPORT 
#    endif
#  endif

#  ifndef message_NO_EXPORT
#    define message_NO_EXPORT 
#  endif
#endif

#ifndef message_DEPRECATED
#  define message_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef message_DEPRECATED_EXPORT
#  define message_DEPRECATED_EXPORT message_EXPORT message_DEPRECATED
#endif

#ifndef message_DEPRECATED_NO_EXPORT
#  define message_DEPRECATED_NO_EXPORT message_NO_EXPORT message_DEPRECATED
#endif

#if 1 /* DEFINE_NO_DEPRECATED */
#  ifndef message_NO_DEPRECATED
#    define message_NO_DEPRECATED
#  endif
#endif

#endif /* message_EXPORT_H */
