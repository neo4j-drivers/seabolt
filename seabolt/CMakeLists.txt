cmake_minimum_required(VERSION 3.7)
set(CMAKE_C_STANDARD 99)
project(seabolt C)
include_directories(include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")

if ( WIN32 )
    # assume OpenSSL is installed in %PROGRAMFILES(X86)%\OpenSSL
    include_directories("$ENV{PROGRAMFILES\(X86\)}\\OpenSSL\\include")
    link_libraries("libssl.lib" "libcrypto.lib")
    link_directories("$ENV{PROGRAMFILES\(X86\)}\\OpenSSL\\lib")
elseif ( APPLE )
    # assume OpenSSL is installed in /usr/local/opt/openssl/
    include_directories("/usr/local/opt/openssl/include")
    link_libraries("libssl.dylib" "libcrypto.dylib")
    link_directories("/usr/local/opt/openssl/lib")
else()
    find_package(OpenSSL REQUIRED)
    if( OpenSSL_FOUND )
        include_directories(${OPENSSL_INCLUDE_DIRS})
        link_directories(${OPENSSL_LIBRARIES})
        link_libraries(${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
    endif()
endif()

file(GLOB H_FILES include/*.h)
file(GLOB C_FILES src/*.c)
add_library(${PROJECT_NAME} SHARED ${H_FILES} ${C_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
