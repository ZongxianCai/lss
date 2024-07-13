aux_source_directory(. DIR_LIB_SRCS)
add_library (base ${DIR_LIB_SRCS})

add_executable(base_test ${DIR_LIB_SRCS})
target_link_libraries(base_test base jsoncpp_static.a)