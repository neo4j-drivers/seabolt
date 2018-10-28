# in case Git is not available, we default to "unknown"
set(VERSION_HASH "unknown")

# find Git and if available set GIT_HASH variable
find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
            OUTPUT_VARIABLE VERSION_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
endif ()

message(STATUS "Git hash is ${VERSION_HASH}")

# generate file version.hpp based on version.hpp.in
configure_file(
        ${INPUT_FILE}
        ${OUTPUT_FILE}
        @ONLY
)
