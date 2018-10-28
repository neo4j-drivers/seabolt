message(STATUS "Passed ${LIBRARIES}")

set(LIBS_PROCESSED "")
foreach (lib ${LIBRARIES})
    if (lib MATCHES "^-l")
        string(SUBSTRING ${lib} 2 -1 lib)
    endif ()

    if (lib MATCHES ".*\\.a$")
        string(APPEND LIBS_PROCESSED "${lib} ")
    elseif (lib MATCHES ".*\\.lib$")
        string(APPEND LIBS_PROCESSED "${lib} ")
    else ()
        string(APPEND LIBS_PROCESSED "-l${lib} ")
    endif ()
endforeach ()

message(STATUS "Processed ${LIBS_PROCESSED}")

configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/seabolt.pc.in
        ${OUTPUT_FILE}
        @ONLY
)
#file(GENERATE
#        OUTPUT ${OUTPUT_FILE}
#        CONTENT "\
#name=${NAME}\n\
#libdir=\${pcfiledir}/${LIBDIR}\n\
#includedir=\${pcfiledir}/${INCLUDEDIR}\n\
#\n\
#Name: ${NAME}\n\
#Description: The C Connector Library for Neo4j\n\
#Version: ${VERSION}\n\
#CFlags: -I\${includedir}\n\
#Libs: -L\${libdir} -l\${name}\n\
#")
