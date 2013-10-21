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
	mathfun_context ctx;
	mathfun fun;
	mathfun_error_p error = NULL;

	if (!mathfun_context_init(&ctx, true, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		return 1;
	}

	if (!mathfun_context_compile(&ctx, (const char**)argv + 1, funct_argc, argv[argc - 1], &fun, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_context_cleanup(&ctx);
		return 1;
	}

	if (!mathfun_dump(&fun, stdout, &ctx, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		mathfun_cleanup(&fun);
		mathfun_context_cleanup(&ctx);
		return 1;
	}

	mathfun_cleanup(&fun);
	mathfun_context_cleanup(&ctx);

	return 0;
}
