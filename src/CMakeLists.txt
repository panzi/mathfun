set(MATHFUN_SRCS bindings.c codegen.c exec.c mathfun.c parser.c)

add_library(mathfun_objs OBJECT ${MATHFUN_SRCS})
#add_library(mathfun SHARED $<TARGET_OBJECTS:mathfun_objs>)

#target_link_libraries(mathfun ${M_LIBRARY})

if(WITH_STATIC_LIB)
	add_library(mathfun_static STATIC $<TARGET_OBJECTS:mathfun_objs>)
	target_link_libraries(mathfun_static ${M_LIBRARY})

	install(TARGETS mathfun_static DESTINATION "lib")
endif()

#install(TARGETS mathfun DESTINATION "lib")
install(FILES mathfun.h DESTINATION "include")
