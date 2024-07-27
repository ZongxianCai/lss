aux_source_directory(. DIR_LIB_SRCS)
aux_source_directory(base DIR_LIB_SRCS)
aux_source_directory(net DIR_LIB_SRCS)

add_library(network ${DIR_LIB_SRCS})

target_link_libraries(network base)

add_subdirectory(net/tests)