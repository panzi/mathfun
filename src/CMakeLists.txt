configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

set(MATHFUN_SRCS bindings.c codegen.c exec.c mathfun.c parser.c error.c
	mathfun.h mathfun_intern.h config.h.in)

add_library(${MATHFUN_LIB_NAME} ${MATHFUN_SRCS})
target_link_libraries(${MATHFUN_LIB_NAME} ${M_LIBRARY})

install(TARGETS ${MATHFUN_LIB_NAME} DESTINATION "lib")
install(FILES mathfun.h DESTINATION "include/mathfun")
