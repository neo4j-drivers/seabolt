include(CheckSymbolExists)

set_target_properties(${SEABOLT_SHARED}
        PROPERTIES
        SOVERSION 1
        OUTPUT_NAME "${SEABOLT_NAME}"
        WINDOWS_EXPORT_ALL_SYMBOLS OFF)

set_target_properties(${SEABOLT_STATIC}
        PROPERTIES
        OUTPUT_NAME "${SEABOLT_STATIC_NAME}")

list(APPEND private_source_files
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-resolver.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address-set.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/address.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/auth.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/buffering.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/config.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connection.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/connector.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/communication.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-plain.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-mock.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/direct-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/error.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/lifecycle.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/log.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/mem.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/name.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/no-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/packstream.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/protocol.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/routing-pool.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/routing-table.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/stats.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/status.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/string-builder.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/time.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v1.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v2.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/v3.c
        ${CMAKE_CURRENT_LIST_DIR}/bolt/values.c)

check_symbol_exists(timespec_get "time.h" HAVE_TIMESPEC_GET)

if (ON_POSIX)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/sync-pthread.c)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-plain-posix.c)
endif ()

if (ON_WINDOWS)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/sync-win32.c)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-plain-win32.c)
endif ()

if (HAVE_TIMESPEC_GET)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/time-timespec.c)
else ()
    if (ON_WINDOWS)
        list(APPEND private_source_files
                ${CMAKE_CURRENT_LIST_DIR}/bolt/time-win32.c)
    endif ()

    if (ON_MACOS)
        list(APPEND private_source_files
                ${CMAKE_CURRENT_LIST_DIR}/bolt/time-macos.c)
    endif ()
endif ()

if (ON_GCC)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/atomic-gcc.c)
else ()
    if (ON_WINDOWS)
        list(APPEND private_source_files
                ${CMAKE_CURRENT_LIST_DIR}/bolt/atomic-win32.c)
    endif ()

    if (ON_MACOS)
        list(APPEND private_source_files
                ${CMAKE_CURRENT_LIST_DIR}/bolt/atomic-macos.c)
    endif ()
endif ()

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

if (WITH_TLS_SUPPORT AND WITH_TLS_OPENSSL)
    find_openssl_both()

    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-secure-openssl.c)
endif ()

if (WITH_TLS_SUPPORT AND WITH_TLS_SECURE_CHANNEL)
    list(APPEND private_source_files
            ${CMAKE_CURRENT_LIST_DIR}/bolt/communication-secure-schannel.c)
endif ()

if (ON_POSIX)
    set(CMAKE_THREAD_PREFER_PTHREAD ON)
    find_package(Threads REQUIRED)
endif ()

