list(APPEND private_source_files
        ${CMAKE_CURRENT_LIST_DIR}/address-resolver.c
        ${CMAKE_CURRENT_LIST_DIR}/address-set.h
        ${CMAKE_CURRENT_LIST_DIR}/address-set.c
        ${CMAKE_CURRENT_LIST_DIR}/address.c
        ${CMAKE_CURRENT_LIST_DIR}/auth.c
        ${CMAKE_CURRENT_LIST_DIR}/buffering.h
        ${CMAKE_CURRENT_LIST_DIR}/buffering.c
        ${CMAKE_CURRENT_LIST_DIR}/common-impl.h
        ${CMAKE_CURRENT_LIST_DIR}/config-impl.h
        ${CMAKE_CURRENT_LIST_DIR}/connections.c
        ${CMAKE_CURRENT_LIST_DIR}/connector.c
        ${CMAKE_CURRENT_LIST_DIR}/direct-pool.h
        ${CMAKE_CURRENT_LIST_DIR}/direct-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/error.c
        ${CMAKE_CURRENT_LIST_DIR}/lifecycle.c
        ${CMAKE_CURRENT_LIST_DIR}/logging.c
        ${CMAKE_CURRENT_LIST_DIR}/mem.c
        ${CMAKE_CURRENT_LIST_DIR}/packstream.h
        ${CMAKE_CURRENT_LIST_DIR}/packstream.c
        ${CMAKE_CURRENT_LIST_DIR}/platform.h
        ${CMAKE_CURRENT_LIST_DIR}/platform.c
        ${CMAKE_CURRENT_LIST_DIR}/protocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/routing-pool.h
        ${CMAKE_CURRENT_LIST_DIR}/routing-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/routing-table.h
        ${CMAKE_CURRENT_LIST_DIR}/routing-table.c
        ${CMAKE_CURRENT_LIST_DIR}/tls.h
        ${CMAKE_CURRENT_LIST_DIR}/tls.c
        ${CMAKE_CURRENT_LIST_DIR}/utils.h
        ${CMAKE_CURRENT_LIST_DIR}/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/v1.h
        ${CMAKE_CURRENT_LIST_DIR}/v1.c
        ${CMAKE_CURRENT_LIST_DIR}/v2.h
        ${CMAKE_CURRENT_LIST_DIR}/v2.c
        ${CMAKE_CURRENT_LIST_DIR}/v3.h
        ${CMAKE_CURRENT_LIST_DIR}/v3.c
        ${CMAKE_CURRENT_LIST_DIR}/values.c)

list(APPEND public_source_files
        ${CMAKE_CURRENT_LIST_DIR}/bolt.h
        ${CMAKE_CURRENT_LIST_DIR}/address-resolver.h
        ${CMAKE_CURRENT_LIST_DIR}/address.h
        ${CMAKE_CURRENT_LIST_DIR}/auth.h
        ${CMAKE_CURRENT_LIST_DIR}/config.h
        ${CMAKE_CURRENT_LIST_DIR}/connections.h
        ${CMAKE_CURRENT_LIST_DIR}/connector.h
        ${CMAKE_CURRENT_LIST_DIR}/error.h
        ${CMAKE_CURRENT_LIST_DIR}/lifecycle.h
        ${CMAKE_CURRENT_LIST_DIR}/logging.h
        ${CMAKE_CURRENT_LIST_DIR}/mem.h
        ${CMAKE_CURRENT_LIST_DIR}/values.h)

target_sources(seabolt
        PRIVATE
        ${private_source_files}
        PUBLIC
        ${public_source_files})

target_link_libraries(seabolt
        PRIVATE
        string-builder)

target_compile_definitions(seabolt
        PUBLIC
        IS_BIG_ENDIAN=$<BOOL:${IS_BIG_ENDIAN}>
        USE_WINSOCK=$<BOOL:${ON_WINDOWS}>
        USE_POSIXSOCK=$<BOOL:${ON_POSIX}>
        USE_WINSSPI=$<BOOL:${WITH_TLS_SECURE_CHANNEL}>
        USE_OPENSSL=$<BOOL:${WITH_TLS_OPENSSL}>)

target_include_directories(seabolt
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

if (ON_POSIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(seabolt
            PUBLIC
            Threads::Threads)
endif ()

if (ON_WINDOWS)
    target_link_libraries(seabolt
            PUBLIC
            ws2_32)
endif ()

if (WITH_TLS_SUPPORT AND WITH_TLS_OPENSSL)
    if (NOT BUILD_SHARED_LIBS)
        set(OPENSSL_USE_STATIC_LIBS ON)
        if (ON_WINDOWS AND MSVC)
            set(OPENSSL_MSVC_STATIC_RT ON)
        endif ()
    endif ()

    find_package(OpenSSL REQUIRED)

    target_link_libraries(seabolt
            PUBLIC
            ${OPENSSL_LIBRARIES}
            ${CMAKE_DL_LIBS})
    if (ON_WINDOWS)
        target_link_libraries(seabolt
                PUBLIC
                crypt32)
    endif ()

    target_include_directories(seabolt
            PRIVATE
            ${OPENSSL_INCLUDE_DIR})
endif ()

if (CMAKE_C_COMPILER_ID MATCHES AppleClang)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/platform.c
            PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
endif ()


include(GenerateExportHeader)
generate_export_header(seabolt
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-exports.h"
        DEFINE_NO_DEPRECATED)
target_include_directories(seabolt
        PUBLIC
        ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR})

set_target_properties(seabolt
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        SOVERSION 17
        OUTPUT_NAME "seabolt"
        PUBLIC_HEADER "${public_source_files};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-exports.h"
        MACOSX_RPATH ON
        WINDOWS_EXPORT_ALL_SYMBOLS OFF)

if (MSVC AND NOT BUILD_SHARED_LIBS)
    set_target_properties(seabolt
        PROPERTIES
            OUTPUT_NAME "seabolt-static")
endif ()