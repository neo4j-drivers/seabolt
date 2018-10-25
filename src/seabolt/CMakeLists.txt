add_library(${SEABOLT_SHARED} SHARED "")
add_library(${SEABOLT_STATIC} STATIC "")
add_executable(${SEABOLT_TEST} "")

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/tests/CMakeLists.txt)
