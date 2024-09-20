aux_source_directory(. SRCS)
add_executable(lss ${SRCS})

add_dependencies(base jsoncpp)
target_link_libraries(lss base network mmedia live jsoncpp_static.a crypto)
install(TARGETS lss RUNTIME DESTINATION sbin)