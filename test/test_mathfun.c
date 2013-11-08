#include <CUnit/Basic.h>
#include <CUnit/TestRun.h>
#include <mathfun.h>
#include <stdlib.h>

#define ASSERT_COMPILE_ERROR(expected, code, ...) \
	const size_t argc = sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *); \
	CU_ASSERT_EQUAL(test_compile_error((const char *[]){__VA_ARGS__}, argc, code), expected);

#define ASSERT_COMPILE_ERROR_NOARGS(expected, code) \
	CU_ASSERT_EQUAL(test_compile_error(NULL, 0, code), expected);

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
	const double res = sin(x);

	mathfun_error_p error = NULL;
	CU_ASSERT(issame(res, MATHFUN_RUN("sin(x)", &error, "x", x)));
	CU_ASSERT(error == NULL);
	if (error) mathfun_error_log_and_cleanup(&error, stderr);
	mathfun fun;
	const char *argnames[] = {"x"};
	mathfun_compile(&fun, argnames, 1, "sin(x)", &error);
	CU_ASSERT(error == NULL);
	if (error) {
		mathfun_error_log_and_cleanup(&error, stderr);
	}
	else {
		CU_ASSERT(issame(res, mathfun_call(&fun, &error, x)));
		CU_ASSERT(error == NULL);
		if (error) mathfun_error_log_and_cleanup(&error, stderr);
		const double args[] = {x};
		CU_ASSERT(issame(res, mathfun_acall(&fun, args, &error)));
		CU_ASSERT(error == NULL);
		if (error) mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_cleanup(&fun);
	}
}

static void test_exec_all() {
	const double x = M_PI_2;
	const double y = 1.0;
	const double z = 2.0;

	const char *code =
		"x in (-3e2 * y)...Inf && y == 1 || !(x <= pi_2 ? z >= 2 || x > NaN || "
		"y in -10..10 : z != -2 && z < x) ? x % z / 3 : -x ** y - z + +cos(5.5)";

	const double res = ((x >= (-3e2 * y) && x < INFINITY) && y == 1) || !(x <= M_PI_2 ?
		z >= 2 || x > NAN || (y >= -10 && y <= 10) : z != -2 && z < x) ?
		mathfun_mod(x, z) / 3 : pow(-x, y) - z + +cos(5.5);

	mathfun_error_p error = NULL;
	CU_ASSERT(issame(res, MATHFUN_RUN(code, &error, "x", x, "y", y, "z", z)));
	CU_ASSERT(error == NULL);
	if (error) mathfun_error_log_and_cleanup(&error, stderr);
	mathfun fun;
	const char *argnames[] = {"x", "y", "z"};
	mathfun_compile(&fun, argnames, 3, code, &error);
	CU_ASSERT(error == NULL);
	if (error) {
		mathfun_error_log_and_cleanup(&error, stderr);
	}
	else {
		CU_ASSERT(issame(res, mathfun_call(&fun, &error, x, y, z)));
		CU_ASSERT(error == NULL);
		if (error) mathfun_error_log_and_cleanup(&error, stderr);
		const double args[] = {x, y, z};
		CU_ASSERT(issame(res, mathfun_acall(&fun, args, &error)));
		CU_ASSERT(error == NULL);
		if (error) mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_cleanup(&fun);
	}
}

static mathfun_value test_funct1(const mathfun_value args[]) {
	return (mathfun_value){ .number = args[0].number + args[1].number };
}

static double test_native_funct1(double a, double b) {
	return a + b;
}

static mathfun_value test_funct2(const mathfun_value args[]) {
	return (mathfun_value){ .number = args[0].number - args[1].number };
}

static mathfun_value test_funct3(const mathfun_value args[]) {
	return (mathfun_value){ .number = args[0].number * args[1].number };
}

#define TEST_CONTEXT \
	mathfun_context ctx; \
	mathfun_error_p error = NULL; \
	CU_ASSERT(mathfun_context_init(&ctx, false, &error)); \
	if (error) { \
		mathfun_error_log_and_cleanup(&error, stderr); \
		return; \
	}

#define TEST_CONTEXT_DEFAULTS \
	mathfun_context ctx; \
	mathfun_error_p error = NULL; \
	CU_ASSERT(mathfun_context_init(&ctx, true, &error)); \
	if (error) { \
		mathfun_error_log_and_cleanup(&error, stderr); \
		return; \
	}

static void test_define_funct() {
	TEST_CONTEXT;

	const mathfun_sig sig = {2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};

	CU_ASSERT(mathfun_context_define_funct(&ctx, "funct1", test_funct1, &sig, &error));

	if (error) mathfun_error_log_and_cleanup(&error, stderr);
	mathfun_context_cleanup(&ctx);
}

static void test_define_const() {
	TEST_CONTEXT;

	CU_ASSERT(mathfun_context_define_const(&ctx, "const", 1.0, &error));

	if (error) mathfun_error_log_and_cleanup(&error, stderr);
	mathfun_context_cleanup(&ctx);
}

