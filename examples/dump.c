#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mathfun.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "invalid number of arguments\n");
		return 1;
	}

	const size_t funct_argc = argc - 2;
	struct mathfun_context ctx;
	struct mathfun mathfun;
	mathfun_error_info error = NULL;

	if (!mathfun_context_init(&ctx, true, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		return 1;
	}

	if (!mathfun_context_compile(&ctx, (const char**)argv + 1, funct_argc, argv[argc - 1], &mathfun, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_context_cleanup(&ctx);
		return 1;
	}

	if (!mathfun_dump(&mathfun, stdout, &ctx, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_cleanup(&mathfun);
		mathfun_context_cleanup(&ctx);
		return 1;
	}

	mathfun_cleanup(&mathfun);
	mathfun_context_cleanup(&ctx);

	return 0;
}
