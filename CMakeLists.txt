cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(seabolt LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build seabolt as a shared library" OFF)

set(ON_WINDOWS OFF)
set(ON_MACOS OFF)
set(ON_POSIX OFF)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ON_WINDOWS ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(ON_MACOS ON)
endif ()

include(CheckIncludeFile)
check_include_file("unistd.h" HAVE_UNISTD_H)
if (HAVE_UNISTD_H)
    set(ON_POSIX ON)
endif ()

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    set(BIG_ENDIAN 1)
else ()
    set(BIG_ENDIAN 0)
endif ()

include(CMakeDependentOption)
option(WITH_TLS_SUPPORT "Build seabolt with TLS support" ON)
cmake_dependent_option(WITH_TLS_LIBRESSL "Use LIBRESSL for TLS support" OFF "WITH_TLS_SUPPORT" ON)
cmake_dependent_option(WITH_TLS_SECURE_CHANNEL "Use Windows Secure Channel for TLS support" OFF "WITH_TLS_SUPPORT" ON)
cmake_dependent_option(WITH_TLS_SECURE_TRANSPORT "Use MacOS Secure Transport for TLS support " OFF "WITH_TLS_SUPPORT" ON)
cmake_dependent_option(WITH_TLS_OPENSSL "Use OPENSSL for TLS support" ON "WITH_TLS_SUPPORT;NOT WITH_TLS_LIBRESSL;NOT WITH_TLS_SECURE_CHANNEL;NOT WITH_TLS_SECURE_TRANSPORT" OFF)

if (WITH_TLS_SUPPORT)
    message(STATUS "Building seabolt with TLS support")

    if (WITH_TLS_OPENSSL)
        message(STATUS "Using OPENSSL for TLS")
    endif ()

    if (WITH_TLS_LIBRESSL)
        message(STATUS "Using LIBRESSL for TLS")
    endif ()

    if (WITH_TLS_SECURE_CHANNEL)
        message(STATUS "Using Windows Secure Channel for TLS")
    endif ()

    if (WITH_TLS_SECURE_TRANSPORT)
        message(STATUS "Using MacOS Secure Transport for TLS")
    endif ()

    if (WITH_TLS_SECURE_CHANNEL AND NOT ON_WINDOWS)
        message(FATAL_ERROR "Windows Secure Channel can only be enabled when building on Windows")
    endif ()

    if (WITH_TLS_SECURE_TRANSPORT AND NOT ON_MACOS)
        message(FATAL_ERROR "MacOS Secure Transport can only be enabled when building on MacOS")
    endif ()
endif ()

enable_testing()
add_subdirectory(src)