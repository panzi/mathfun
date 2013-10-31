find_package(PkgConfig)

pkg_check_modules(CUNIT REQUIRED cunit)

include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(test_mathfun test_mathfun.c)
target_link_libraries(test_mathfun ${MATHFUN_LIB_NAME} ${CUNIT_LIBRARIES})

add_test(test_mathfun ${CMAKE_CURRENT_BINARY_DIR}/test_mathfun)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test_mathfun)
