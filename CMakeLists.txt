cmake_minimum_required(VERSION 3.5)
project(seabolt-all)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_custom_target(seabolt-all)
add_dependencies(seabolt-all seabolt seabolt-cli)

add_subdirectory(seabolt)
add_subdirectory(seabolt-cli)
add_subdirectory(seabolt-test)
