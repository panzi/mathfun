include_directories("../src")

set(MATHFUN_EXAMPLES dump eval evaltree wavegen)

foreach(example ${MATHFUN_EXAMPLES})
	add_executable(${example} ${example}.c)
	target_link_libraries(${example} ${MATHFUN_LIB_NAME})
endforeach()

if(WITH_WAVEGEN_INSTALL)
	install(TARGETS wavegen RUNTIME DESTINATION "bin")
endif()
