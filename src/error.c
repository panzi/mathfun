#include <string.h>
#include <errno.h>

#include "mathfun_intern.h"

struct mathfun_error {
	enum mathfun_error_type type;
	int         errnum;
	size_t      lineno;
	size_t      column;
	const char *str;
	const char *errpos;
	size_t      errlen;
	size_t      argc;
	size_t      expected_argc;
};

static struct mathfun_error mathfun_error = {
	MATHFUN_OK, 0, 0, 0, NULL, NULL, 0, 0, 0
};

enum mathfun_error_type mathfun_error_type() {
	return mathfun_error.type;
}

int mathfun_error_errno() {
	return mathfun_error.errnum;
}

size_t mathfun_error_lineno() {
	return mathfun_error.lineno;
}

size_t mathfun_error_column() {
	return mathfun_error.column;
}

size_t mathfun_error_errpos() {
	return mathfun_error.errpos - mathfun_error.str;
}

size_t mathfun_error_errlen() {
	return mathfun_error.errlen;
}

void mathfun_error_clear() {
	mathfun_error.type          = MATHFUN_OK;
	mathfun_error.errnum        = 0;
	mathfun_error.lineno        = 0;
	mathfun_error.column        = 0;
	mathfun_error.str           = NULL;
	mathfun_error.errpos        = NULL;
	mathfun_error.errlen        = 0;
	mathfun_error.argc          = 0;
	mathfun_error.expected_argc = 0;
}

void mathfun_raise_error(enum mathfun_error_type type) {
	mathfun_error_clear();

	mathfun_error.type = type;

	switch (type) {
		case MATHFUN_OK:
			break;

		case MATHFUN_IO_ERROR:
		case MATHFUN_MATH_ERROR:
		case MATHFUN_MEMORY_ERROR:
		case MATHFUN_C_ERROR:
			mathfun_error.errnum = errno;
			break;

		default:
			mathfun_error.errnum = errno = EINVAL;
			break;
	}
}

void mathfun_raise_name_error(enum mathfun_error_type type, const char *name) {
	mathfun_raise_error(type);
	mathfun_error.str = name;
}

void mathfun_raise_parser_error(const struct mathfun_parser *parser,
	enum mathfun_error_type type, const char *errpos) {
	mathfun_error_clear();

	mathfun_error.type   = type;
	mathfun_error.errnum = errno = EINVAL;
	mathfun_error.str    = parser->code;
	mathfun_error.errpos = errpos ? errpos : parser->ptr;

	switch (type) {
		case MATHFUN_PARSER_UNDEFINED_REFERENCE:
		case MATHFUN_PARSER_NOT_A_FUNCTION:
		case MATHFUN_PARSER_NOT_A_VARIABLE:
			mathfun_error.errlen = mathfun_find_identifier_end(mathfun_error.errpos) - mathfun_error.errpos;
			break;

		default:
			mathfun_error.errlen = errpos ? parser->ptr - errpos : 1;
	}

	size_t lineno = 1;
	size_t column = 1;
	const char *ptr = parser->code;
	for (; ptr < mathfun_error.errpos; ++ ptr) {
		if (*ptr == '\n') {
			++ lineno;
			column = 1;
		}
		else {
			++ column;
		}
	}

	mathfun_error.lineno = lineno;
	mathfun_error.column = column;
}

void mathfun_raise_c_error() {
	switch (errno) {
		case 0:
			mathfun_error_clear();
			break;

		case ERANGE:
		case EDOM:
			mathfun_raise_math_error(errno);
			break;

		default:
			mathfun_raise_error(MATHFUN_C_ERROR);
			break;
	}
}

void mathfun_raise_math_error(int errnum) {
	errno = errnum;
	mathfun_raise_error(MATHFUN_MATH_ERROR);
}

void mathfun_raise_parser_argc_error(const struct mathfun_parser *parser,
	const char *errpos, size_t expected, size_t got) {
	mathfun_raise_parser_error(parser, MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS, errpos);
	mathfun_error.expected_argc = expected;
	mathfun_error.argc          = got;
}

void mathfun_log_parser_error(FILE *stream, const char *fmt, ...) {
	va_list ap;
	const char *endl = strchr(mathfun_error.errpos, '\n');
	int n = endl ? endl - mathfun_error.str : (int)strlen(mathfun_error.str);

	fprintf(stream, "%"PRIzu":%"PRIzu": parser error: ", mathfun_error.lineno, mathfun_error.column);

	va_start(ap, fmt);
	vfprintf(stream, fmt, ap);
	va_end(ap);

	fprintf(stream, "\n%.*s\n", n, mathfun_error.str);
	if (mathfun_error.column > 0) {
		for (size_t column = mathfun_error.column - 1; column > 0; -- column) {
			fputc('-', stream);
		}
	}
	fprintf(stream, "^\n");
}

void mathfun_error_log(FILE *stream) {
	switch (mathfun_error.type) {
		case MATHFUN_OK:
			fprintf(stream, "no error\n");
			break;

		case MATHFUN_MEMORY_ERROR:
		case MATHFUN_IO_ERROR:
		case MATHFUN_MATH_ERROR:
		case MATHFUN_C_ERROR:
			fprintf(stream, "error: %s\n", strerror(mathfun_error.errnum));
			break;

		case MATHFUN_ILLEGAL_NAME:
			fprintf(stream, "error: illegal name: '%s'\n", mathfun_error.str);
			break;

		case MATHFUN_DUPLICATE_ARGUMENT:
			fprintf(stream, "error: duplicate argument: '%s'\n", mathfun_error.str);
			break;

		case MATHFUN_NAME_EXISTS:
			fprintf(stream, "error: name already exists: '%s'\n", mathfun_error.str);
			break;

		case MATHFUN_NO_SUCH_NAME:
			fprintf(stream, "error: no such constant or function: '%s'\n", mathfun_error.str);
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
			mathfun_log_parser_error(stream, "expected ')'");
			break;

		case MATHFUN_PARSER_UNDEFINED_REFERENCE:
			mathfun_log_parser_error(stream, "undefined reference: '%.*s'",
				mathfun_error.errlen, mathfun_error.errpos);
			break;

		case MATHFUN_PARSER_NOT_A_FUNCTION:
			mathfun_log_parser_error(stream, "reference is not a function: '%.*s'",
				mathfun_error.errlen, mathfun_error.errpos);
			break;

		case MATHFUN_PARSER_NOT_A_VARIABLE:
			mathfun_log_parser_error(stream, "reference is not an argument or constant: '%.*s'",
				mathfun_error.errlen, mathfun_error.errpos);
			break;

		case MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS:
			mathfun_log_parser_error(stream, "illegal number of arguments: expected %"PRIzu" but got %"PRIzu,
				mathfun_error.expected_argc, mathfun_error.argc);
			break;

		case MATHFUN_PARSER_EXPECTED_NUMBER:
			mathfun_log_parser_error(stream, "expected a number");
			break;

		case MATHFUN_PARSER_EXPECTED_IDENTIFIER:
			mathfun_log_parser_error(stream, "expected an identifier");
			break;

		case MATHFUN_PARSER_TRAILING_GARBAGE:
			mathfun_log_parser_error(stream, "trailing garbage");
			break;

		default:
			fprintf(stream, "error: unknown error: %d\n", mathfun_error.type);
			break;
	}
}
