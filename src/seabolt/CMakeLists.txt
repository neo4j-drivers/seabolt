if (MSVC)
	add_compile_options(/W4 /WX)
else()
	add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

if (ON_WINDOWS)
    if (CMAKE_C_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES GNU)
         add_compile_options(-Wno-pedantic-ms-format)
    endif()
endif()

add_library(seabolt "")

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/tests/CMakeLists.txt)
