#include <string.h>
#include <errno.h>

#include "mathfun_intern.h"

// special error info in case of ENOMEM, because then it's unlikely that allocating a new
// error object will work and this is also the only way to signal a proper error in case
// there is an ENOMEM when allocating an error object.
static const struct mathfun_error mathfun_memory_error = {
	MATHFUN_MEMORY_ERROR, ENOMEM, 0, 0, NULL, NULL, 0, 0, 0
};

enum mathfun_error_type mathfun_error_type(mathfun_error_info error) {
	if (error) return error->type;

	switch (errno) {
		case 0:
			return MATHFUN_OK;

		case EDOM:
		case ERANGE:
			return MATHFUN_MATH_ERROR;

		case ENOMEM:
			return MATHFUN_MEMORY_ERROR;

		default:
			return MATHFUN_C_ERROR;
	}
}

int mathfun_error_errno(mathfun_error_info error) {
	return error ? error->errnum : errno;
}

size_t mathfun_error_lineno(mathfun_error_info error) {
	return error ? error->lineno : 0;
}

size_t mathfun_error_column(mathfun_error_info error) {
	return error ? error->column : 0;
}

size_t mathfun_error_errpos(mathfun_error_info error) {
	return error ? error->errpos - error->str : 0;
}

size_t mathfun_error_errlen(mathfun_error_info error) {
	return error ? error->errlen : 0;
}

void mathfun_error_log_and_cleanup(mathfun_error_info *errptr, FILE *stream) {
	mathfun_error_log(errptr ? *errptr : NULL, stream);
	mathfun_error_cleanup(errptr);
}

void mathfun_error_cleanup(mathfun_error_info *errptr) {
	if (errptr) {
		if (*errptr != &mathfun_memory_error) {
			free((struct mathfun_error*)*errptr);
		}
		*errptr = NULL;
	}
}

struct mathfun_error *mathfun_error_alloc(enum mathfun_error_type type) {
	struct mathfun_error *error = calloc(1, sizeof(struct mathfun_error));

	if (error) {
		error->type = type;

		switch (type) {
			case MATHFUN_OK: // probably I should assert(false) here
				break;

			case MATHFUN_IO_ERROR:
			case MATHFUN_MATH_ERROR:
			case MATHFUN_MEMORY_ERROR:
			case MATHFUN_C_ERROR:
				error->errnum = errno;
				break;

			default:
				error->errnum = errno = EINVAL;
				break;
		}
	}

	return error;
}

void mathfun_raise_error(mathfun_error_info *errptr, enum mathfun_error_type type) {
	if (!errptr) return;
	if (type == MATHFUN_MEMORY_ERROR || (type == MATHFUN_C_ERROR && errno == ENOMEM)) {
		*errptr = &mathfun_memory_error;
	}
	else {
		*errptr = mathfun_error_alloc(type);
	}
}

void mathfun_raise_name_error(mathfun_error_info *errptr, enum mathfun_error_type type,
	const char *name) {
	if (errptr) {
		struct mathfun_error *error = mathfun_error_alloc(type);

		if (error) {
			error->str = name;
			*errptr = error;
		}
		else {
			*errptr = &mathfun_memory_error;
		}
	}
}

static struct mathfun_error *mathfun_alloc_parse_error(
	const struct mathfun_parser *parser, enum mathfun_error_type type, const char *errpos) {
	struct mathfun_error *error = mathfun_error_alloc(type);
	
	if (error) {
		error->type   = type;
		error->errnum = errno = EINVAL;
		error->str    = parser->code;
		error->errpos = errpos ? errpos : parser->ptr;

		switch (type) {
			case MATHFUN_PARSER_UNDEFINED_REFERENCE:
			case MATHFUN_PARSER_NOT_A_FUNCTION:
			case MATHFUN_PARSER_NOT_A_VARIABLE:
				error->errlen = mathfun_find_identifier_end(error->errpos) - error->errpos;
				break;

			default:
				error->errlen = errpos ? parser->ptr - errpos : 1;
		}

		size_t lineno = 1;
		size_t column = 1;
		const char *ptr = parser->code;
		for (; ptr < error->errpos; ++ ptr) {
			if (*ptr == '\n') {
				++ lineno;
				column = 1;
			}
			else {
				++ column;
			}
		}

		error->lineno = lineno;
		error->column = column;
	}

	return error;
}

void mathfun_raise_parser_error(const struct mathfun_parser *parser,
	enum mathfun_error_type type, const char *errpos) {
	if (parser->error) {
		struct mathfun_error *error = mathfun_alloc_parse_error(parser, type, errpos);

		if (error) {
			*parser->error = error;
		}
		else {
			*parser->error = &mathfun_memory_error;
		}
	}
}

