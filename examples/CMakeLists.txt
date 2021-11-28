include_directories("${PROJECT_SOURCE_DIR}/src")

set(MATHFUN_EXAMPLES dump eval evaltree wavegen livewave)

foreach(example ${MATHFUN_EXAMPLES})
	add_executable(${example} ${example}.c)
	target_link_libraries(${example} ${MATHFUN_LIB_NAME})
endforeach()

if(INSTALL_WAVEGEN)
	install(TARGETS wavegen RUNTIME DESTINATION "bin")
endif()
