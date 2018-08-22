cmake_minimum_required(VERSION 3.5)
project(seabolt-integration-test CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin")
# To have MSVC generate its outputs independent of the configuration
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin" )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

if (MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/W4 /Zi /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_CXX_FLAGS_RELEASE "/Ox /W4 /D_CRT_SECURE_NO_WARNINGS")
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
endif()

file(GLOB HPP_FILES include/*.hpp)
file(GLOB CPP_FILES src/*.cpp)
include_directories(${seabolt_INCLUDE_DIRS})
include_directories(${seabolt_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} ${HPP_FILES} ${CPP_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "seabolt-integration-test")
target_link_libraries(${PROJECT_NAME} seabolt)
