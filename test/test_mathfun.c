#include <CUnit/Basic.h>
#include <CUnit/TestRun.h>
#include <mathfun.h>
#include <stdlib.h>

#define STRINGIFY(arg)  STRINGIFY1(arg)
#define STRINGIFY1(arg) STRINGIFY2(arg)
#define STRINGIFY2(arg) #arg

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define STRINGIFY_LIST_0()
#define STRINGIFY_LIST_1(x) STRINGIFY(x)
#define STRINGIFY_LIST_2(x, ...) STRINGIFY(x), STRINGIFY_LIST_1(__VA_ARGS__)
#define STRINGIFY_LIST_3(x, ...) STRINGIFY(x), STRINGIFY_LIST_2(__VA_ARGS__)
#define STRINGIFY_LIST_4(x, ...) STRINGIFY(x), STRINGIFY_LIST_3(__VA_ARGS__)
#define STRINGIFY_LIST_5(x, ...) STRINGIFY(x), STRINGIFY_LIST_4(__VA_ARGS__)
#define STRINGIFY_LIST_6(x, ...) STRINGIFY(x), STRINGIFY_LIST_5(__VA_ARGS__)
#define STRINGIFY_LIST_7(x, ...) STRINGIFY(x), STRINGIFY_LIST_6(__VA_ARGS__)
#define STRINGIFY_LIST_8(x, ...) STRINGIFY(x), STRINGIFY_LIST_7(__VA_ARGS__)

#define STRINGIFY_LIST_(N, ...) CONCATENATE(STRINGIFY_LIST_, N)(__VA_ARGS__)
#define STRINGIFY_LIST(...) STRINGIFY_LIST_(PP_NARG(__VA_ARGS__), __VA_ARGS__)

#define PP_NARG(...) PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define PP_RSEQ_N() 8,7,6,5,4,3,2,1,0

#define ASSERT_COMPILE_ERROR(expected, code, ...) \
	const size_t argc = sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *); \
	CU_ASSERT_EQUAL(test_compile_error((const char *[]){__VA_ARGS__}, argc, code), expected);

#define ASSERT_COMPILE_ERROR_NOARGS(expected, code) \
	CU_ASSERT_EQUAL(test_compile_error(NULL, 0, code), expected);

#define ASSERT_EXEC(expr, cexpr, ...) \
{ \
	mathfun_error_p error = NULL; \
	CU_ASSERT(issame(cexpr, mathfun_run(expr, &error, STRINGIFY_LIST(__VA_ARGS__), NULL, __VA_ARGS__))); \
	CU_ASSERT(error == NULL); \
	if (error) mathfun_error_log_and_cleanup(&error, stderr); \
	mathfun fun; \
	const char *argnames[] = {STRINGIFY_LIST(__VA_ARGS__)}; \
	mathfun_compile(&fun, argnames, PP_NARG(__VA_ARGS__), expr, &error); \
	CU_ASSERT(error == NULL); \
	if (error) { \
		mathfun_error_log_and_cleanup(&error, stderr); \
	} \
	else { \
		CU_ASSERT(issame(cexpr, mathfun_call(&fun, &error, __VA_ARGS__))); \
		CU_ASSERT(error == NULL); \
		if (error) mathfun_error_log_and_cleanup(&error, stderr); \
		const double args[] = {__VA_ARGS__}; \
		CU_ASSERT(issame(cexpr, mathfun_acall(&fun, args, &error))); \
		CU_ASSERT(error == NULL); \
		if (error) mathfun_error_log_and_cleanup(&error, stderr); \
		mathfun_cleanup(&fun); \
	}\
}

#define ASSERT_EXEC_DIRECT(expr, ...) ASSERT_EXEC(STRINGIFY(expr), expr, __VA_ARGS__)

static bool issame(double x, double y) {
	return isnan(x) ? isnan(y) : x == y;
}

static bool test_compile_success(const char *argnames[], size_t argc, const char *code) {
	mathfun fun;
	mathfun_error_p error = NULL;
	bool compile_success = mathfun_compile(&fun, argnames, argc, code, &error);
	if (!compile_success) {
		mathfun_error_log_and_cleanup(&error, stderr);
	}
	mathfun_cleanup(&fun);
	return compile_success;
}

