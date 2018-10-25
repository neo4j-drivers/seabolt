list(APPEND private_source_files
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-resolver.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-set.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/auth.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/buffering.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/config.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connection.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connector.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/direct-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/error.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/lifecycle.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/log.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/mem.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/packstream.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/platform.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/routing-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/routing-table.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/stats.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/status.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/string-builder.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/tls.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v1.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v2.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v3.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/values.c)

list(APPEND public_header_files
        ${CMAKE_CURRENT_LIST_DIR}/bolt/bolt.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-set.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-resolver.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/auth.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/bolt-public.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/config.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connection.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connector.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/error.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/lifecycle.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/log.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/stats.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/status.h
        ${CMAKE_CURRENT_LIST_DIR}/bolt/values.h)

target_sources(${SEABOLT_SHARED}
        PRIVATE
        ${private_source_files})

target_sources(${SEABOLT_STATIC}
        PRIVATE
        ${private_source_files})

target_compile_definitions(${SEABOLT_SHARED}
        PUBLIC
        IS_BIG_ENDIAN=$<BOOL:${IS_BIG_ENDIAN}>
        USE_WINSOCK=$<BOOL:${ON_WINDOWS}>
        USE_POSIXSOCK=$<BOOL:${ON_POSIX}>
        USE_WINSSPI=$<BOOL:${WITH_TLS_SECURE_CHANNEL}>
        USE_OPENSSL=$<BOOL:${WITH_TLS_OPENSSL}>
        INTERFACE
        $<INSTALL_INTERFACE:USING_seabolt>)

target_compile_definitions(${SEABOLT_STATIC}
        PUBLIC
        IS_BIG_ENDIAN=$<BOOL:${IS_BIG_ENDIAN}>
        USE_WINSOCK=$<BOOL:${ON_WINDOWS}>
        USE_POSIXSOCK=$<BOOL:${ON_POSIX}>
        USE_WINSSPI=$<BOOL:${WITH_TLS_SECURE_CHANNEL}>
        USE_OPENSSL=$<BOOL:${WITH_TLS_OPENSSL}>
        INTERFACE
        $<INSTALL_INTERFACE:USING_seabolt>)

if (ON_POSIX)
    set(CMAKE_THREAD_PREFER_PTHREAD ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${SEABOLT_SHARED}
            PUBLIC
            pthread)

    target_link_libraries(${SEABOLT_STATIC}
            PUBLIC
            pthread)
endif ()

if (ON_WINDOWS)
    target_link_libraries(${SEABOLT_SHARED}
            PUBLIC
            ws2_32)
endif ()

if (WITH_TLS_SUPPORT AND WITH_TLS_OPENSSL)
    find_openssl_both()

    target_link_libraries(${SEABOLT_SHARED}
            PUBLIC
            ${OPENSSL_SHARED_LIBRARIES}
            ${CMAKE_DL_LIBS})

    target_link_libraries(${SEABOLT_STATIC}
            PUBLIC
            ${OPENSSL_STATIC_LIBRARIES}
            ${CMAKE_DL_LIBS})

    if (ON_WINDOWS)
        target_link_libraries(${SEABOLT_SHARED}
                PUBLIC
                crypt32)
    endif ()

    target_include_directories(${SEABOLT_SHARED}
            PRIVATE
            ${OPENSSL_SHARED_INCLUDE_DIR})

    target_include_directories(${SEABOLT_STATIC}
            PRIVATE
            ${OPENSSL_SHARED_INCLUDE_DIR})
endif ()

if (CMAKE_C_COMPILER_ID MATCHES AppleClang)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/bolt/platform.c
            PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
endif ()

# in case Git is not available, we default to "unknown"
set(PROJECT_VERSION_HASH "unknown")

# find Git and if available set GIT_HASH variable
find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            OUTPUT_VARIABLE PROJECT_VERSION_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
endif ()

message(STATUS "Git hash is ${PROJECT_VERSION_HASH}")

# generate file version.h based on version.h.in
configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/version.h.in
        ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h
        @ONLY
)

include(GenerateExportHeader)
generate_export_header(${SEABOLT_SHARED}
        BASE_NAME seabolt
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-exports.h"
        DEFINE_NO_DEPRECATED)
set_target_properties(${SEABOLT_STATIC}
        PROPERTIES
        COMPILE_FLAGS -DLIBSHARED_AND_STATIC_STATIC_DEFINE)

target_include_directories(${SEABOLT_SHARED}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>)

target_include_directories(${SEABOLT_STATIC}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>)

set_target_properties(${SEABOLT_SHARED}
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        SOVERSION 1
        OUTPUT_NAME "${SEABOLT_NAME}"
        PUBLIC_HEADER "${public_header_files};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-exports.h;${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h"
        WINDOWS_EXPORT_ALL_SYMBOLS OFF)

set_target_properties(${SEABOLT_STATIC}
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        OUTPUT_NAME "${SEABOLT_STATIC_NAME}"
        PUBLIC_HEADER "${public_header_files};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-exports.h;${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h")
