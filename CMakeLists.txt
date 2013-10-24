cmake_minimum_required(VERSION 2.8.8)

project(mathfun)

option(BUILD_EXAMPLES "Build examples" OFF)
option(INSTALL_WAVEGEN "Install wavegen binary (from examples)" OFF)
option(BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
option(BUILD_DOCS "Build doxygen documentation" OFF)

set(MATHFUN_MAJOR_VERSION 1)
set(MATHFUN_MINOR_VERSION 0)
set(MATHFUN_PATCH_VERSION 0)

set(MATHFUN_NAME mathfun${MATHFUN_MAJOR_VERSION}${MATHFUN_MINOR_VERSION})
set(MATHFUN_VERSION ${MATHFUN_MAJOR_VERSION}.${MATHFUN_MINOR_VERSION}.${MATHFUN_PATCH_VERSION})

if(BUILD_SHARED_LIBS)
	set(MATHFUN_LIB_NAME ${MATHFUN_NAME})
else()
	set(MATHFUN_LIB_NAME ${MATHFUN_NAME}_static)
endif()

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
	set(CMAKE_INSTALL_LIBDIR "lib")
endif()

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
	if(M_LIBRARY)
		set(MATHFUN_PRIVATE_LIBS -l${M_LIBRARY})
	else()
		message(STATUS "math library 'libm' not found")
	endif()
else()
	# not needed on windows
	set(M_LIBRARY "")
endif()

# from libpng
# Set a variable with CMake code which:
# Creates a symlink from src to dest (if possible) or alternatively
# copies if different.
macro(mathfun_generate_symlink_code CODE SRC DEST)
	if(WIN32 AND NOT CYGWIN)
		set(_mathfun_gsc_message "Copying ${SRC} to ${DEST} if needed")
		set(_mathfun_gsc_operation "copy_if_different")
	else()
		set(_mathfun_gsc_message "Symlinking ${SRC} to ${DEST}")
		set(_mathfun_gsc_operation "create_symlink")
	endif()

	set(${CODE} "
		message(STATUS \"${_mathfun_gsc_message}\")
		execute_process(COMMAND \${CMAKE_COMMAND} -E ${_mathfun_gsc_operation}
			\"${SRC}\" \"${DEST}\")
		")
endmacro()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mathfun.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/${MATHFUN_NAME}.pc @ONLY)

mathfun_generate_symlink_code(MATHFUN_PC_INSTALL_CODE
  ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
  ${CMAKE_CURRENT_BINARY_DIR}/mathfun.pc)
install(CODE ${MATHFUN_PC_INSTALL_CODE})
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/mathfun.pc
	${CMAKE_CURRENT_BINARY_DIR}/${MATHFUN_NAME}.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# for config.h
include_directories("${PROJECT_BINARY_DIR}/src")

add_subdirectory(src)

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(BUILD_DOCS)
	find_package(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR
			"Doxygen is needed to build the documentation.")
	endif()

	configure_file(Doxyfile.in 
		"${PROJECT_BINARY_DIR}/Doxyfile" @ONLY)

	add_custom_target(docs ALL
		COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
		SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif()

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")
