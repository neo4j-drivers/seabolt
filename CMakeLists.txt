cmake_minimum_required(VERSION 3.5)
project(seabolt-all)

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()

  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
endif()

add_custom_target(seabolt-all)
add_dependencies(seabolt-all seabolt seabolt-cli)

add_subdirectory(seabolt)
add_subdirectory(seabolt-cli)
add_subdirectory(seabolt-test)
add_subdirectory(seabolt-integration-test)
