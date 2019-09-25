message(STATUS "Passed ${LIBRARIES}")

set(LIBS_PROCESSED "")
foreach (lib ${LIBRARIES})
    if (lib MATCHES "^\\$")
        continue()
    endif ()

    if (lib MATCHES ">$")
        continue()
    endif ()

    if (lib MATCHES "Threads::Threads")
        continue()
    endif ()

    if (lib MATCHES "^-l")
        string(SUBSTRING ${lib} 2 -1 lib)
    endif ()

    if (lib MATCHES ".*\\.a$")
        string(APPEND LIBS_PROCESSED "${lib} ")
    elseif (lib MATCHES ".*\\.so")
        string(APPEND LIBS_PROCESSED "${lib} ")
    elseif (lib MATCHES ".*\\.dylib")
        string(APPEND LIBS_PROCESSED "${lib} ")
    elseif (lib MATCHES ".*\\.lib$")
        string(APPEND LIBS_PROCESSED "-l${lib} ")
    else ()
        string(APPEND LIBS_PROCESSED "-l${lib} ")
    endif ()
endforeach ()

message(STATUS "Processed ${LIBS_PROCESSED}")

configure_file(
        ${INPUT_FILE}
        ${OUTPUT_FILE}
        @ONLY
)
