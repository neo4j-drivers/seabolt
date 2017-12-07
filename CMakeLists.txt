cmake_minimum_required(VERSION 3.7)
project(seabolt-all)

enable_testing()

set(CMAKE_C_STANDARD 11)

add_custom_target(seabolt-all)
add_dependencies(seabolt-all seabolt seabolt-test)

add_subdirectory(seabolt)
add_subdirectory(seabolt-test)
