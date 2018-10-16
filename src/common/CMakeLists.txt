add_library(catch2 INTERFACE)

target_include_directories(catch2
        INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/tests)