cmake_minimum_required(VERSION 3.5)

project(seabolt-cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
	set(CMAKE_C_FLAGS_DEBUG "/DEBUG:FULL /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_C_FLAGS_RELEASE "/Ox /DEBUG:NONE /D_CRT_SECURE_NO_WARNINGS")
else()
	set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin")

file(GLOB C_FILES src/main.c)
include_directories(${seabolt_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${H_FILES} ${C_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "seabolt")
target_link_libraries(${PROJECT_NAME} seabolt)
