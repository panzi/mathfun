include_directories("../src")

add_executable(dump dump.c)
target_link_libraries(dump mathfun_static)

add_executable(wavegen wavegen.c)
target_link_libraries(wavegen mathfun_static)

add_executable(eval eval.c)
target_link_libraries(eval mathfun_static)

add_executable(evaltree evaltree.c)
target_link_libraries(evaltree mathfun_static)

if(WITH_WAVEGEN_INSTALL)
	install(TARGETS wavegen RUNTIME DESTINATION bin)
endif()
