aux_source_directory(. SRCS)
add_executable(lss ${SRCS})

add_dependencies(base jsoncpp)
target_link_libraries(lss base jsoncpp_static.a)
install(TARGETS lss RUNTIME DESTINATION sbin)