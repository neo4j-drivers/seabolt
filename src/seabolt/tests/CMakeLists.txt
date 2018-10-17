add_executable(seabolt-test
        ${CMAKE_CURRENT_LIST_DIR}/integration.hpp
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/seabolt.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-address-set.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-addressing.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-chunking-v1.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-direct.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-pooling.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-values.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test-warden.cpp
        )

target_include_directories(seabolt-test
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(seabolt-test
        PRIVATE
        seabolt
        catch2
        string-builder)

include(ParseAndAddCatchTests)
set(PARSE_CATCH_TESTS_ADD_TO_CONFIGURE_DEPENDS ON)
ParseAndAddCatchTests(seabolt-test)

