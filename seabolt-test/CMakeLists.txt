cmake_minimum_required(VERSION 3.7)
project(seabolt-test CXX)

if (CMAKE_BUILD_TYPE MATCHES Debug)

    include_directories(include)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    file(GLOB HPP_FILES include/*.hpp)
    file(GLOB CPP_FILES src/*.cpp)
    include_directories(${seabolt_INCLUDE_DIRS})
    add_executable(${PROJECT_NAME} ${HPP_FILES} ${CPP_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "seabolt-test" SUFFIX "")
    target_link_libraries(${PROJECT_NAME} seabolt)

endif()
