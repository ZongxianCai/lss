aux_source_directory(. SRCS)
add_executable(lss ${SRCS})

install(TARGETS lss RUNTIME DESTINATION sbin)