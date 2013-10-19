cmake_minimum_required(VERSION 2.4.4)

project(mathfun)

set(MATHFUN_MAJOR 1)
set(MATHFUN_MINOR 0)
set(MATHFUN_PATCH 0)
set(MATHFUN_NAME mathfun${MATHFUN_MAJOR}${MATHFUN_MINOR})
set(MATHFUN_VERSION ${MATHFUN_MAJOR}.${MATHFUN_MINOR}.${MATHFUN_PATCH})

option(WITH_STATIC_LIB "Also build a static libraray (needed by examples)" ON)
option(WITH_EXAMPLES "Build examples" OFF)
option(WITH_WAVEGEN_INSTALL "Install wavegen example" OFF)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=gnu99")

	if(NOT(CMAKE_BUILD_TYPE STREQUAL "Debug"))
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
	endif()

	if(NOT(CMAKE_COMPILER_IS_MINGW64))
		# can't be pedantic for mingw64 because of format strings
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
	endif()
endif()

if(NOT WIN32)
	find_library(M_LIBRARY
		NAMES m
		PATHS /usr/lib /usr/local/lib)
	if(NOT M_LIBRARY)
		message(STATUS "math library 'libm' not found")
	endif()
else()
	# not needed on windows
	set(M_LIBRARY "")
endif()

add_subdirectory(src)

if(WITH_EXAMPLES)
	add_subdirectory(examples)
endif()

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
