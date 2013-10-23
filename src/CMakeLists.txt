include(GenerateExportHeader)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

set(MATHFUN_SRCS bindings.c codegen.c exec.c mathfun.c parser.c error.c
	mathfun.h mathfun_intern.h config.h.in)

add_compiler_export_flags()
add_library(${MATHFUN_LIB_NAME} ${MATHFUN_SRCS})
generate_export_header(${MATHFUN_LIB_NAME}
	EXPORT_MACRO_NAME MATHFUN_EXPORT
	EXPORT_FILE_NAME export.h
	STATIC_DEFINE MATHFUN_STATIC_LIB)

target_link_libraries(${MATHFUN_LIB_NAME} ${M_LIBRARY})

install(TARGETS ${MATHFUN_LIB_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
	 mathfun.h
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
	"${CMAKE_CURRENT_BINARY_DIR}/export.h"
	DESTINATION "include/${MATHFUN_NAME}")