static enum mathfun_error_type test_compile_error(const char *argnames[], size_t argc, const char *code) {
	mathfun fun;
	mathfun_error_p error = NULL;
	enum mathfun_error_type error_type = mathfun_compile(&fun, argnames, argc, code, &error) ?
		MATHFUN_OK : mathfun_error_type(error);
	mathfun_error_cleanup(&error);
	mathfun_cleanup(&fun);
	return error_type;
}

static void test_compile() {
	const char *argnames[] = { "x" };
	CU_ASSERT(test_compile_success(argnames, 1, "sin(x)"));
}

static void test_empty_argument_name() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "");
}

static void test_empty_argument_name_with_spaces() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "foo  ");
}

static void test_illegal_argument_name_true() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "true");
}

static void test_illegal_argument_name_FalSE() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "FalSE");
}

static void test_illegal_argument_name_Inf() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "Inf");
}

static void test_illegal_argument_name_nan() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "nan");
}

static void test_illegal_argument_name_number() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "123");
}

static void test_illegal_argument_name_minus() {
	ASSERT_COMPILE_ERROR(MATHFUN_ILLEGAL_NAME, "pi", "-");
}

static void test_duplicate_argument_name() {
	ASSERT_COMPILE_ERROR(MATHFUN_DUPLICATE_ARGUMENT, "bar", "foo", "bar", "foo");
}

static void test_empty_expr() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, "");
}

static void test_parser_expected_close_parenthesis_but_got_eof() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, "(5 + 2");
}

static void test_parser_expected_close_parenthesis_but_got_something_else() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS, "(5 + 2 3");
}

static void test_parser_funct_expected_close_parenthesis_but_got_eof() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, "sin(5 + 2");
}

static void test_parser_funct_expected_close_parenthesis_but_got_something_else() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS, "sin(5 + 2 3");
}

static void test_parser_undefined_reference_funct() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNDEFINED_REFERENCE, "foo()");
}

static void test_parser_undefined_reference_var() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNDEFINED_REFERENCE, "bar");
}

static void test_parser_not_a_function_but_an_argument() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_NOT_A_FUNCTION, "x()", "x");
}

static void test_parser_not_a_function_but_a_const() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_NOT_A_FUNCTION, "pi()");
}

static void test_parser_not_a_variable() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_NOT_A_VARIABLE, "sin");
}

static void test_parser_illegal_number_of_arguments() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS, "sin(pi,e)");
}

static void test_parser_expected_number_but_got_something_else() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_EXPECTED_NUMBER, ".x");
}

static void test_parser_expected_identifier_but_got_something_else() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_EXPECTED_IDENTIFIER, "$");
}

static void test_parser_expected_colon_but_got_eof() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, "true ? pi ");
}

static void test_parser_expected_colon_but_got_something_else() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_PARSER_EXPECTED_COLON, "true ? pi e");
}

static void test_parser_expected_dots_but_got_eof() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, "x in 5", "x");
}

static void test_parser_expected_dots_but_got_something_else() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_EXPECTED_DOTS, "x in 5 5", "x");
}

static void test_parser_type_error_expected_number() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_TYPE_ERROR, "x in 1...5", "x");
}

static void test_parser_type_error_expected_boolean() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_TYPE_ERROR, "x ? pi : e", "x");
}

static void test_parser_trailing_garbage() {
	ASSERT_COMPILE_ERROR(MATHFUN_PARSER_TRAILING_GARBAGE, "x 5", "x");
}

static void test_math_error_in_const_folding() {
	ASSERT_COMPILE_ERROR_NOARGS(MATHFUN_MATH_ERROR, "5 % 0");
}

static void test_mod() {
	errno = 0;
	CU_ASSERT(issame(mathfun_mod(5.0, 0.0), NAN));
	CU_ASSERT_EQUAL(errno, EDOM);

	errno = 0;
	CU_ASSERT(issame(mathfun_mod(9.0, 5.0), 4.0));
	CU_ASSERT_EQUAL(errno, 0);

	errno = 0;
	CU_ASSERT(issame(mathfun_mod(-2.1, 5.2), 3.1));
	CU_ASSERT_EQUAL(errno, 0);

	errno = 0;
	CU_ASSERT(issame(mathfun_mod(-8.0, -5.2), -2.8));
	CU_ASSERT_EQUAL(errno, 0);

	errno = 0;
	CU_ASSERT(issame(mathfun_mod(9.0, -5.0), -1.0));
	CU_ASSERT_EQUAL(errno, 0);
	
	errno = 0;
	CU_ASSERT(issame(mathfun_mod(INFINITY, 1.0), NAN));
	CU_ASSERT_EQUAL(errno, EDOM);
	
	errno = 0;
	CU_ASSERT(issame(mathfun_mod(INFINITY, -1.0), -NAN));
	CU_ASSERT_EQUAL(errno, EDOM);
	
	errno = 0;
	CU_ASSERT(issame(mathfun_mod(-INFINITY, 1.0), -NAN));
	CU_ASSERT_EQUAL(errno, EDOM);
}

