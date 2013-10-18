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
	struct mathfun mathfun;
	int errnum = mathfun_compile(&mathfun, (const char**)argv + 1, funct_argc, argv[argc - 1]);
	if (errnum != 0) {
		fprintf(stderr, "error compiling expression: %s\n", strerror(errnum));
		return 1;
	}

	mathfun_dump(&mathfun, stdout);
	mathfun_cleanup(&mathfun);

	return 0;
}
