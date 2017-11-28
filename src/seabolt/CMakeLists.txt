set(LIB_SOURCE_FILES values.h)
add_library(seabolt SHARED ${LIB_SOURCE_FILES})
set_target_properties(seabolt PROPERTIES LINKER_LANGUAGE C)