static void test_define_multiple() {
	TEST_CONTEXT;

	const mathfun_sig sig = {2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};
	const mathfun_decl decls1[] = {
		{ MATHFUN_DECL_CONST, "b", { .value = 2.0 } },
		{ MATHFUN_DECL_CONST, "a", { .value = 1.0 } },

		{ MATHFUN_DECL_FUNCT, "funct1", { .funct = { test_funct1, &sig, (mathfun_native_funct)test_native_funct1 } } },
		{ MATHFUN_DECL_FUNCT, "funct2", { .funct = { test_funct2, &sig, NULL } } },

		{ -1, NULL, { .value = 0 } }
	};
	const mathfun_decl decls2[] = {
		{ MATHFUN_DECL_CONST, "c", { .value = 3.0 } },

		{ MATHFUN_DECL_FUNCT, "funct3", { .funct = { test_funct3, &sig, NULL } } },

		{ -1, NULL, { .value = 0 } }
	};

	CU_ASSERT(mathfun_context_define(&ctx, decls1, &error));
	if (error) mathfun_error_log_and_cleanup(&error, stderr);

	CU_ASSERT(mathfun_context_define(&ctx, decls2, &error));
	if (error) mathfun_error_log_and_cleanup(&error, stderr);

	mathfun_context_cleanup(&ctx);
}

static void test_define_defaults() {
	TEST_CONTEXT_DEFAULTS;
	mathfun_context_cleanup(&ctx);
}

static void test_get_funct() {
	TEST_CONTEXT_DEFAULTS;

	const mathfun_decl *decl = mathfun_context_get(&ctx, "sin");
	CU_ASSERT(decl != NULL && decl->type == MATHFUN_DECL_FUNCT);

	mathfun_context_cleanup(&ctx);
}

static void test_get_const() {
	TEST_CONTEXT_DEFAULTS;

	const mathfun_decl *decl = mathfun_context_get(&ctx, "e");
	CU_ASSERT(decl != NULL && decl->type == MATHFUN_DECL_CONST);

	mathfun_context_cleanup(&ctx);
}

static void test_get_funct_name() {
	TEST_CONTEXT;

	const mathfun_sig sig = {2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};

	CU_ASSERT(mathfun_context_define_funct(&ctx, "funct1", test_funct1, &sig, &error));
	if (error) mathfun_error_log_and_cleanup(&error, stderr);

	const char *name = mathfun_context_funct_name(&ctx, test_funct1);
	CU_ASSERT(name != NULL && strcmp(name, "funct1") == 0);

	mathfun_context_cleanup(&ctx);
}

static void test_undefine() {
	TEST_CONTEXT_DEFAULTS;

	CU_ASSERT(mathfun_context_undefine(&ctx, "sin", &error));
	if (error) mathfun_error_log_and_cleanup(&error, stderr);

	CU_ASSERT(mathfun_context_get(&ctx, "sin") == NULL);

	mathfun_context_cleanup(&ctx);
}

static void test_define_existing() {
	TEST_CONTEXT_DEFAULTS;

	const mathfun_sig sig = {2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};

	CU_ASSERT(!mathfun_context_define_funct(&ctx, "sin", test_funct1, &sig, &error));
	CU_ASSERT_EQUAL(mathfun_error_type(error), MATHFUN_NAME_EXISTS);
	mathfun_error_cleanup(&error);

	CU_ASSERT(!mathfun_context_define_const(&ctx, "e", M_E, &error));
	CU_ASSERT_EQUAL(mathfun_error_type(error), MATHFUN_NAME_EXISTS);
	mathfun_error_cleanup(&error);

	mathfun_context_cleanup(&ctx);
}

static void test_undefine_none_existing() {
	TEST_CONTEXT_DEFAULTS;

	CU_ASSERT(!mathfun_context_undefine(&ctx, "blargh", &error));
	CU_ASSERT_EQUAL(mathfun_error_type(error), MATHFUN_NO_SUCH_NAME);
	mathfun_error_cleanup(&error);

	mathfun_context_cleanup(&ctx);
}

static void test_get_none_existing() {
	TEST_CONTEXT_DEFAULTS;

	CU_ASSERT(mathfun_context_get(&ctx, "blargh") == NULL);

	mathfun_context_cleanup(&ctx);
}

static void test_get_funct_name_none_existing() {
	TEST_CONTEXT_DEFAULTS;

	CU_ASSERT(mathfun_context_funct_name(&ctx, test_funct1) == NULL);

	mathfun_context_cleanup(&ctx);
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
	{"mathfun_mod", test_mod},
	{"sin(x)", test_exec_sin_x},
	{"expression with all operators", test_exec_all},
	{NULL, NULL}
};

CU_TestInfo context_test_infos[] = {
	{"define a function", test_define_funct},
	{"define a constant", test_define_const},
	{"define multiple references", test_define_multiple},
	{"define defaults", test_define_defaults},
	{"get declaration a function", test_get_funct},
	{"get declaration a constant", test_get_const},
	{"get name of a function", test_get_funct_name},
	{"undefine a reference", test_undefine},

	{"define same reference twice", test_define_existing},
	{"undefine not existing reference", test_undefine_none_existing},
	{"get declaration of not existing reference", test_get_none_existing},
	{"get name of not existing function", test_get_funct_name_none_existing},

	{NULL, NULL}
};

CU_SuiteInfo test_suite_infos[] = {
	{"context", NULL, NULL, context_test_infos},
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
