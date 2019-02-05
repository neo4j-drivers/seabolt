# in case Git is not available, we default to "unknown"
set(VERSION_HASH "unknown")

if (DEFINED ENV{SEABOLT_VERSION_HASH})
    set(VERSION_HASH $ENV{SEABOLT_VERSION_HASH})
    message(STATUS "Using Git hash from environment: ${VERSION_HASH}")
else ()
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

    message(STATUS "Using Git hash from git command: ${VERSION_HASH}")
endif ()

get_filename_component(OUTPUT_FILE_NAME ${OUTPUT_FILE} NAME)
set(OUTPUT_TMP_FILE "${OUTPUT_FILE_NAME}.tmp")

# generate a temporary outputfile based on input file
configure_file(
        ${INPUT_FILE}
        ${OUTPUT_TMP_FILE}
        @ONLY
)

if (EXISTS ${OUTPUT_FILE})
    file(SHA256 ${OUTPUT_FILE} OLD_CONTENT_HASH)
else ()
    set(OLD_CONTENT_HASH "")
endif ()

file(SHA256 ${OUTPUT_TMP_FILE} NEW_CONTENT_HASH)

if (OLD_CONTENT_HASH STREQUAL NEW_CONTENT_HASH)
    # let's not copy contents because they're equal
else ()
    configure_file(
            ${INPUT_FILE}
            ${OUTPUT_FILE}
            @ONLY
    )
endif ()

file(REMOVE ${OUTPUT_TMP_FILE})
