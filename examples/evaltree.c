#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mathfun.h>
#include <math.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "invalid number of arguments\n");
		return 1;
	}

	char **argnames = argv + 1;
	const size_t funct_argc = argc - 2;
	double *funct_args = calloc(funct_argc, sizeof(double));

	if (!funct_args) {
		perror("error allocating arguments");
		return 1;
	}

	for (size_t i = 0; i < funct_argc; ++ i) {
		char *argname = argnames[i];
		char *assign  = strchr(argname, '=');

		if (!assign) {
			fprintf(stderr, "invalid argument: %s\n", argname);
			free(funct_args);
			return 1;
		}

		char *arg = assign + 1;
		char *endptr = NULL;
		funct_args[i] = strtod(arg, &endptr);

		if (arg == endptr) {
			fprintf(stderr, "invalid argument: %s\n", argname);
			free(funct_args);
			return 1;
		}
		
		*assign = 0;
	}

	mathfun_error_p error = NULL;
	double value = mathfun_arun((const char **)argnames, funct_argc, argv[argc - 1], funct_args, &error);
	free(funct_args);
	
	if (error) {
		mathfun_error_log_and_cleanup(&error, stderr);
		return 1;
	}

	printf("%.22g\n", value);

	return 0;
}