void mathfun_raise_parser_argc_error(const struct mathfun_parser *parser,
	const char *errpos, size_t expected, size_t got) {
	if (parser->error) {
		struct mathfun_error *error = mathfun_alloc_parse_error(parser, MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS, errpos);

		if (error) {
			error->expected_argc = expected;
			error->argc          = got;

			*parser->error = error;
		}
		else {
			*parser->error = &mathfun_memory_error;
		}
	}
}

void mathfun_raise_c_error(mathfun_error_info *errptr) {
	if (errptr) {
		switch (errno) {
			case 0:
				*errptr = NULL;
				break;

			case ERANGE:
			case EDOM:
				mathfun_raise_math_error(errptr, errno);
				break;

			default:
				mathfun_raise_error(errptr, MATHFUN_C_ERROR);
				break;
		}
	}
}

void mathfun_raise_math_error(mathfun_error_info *errptr, int errnum) {
	errno = errnum;
	mathfun_raise_error(errptr, MATHFUN_MATH_ERROR);
}

static void mathfun_log_parser_error(mathfun_error_info error, FILE *stream, const char *fmt, ...) {
	va_list ap;
	const char *endl = strchr(error->errpos, '\n');
	int n = endl ? endl - error->str : (int)strlen(error->str);

	fprintf(stream, "%"PRIzu":%"PRIzu": parser error: ", error->lineno, error->column);

	va_start(ap, fmt);
	vfprintf(stream, fmt, ap);
	va_end(ap);

	fprintf(stream, "\n%.*s\n", n, error->str);
	if (error->column > 0) {
		for (size_t column = error->column - 1; column > 0; -- column) {
			fputc('-', stream);
		}
	}
	fprintf(stream, "^\n");
}

void mathfun_error_log(mathfun_error_info error, FILE *stream) {
	enum mathfun_error_type type = MATHFUN_OK;
	int errnum = 0;

	if (error) {
		type   = error->type;
		errnum = error->errnum;
	}
	else {
		errnum = errno;

		if (errnum != 0) {
			type = MATHFUN_C_ERROR;
		}
	}

	switch (type) {
		case MATHFUN_OK:
			fprintf(stream, "no error\n");
			break;

		case MATHFUN_MEMORY_ERROR:
		case MATHFUN_IO_ERROR:
		case MATHFUN_MATH_ERROR:
		case MATHFUN_C_ERROR:
			fprintf(stream, "error: %s\n", strerror(errnum));
			break;

		case MATHFUN_ILLEGAL_NAME:
			fprintf(stream, "error: illegal name: '%s'\n", error->str);
			break;

		case MATHFUN_DUPLICATE_ARGUMENT:
			fprintf(stream, "error: duplicate argument: '%s'\n", error->str);
			break;

		case MATHFUN_NAME_EXISTS:
			fprintf(stream, "error: name already exists: '%s'\n", error->str);
			break;

		case MATHFUN_NO_SUCH_NAME:
			fprintf(stream, "error: no such constant or function: '%s'\n", error->str);
			break;

		case MATHFUN_TOO_MANY_ARGUMENTS:
			fprintf(stream, "error: too many arguments\n");
			break;

		case MATHFUN_EXCEEDS_MAX_FRAME_SIZE:
			fprintf(stream, "error: expression would exceed maximum frame size\n");
			break;

		case MATHFUN_INTERNAL_ERROR:
			fprintf(stream, "error: internal error\n");
			break;

		case MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS:
			mathfun_log_parser_error(error, stream, "expected ')'");
			break;

		case MATHFUN_PARSER_UNDEFINED_REFERENCE:
			mathfun_log_parser_error(error, stream, "undefined reference: '%.*s'",
				error->errlen, error->errpos);
			break;

		case MATHFUN_PARSER_NOT_A_FUNCTION:
			mathfun_log_parser_error(error, stream, "reference is not a function: '%.*s'",
				error->errlen, error->errpos);
			break;

		case MATHFUN_PARSER_NOT_A_VARIABLE:
			mathfun_log_parser_error(error, stream, "reference is not an argument or constant: '%.*s'",
				error->errlen, error->errpos);
			break;

		case MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS:
			mathfun_log_parser_error(error, stream, "illegal number of arguments: expected %"PRIzu" but got %"PRIzu,
				error->expected_argc, error->argc);
			break;

		case MATHFUN_PARSER_EXPECTED_NUMBER:
			mathfun_log_parser_error(error, stream, "expected a number");
			break;

		case MATHFUN_PARSER_EXPECTED_IDENTIFIER:
			mathfun_log_parser_error(error, stream, "expected an identifier");
			break;

		case MATHFUN_PARSER_TRAILING_GARBAGE:
			mathfun_log_parser_error(error, stream, "trailing garbage");
			break;

		default:
			fprintf(stream, "error: unknown error: %d\n", type);
			break;
	}
}