add_custom_target(
        write_version_info
        ALL
        BYPRODUCTS
        ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h
        COMMAND
        ${CMAKE_COMMAND}
        -D INPUT_FILE=${CMAKE_CURRENT_LIST_DIR}/version.h.in
        -D OUTPUT_FILE=${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h
        -D VERSION=${PROJECT_VERSION}
        -D VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D VERSION_PATCH=${PROJECT_VERSION_PATCH}
        -D VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
        -P ${PROJECT_SOURCE_DIR}/cmake/GitHash.cmake
        WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
)

foreach (target ${SEABOLT_SHARED} ${SEABOLT_STATIC})
    target_sources(${target}
            PRIVATE
            ${private_source_files})

    target_compile_definitions(${target}
            PUBLIC
            IS_BIG_ENDIAN=$<BOOL:${IS_BIG_ENDIAN}>
            USE_WINSOCK=$<BOOL:${ON_WINDOWS}>
            USE_POSIXSOCK=$<BOOL:${ON_POSIX}>
            USE_WINSSPI=$<BOOL:${WITH_TLS_SECURE_CHANNEL}>
            USE_OPENSSL=$<BOOL:${WITH_TLS_OPENSSL}>
            INTERFACE
            $<INSTALL_INTERFACE:USING_seabolt>)

    if (ON_POSIX)
        target_link_libraries(${target}
                PUBLIC
                ${CMAKE_THREAD_LIBS_INIT})
    endif ()

    if (ON_WINDOWS)
        target_link_libraries(${target}
                PUBLIC
                ws2_32)

        target_compile_definitions(${target}
                PRIVATE
                _WIN32_WINNT=0x0601)
    endif ()

    if (WITH_TLS_SUPPORT AND WITH_TLS_OPENSSL)
        target_include_directories(${target}
                PRIVATE
                ${OPENSSL_SHARED_INCLUDE_DIR})

        if (ON_WINDOWS)
            target_link_libraries(${target}
                    PUBLIC
                    crypt32)
        endif ()
    endif ()

    if (WITH_TLS_SUPPORT AND WITH_TLS_SECURE_CHANNEL)
        target_compile_definitions(${target}
                PRIVATE
                SECURITY_WIN32)

        target_link_libraries(${target}
                PUBLIC
                secur32 crypt32)
    endif ()

    target_include_directories(${target}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt>
            $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>)

    set_target_properties(${target}
            PROPERTIES
            POSITION_INDEPENDENT_CODE 1
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1
            PUBLIC_HEADER "${public_header_files};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-exports.h;${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-version.h")

    add_dependencies(${target} write_version_info)
endforeach ()

if (WITH_TLS_SUPPORT AND WITH_TLS_OPENSSL)
    target_link_libraries(${SEABOLT_SHARED}
            PRIVATE
            ${OPENSSL_SHARED_LIBRARIES})

    target_link_libraries(${SEABOLT_STATIC}
            PUBLIC
            ${OPENSSL_STATIC_LIBRARIES})

    if (ON_POSIX)
        include(FindPkgConfig REQUIRED)
        pkg_check_modules(OPENSSLDEPS openssl)

        target_link_libraries(${SEABOLT_SHARED}
                PRIVATE
                ${OPENSSLDEPS_LIBRARIES})

        target_link_libraries(${SEABOLT_STATIC}
                PUBLIC
                ${OPENSSLDEPS_STATIC_LIBRARIES})
    endif ()
endif ()

if (CMAKE_C_COMPILER_ID MATCHES AppleClang)
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/bolt/atomic-macos.c
            PROPERTIES
            COMPILE_FLAGS -Wno-deprecated-declarations)
endif ()

include(GenerateExportHeader)
generate_export_header(${SEABOLT_SHARED}
        BASE_NAME seabolt
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/bolt/bolt-exports.h")
target_compile_definitions(${SEABOLT_STATIC}
        PUBLIC
        SEABOLT_STATIC_DEFINE)

add_custom_command(
        TARGET ${SEABOLT_SHARED} POST_BUILD
        COMMAND
        ${CMAKE_COMMAND}
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D NAME=${SEABOLT_NAME}
        -D VERSION=${PROJECT_VERSION}
        -D LIB_DIR=${LIBDIR_RELATIVE_PKGCONFIGDIR}
        -D INCLUDE_DIR=${INCLUDEDIR_RELATIVE_PKGCONFIGDIR}
        -D OUTPUT_NAME=${SEABOLT_NAME}
        -D LIBRARIES="$<TARGET_PROPERTY:${SEABOLT_SHARED},INTERFACE_LINK_LIBRARIES>"
        -D INPUT_FILE=${PROJECT_SOURCE_DIR}/cmake/seabolt.pc.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_PKGCONFIGDIR}/${SEABOLT_NAME}.pc
        -P ${PROJECT_SOURCE_DIR}/cmake/PkgConfigFiles.cmake
)

if (NOT ON_WINDOWS OR WITH_TLS_SECURE_CHANNEL)
    add_custom_command(
            TARGET ${SEABOLT_STATIC} POST_BUILD
            COMMAND
            ${CMAKE_COMMAND}
            -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -D NAME=${SEABOLT_NAME}-static
            -D VERSION=${PROJECT_VERSION}
            -D LIB_DIR=${LIBDIR_RELATIVE_PKGCONFIGDIR}
            -D INCLUDE_DIR=${INCLUDEDIR_RELATIVE_PKGCONFIGDIR}
            -D OUTPUT_NAME=$<TARGET_FILE_NAME:${SEABOLT_STATIC}>
            -D LIBRARIES="$<TARGET_PROPERTY:${SEABOLT_STATIC},INTERFACE_LINK_LIBRARIES>"
            -D INPUT_FILE=${PROJECT_SOURCE_DIR}/cmake/seabolt-static.pc.in
            -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_PKGCONFIGDIR}/${SEABOLT_NAME}-static.pc
            -P ${PROJECT_SOURCE_DIR}/cmake/PkgConfigFiles.cmake
    )
endif ()