static void test_exec_sin_x() {
	const double x = M_PI_2;
	ASSERT_EXEC_DIRECT(sin(x), x);
}

static void test_exec_all() {
	const double x = M_PI_2;
	const double y = 1.0;
	const double z = 2.0;
	ASSERT_EXEC(
		"x in (-3e2 * y)...Inf && y == 1 || !(x <= pi_2 ? z >= 2 || x > NaN || y in -10..10 : z != -2 && z < x) ? x % z / 3 : -x ** y - z + +cos(5.5)",
		((x >= (-3e2 * y) && x < INFINITY) && y == 1) || !(x <= M_PI_2 ?
			z >= 2 || x > NAN || (y >= -10 && y <= 10) : z != -2 && z < x) ?
			mathfun_mod(x, z) / 3 : pow(-x, y) - z + +cos(5.5),
		x, y, z);
}

CU_TestInfo compile_test_infos[] = {
	{"compile", test_compile},
	{"empty argument name", test_empty_argument_name},
	{"argument name with spaces",test_empty_argument_name_with_spaces},
	{"illegal argument name: true", test_illegal_argument_name_true},
	{"illegal argument name: FalSE", test_illegal_argument_name_FalSE},
	{"illegal argument name: Inf", test_illegal_argument_name_Inf},
	{"illegal argument name: nan", test_illegal_argument_name_nan},
	{"illegal argument name: 123", test_illegal_argument_name_number},
	{"illegal argument name: -", test_illegal_argument_name_minus},
	{"duplicate argument name", test_duplicate_argument_name},
	{"empty expression", test_empty_expr},
	{"eof instead of )", test_parser_expected_close_parenthesis_but_got_eof},
	{"missing )", test_parser_expected_close_parenthesis_but_got_something_else},
	{"eof instead of ) in function call", test_parser_funct_expected_close_parenthesis_but_got_eof},
	{"missing ) in function call", test_parser_funct_expected_close_parenthesis_but_got_something_else},
	{"undefined reference (function)", test_parser_undefined_reference_funct},
	{"undefined reference (const/argument)", test_parser_undefined_reference_var},
	{"argument is not a function", test_parser_not_a_function_but_an_argument},
	{"const is not a function", test_parser_not_a_function_but_a_const},
	{"function is not a variable", test_parser_not_a_variable},
	{"illegal number of arguments", test_parser_illegal_number_of_arguments},
	{"illegal number", test_parser_expected_number_but_got_something_else},
	{"not an identifier", test_parser_expected_identifier_but_got_something_else},
	{"eof instead of :", test_parser_expected_colon_but_got_eof},
	{"missing :", test_parser_expected_colon_but_got_something_else},
	{"eof instead of ... (dots)", test_parser_expected_dots_but_got_eof},
	{"missing ... (dots)", test_parser_expected_dots_but_got_something_else},
	{"type error: expected number", test_parser_type_error_expected_number},
	{"type error: expected boolean", test_parser_type_error_expected_boolean},
	{"trailing garbage", test_parser_trailing_garbage},
	{"math error in const folding", test_math_error_in_const_folding},
	{NULL, NULL}
};

CU_TestInfo exec_test_infos[] = {
	{"%", test_mod},
	{"sin(x)", test_exec_sin_x},
	{"expression with all operators", test_exec_all},
	{NULL, NULL}
};

CU_SuiteInfo test_suite_infos[] = {
	{"compile", NULL, NULL, compile_test_infos},
	{"execute", NULL, NULL, exec_test_infos},
	{NULL, NULL, NULL, NULL}
};

int main() {
	if (CUE_SUCCESS != CU_initialize_registry()) {
		return CU_get_error();
	}

	if (CUE_SUCCESS != CU_register_suites(test_suite_infos)) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_ErrorCode error_code = CU_get_error();
	unsigned int failures = CU_get_number_of_failures();
	CU_cleanup_registry();
	return error_code || failures;
}
