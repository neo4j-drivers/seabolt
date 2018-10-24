add_executable(seabolt-cli "")

target_sources(seabolt-cli
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c)

target_link_libraries(seabolt-cli
        PRIVATE
        ${SEABOLT_SHARED})
