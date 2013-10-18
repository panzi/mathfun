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
	int errnum = mathfun_context_init(&ctx, true);
	if (errnum != 0) {
		perror("error creating mathfun context");
		return 1;
	}

	errnum = mathfun_context_compile(&ctx, (const char**)argv + 1, funct_argc, argv[argc - 1], &mathfun);
	if (errnum != 0) {
		mathfun_context_cleanup(&ctx);
		perror("error compiling expression");
		return 1;
	}

	mathfun_dump(&mathfun, stdout, &ctx);
	mathfun_cleanup(&mathfun);
	mathfun_context_cleanup(&ctx);

	return 0;
}
