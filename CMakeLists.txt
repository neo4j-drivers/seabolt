cmake_minimum_required(VERSION 3.5)
project(seabolt-all)

add_custom_target(seabolt-all)
add_dependencies(seabolt-all seabolt seabolt-cli)

add_subdirectory(seabolt)
add_subdirectory(seabolt-cli)
add_subdirectory(seabolt-test)
