cmake_minimum_required(VERSION 3.5)
project(seabolt-test CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)

    include_directories(include)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin")

    file(GLOB HPP_FILES include/*.hpp)
    file(GLOB CPP_FILES src/*.cpp)
    include_directories(${seabolt_INCLUDE_DIRS})
    add_executable(${PROJECT_NAME} ${HPP_FILES} ${CPP_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "seabolt-test")
    target_link_libraries(${PROJECT_NAME} seabolt)

endif()
