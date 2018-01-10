cmake_minimum_required(VERSION 3.5)
project(seabolt-cli)
include_directories(include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin")

file(GLOB H_FILES include/*.h)
file(GLOB C_FILES src/*.c)
include_directories(${seabolt_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${H_FILES} ${C_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "seabolt" SUFFIX "")
target_link_libraries(${PROJECT_NAME} seabolt)
