add_library(catch2 INTERFACE)

target_include_directories(catch2
        INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/tests)

add_library(string-builder
        OBJECT
        ${CMAKE_CURRENT_LIST_DIR}/utils/string-builder.c)

target_include_directories(string-builder
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/utils)
