cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(seabolt C)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

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
file(GLOB C_FILES src/*.c src/**/*.c)
add_library(${PROJECT_NAME} SHARED ${H_FILES} ${C_FILES})

target_link_libraries(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
        SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        )

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Neo4j")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/pkg")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_GENERATOR "TGZ;TXZ;ZIP")
include (CPack)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${H_FILES} DESTINATION include)
