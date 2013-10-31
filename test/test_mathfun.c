#include <CUnit/Basic.h>
#include <CUnit/TestRun.h>
#include <mathfun.h>
#include <stdlib.h>

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

void test_compile() {
	CU_ASSERT(test_compile_success((const char*[]){ "x" }, 1, "sin(x)"));
}

int main() {
	CU_pSuite pSuite = NULL;
	
	if (CUE_SUCCESS != CU_initialize_registry()) {
		return CU_get_error();
	}

	pSuite = CU_add_suite("mathfun", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (NULL == CU_add_test(pSuite, "test compile", test_compile)) {
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
