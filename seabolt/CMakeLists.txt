cmake_minimum_required(VERSION 3.5)

project(seabolt C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
	set(CMAKE_C_FLAGS_DEBUG "-DEBUG:FULL /D_DEBUG /W4")
	set(CMAKE_C_FLAGS_RELEASE "/Ox -DEBUG:NONE /W4")
else()
	set(CMAKE_C_FLAGS_DEBUG "-g -D_DEBUG -Wall")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
endif()

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_TWEAK beta.1)

include_directories(include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/lib")

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
	set(BIG_ENDIAN 1)
else()
	set(BIG_ENDIAN 0)
endif()

# Configure socket & TLS library
set(OPENSSL 1)
set(WINSOCK 0)
set(POSIXSOCK 1)
set(WINSSPI 0)
set(PTHREAD 1)
if ( WIN32 )
	set(OPENSSL 1)
	set(POSIXSOCK 0)
	set(WINSOCK 1)
	set(WINSSPI 0)
	set(PTHREAD 0)
endif ()

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/bolt/config-options.h.in"
  "${PROJECT_SOURCE_DIR}/include/bolt/config-options.h"
  )


file(GLOB H_FILES include/bolt/*.h)
file(GLOB C_FILES src/bolt/*.c src/bolt/**/*.c)
add_library(${PROJECT_NAME} SHARED ${H_FILES} ${C_FILES})

target_link_libraries(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
        SOVERSION 0
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${PROJECT_VERSION_TWEAK}"
        )

if ( ${WINSOCK} )
	target_link_libraries(${PROJECT_NAME} "ws2_32.lib")
endif ()

if ( ${WINSSPI} )
	target_link_libraries(${PROJECT_NAME} "fwpuclnt.lib")
endif ()

if ( ${OPENSSL} )
	if ( WIN32 OR APPLE )
		if ( NOT DEFINED ENV{OPENSSL_LIB_ROOT} )
			MESSAGE(FATAL_ERROR "You did not define the environment variable OPENSSL_LIB_ROOT which is needed to locate OpenSSL dependency. Please set this variable and execute cmake again.")
		endif ()

		set(OPENSSL_INCLUDE_DIRS "$ENV{OPENSSL_LIB_ROOT}/include")
		find_library(OPENSSL_SSL_LIBRARY ssl libssl PATHS "$ENV{OPENSSL_LIB_ROOT}" PATH_SUFFIXES lib NO_DEFAULT_PATH)
		find_library(OPENSSL_CRYPTO_LIBRARY crypto libcrypto PATHS "$ENV{OPENSSL_LIB_ROOT}" PATH_SUFFIXES lib NO_DEFAULT_PATH)
	else ()
		find_package(OpenSSL REQUIRED)
	endif ()

	target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
endif ()

if ( ${PTHREAD} )
	target_link_libraries(${PROJECT_NAME} "pthread")
endif ()

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Neo4j")
set(CPACK_PACKAGE_CONTACT "Neo4j Drivers Team <drivers@neo4j.com>")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/pkg")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "DEB;RPM;TGZ;TXZ;ZIP")
include (CPack)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/bolt" DESTINATION include FILES_MATCHING PATTERN "*.h")
