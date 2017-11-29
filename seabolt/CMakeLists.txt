cmake_minimum_required(VERSION 3.7)
set(CMAKE_C_STANDARD 99)
project(seabolt)
include_directories(include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

file(GLOB H_FILES include/*.h)
file(GLOB C_FILES src/*.c)
add_library(${PROJECT_NAME} SHARED ${H_FILES} ${C_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PROJECT_NAME})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
        CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
