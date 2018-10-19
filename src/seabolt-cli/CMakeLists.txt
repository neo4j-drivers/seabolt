add_executable(seabolt-cli "")

target_sources(seabolt-cli
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        $<TARGET_OBJECTS:string-builder>)

target_link_libraries(seabolt-cli
        PRIVATE
        ${SEABOLT_SHARED})

target_include_directories(seabolt-cli
        PRIVATE
        $<TARGET_PROPERTY:string-builder,INTERFACE_INCLUDE_DIRECTORIES>)

set_target_properties(seabolt-cli
        PROPERTIES
        MACOSX_RPATH ON
        SKIP_BUILD_RPATH OFF
        BUILD_WITH_INSTALL_RPATH OFF)
