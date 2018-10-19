list(APPEND private_source_files
        ${CMAKE_CURRENT_LIST_DIR}/address-resolver.c
        ${CMAKE_CURRENT_LIST_DIR}/address-set.c
        ${CMAKE_CURRENT_LIST_DIR}/address.c
        ${CMAKE_CURRENT_LIST_DIR}/auth.c
        ${CMAKE_CURRENT_LIST_DIR}/buffering.c
        ${CMAKE_CURRENT_LIST_DIR}/connections.c
        ${CMAKE_CURRENT_LIST_DIR}/connector.c
        ${CMAKE_CURRENT_LIST_DIR}/direct-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/error.c
        ${CMAKE_CURRENT_LIST_DIR}/lifecycle.c
        ${CMAKE_CURRENT_LIST_DIR}/logging.c
        ${CMAKE_CURRENT_LIST_DIR}/mem.c
        ${CMAKE_CURRENT_LIST_DIR}/packstream.c
        ${CMAKE_CURRENT_LIST_DIR}/platform.c
        ${CMAKE_CURRENT_LIST_DIR}/protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/routing-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/routing-table.c
        ${CMAKE_CURRENT_LIST_DIR}/tls.c
        ${CMAKE_CURRENT_LIST_DIR}/utils.c
        ${CMAKE_CURRENT_LIST_DIR}/v1.c
        ${CMAKE_CURRENT_LIST_DIR}/v2.c
        ${CMAKE_CURRENT_LIST_DIR}/v3.c
        ${CMAKE_CURRENT_LIST_DIR}/values.c)

list(APPEND public_header_files
        ${CMAKE_CURRENT_LIST_DIR}/bolt.h
        ${CMAKE_CURRENT_LIST_DIR}/address.h
        ${CMAKE_CURRENT_LIST_DIR}/address-set.h
        ${CMAKE_CURRENT_LIST_DIR}/address-resolver.h
        ${CMAKE_CURRENT_LIST_DIR}/auth.h
        ${CMAKE_CURRENT_LIST_DIR}/config.h
        ${CMAKE_CURRENT_LIST_DIR}/connections.h
        ${CMAKE_CURRENT_LIST_DIR}/connector.h
        ${CMAKE_CURRENT_LIST_DIR}/error.h
        ${CMAKE_CURRENT_LIST_DIR}/lifecycle.h
        ${CMAKE_CURRENT_LIST_DIR}/logging.h
        ${CMAKE_CURRENT_LIST_DIR}/mem.h
        ${CMAKE_CURRENT_LIST_DIR}/values.h)

target_sources(${SEABOLT_SHARED}
        PRIVATE
        ${private_source_files}
        $<TARGET_OBJECTS:string-builder>)

target_include_directories(${SEABOLT_SHARED}
        PRIVATE
        $<TARGET_PROPERTY:string-builder,INTERFACE_INCLUDE_DIRECTORIES>)

target_compile_definitions(${SEABOLT_SHARED}
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
    if (ON_WINDOWS)
        target_link_libraries(${SEABOLT_SHARED}
                PUBLIC
                crypt32)
    endif ()

    target_include_directories(${SEABOLT_SHARED}
            PRIVATE
            ${OPENSSL_SHARED_INCLUDE_DIR})
endif ()

if (CMAKE_C_COMPILER_ID MATCHES AppleClang)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/platform.c
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
        ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-version.h
        @ONLY
)

include(GenerateExportHeader)
generate_export_header(${SEABOLT_SHARED}
        BASE_NAME seabolt
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-exports.h"
        DEFINE_NO_DEPRECATED)
target_include_directories(${SEABOLT_SHARED}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}>
        $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>)

set_target_properties(${SEABOLT_SHARED}
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        SOVERSION 1
        OUTPUT_NAME "${SEABOLT_NAME}"
        PUBLIC_HEADER "${public_header_files};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-exports.h;${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt-version.h"
        MACOSX_RPATH ON
        SKIP_BUILD_RPATH OFF
        BUILD_WITH_INSTALL_RPATH OFF
        WINDOWS_EXPORT_ALL_SYMBOLS OFF)
