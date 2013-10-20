#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "mathfun_intern.h"

// BNF
// expression   ::= a_expr
// a_expr       ::= m_expr (("+"|"-") m_expr)*
// m_expr       ::= u_expr (("*"|"/"|"%") u_expr)*
// u_expr       ::= power | ("-"|"+") u_expr
// power        ::= atom ["**" u_expr]
// atom         ::= identifier ("(" [expression ("," expression)*] ")")? | number | "(" expression ")"
// identifier   ::= (alpha|"_")(alnum|"_")*
// number       ::= "Inf" | "NaN" | ["-"]("0"|"1"..."9"digit*)["."digit*][("e"|"E")["+"|"-"]digit+]
//
// or just use strtod for number

struct mathfun_expr *mathfun_parse_a_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_m_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_u_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_power(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_atom(struct mathfun_parser *parser);
const char          *mathfun_parse_identifier(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_number(struct mathfun_parser *parser);

static bool strbuf_init(struct strbuf *buf) {
	buf->data = NULL;
	buf->size = 0;
	buf->used = 0;

	size_t size = 256;
	buf->data = malloc(size);

	if (!buf->data) {
		mathfun_raise_error(MATHFUN_MEMORY_ERROR);
		return false;
	}

	buf->size = size;
	return true;
}

static bool strbuf_ensure(struct strbuf *buf, size_t n) {
	if (buf->used + n > buf->size) {
		size_t size = buf->used + n;
		size_t rem  = size % 256;
		size += rem == 0 ? 256 : rem;
		char *data = realloc(buf->data, size);

		if (!data) {
			mathfun_raise_error(MATHFUN_MEMORY_ERROR);
			return false;
		}

		buf->data = data;
		buf->size = size;
	}
	return true;
}

static bool strbuf_append(struct strbuf *buf, const char *str, size_t n) {
	if (!strbuf_ensure(buf, n)) return false;
	memcpy(buf->data + buf->used, str, n);
	buf->used += n;
	return true;
}

static bool strbuf_append_nil(struct strbuf *buf) {
	if (!strbuf_ensure(buf, 1)) return false;
	buf->data[buf->used] = 0;
	++ buf->used;
	return true;
}

static void strbuf_clear(struct strbuf *buf) {
	buf->used = 0;
}

static void strbuf_cleanup(struct strbuf *buf) {
	free(buf->data);
	buf->data = NULL;
	buf->size = 0;
	buf->used = 0;
}

static void skipws(struct mathfun_parser *parser) {
	const char *ptr = parser->ptr;
	while (isspace(*ptr)) ++ ptr;
	parser->ptr = ptr;
}

struct mathfun_expr *mathfun_parse_a_expr(struct mathfun_parser *parser) {
	struct mathfun_expr *expr = mathfun_parse_m_expr(parser);

	if (!expr) return NULL;

	for (;;) {
		skipws(parser);
		const char ch = *parser->ptr;
		if (ch == '+' || ch == '-') {
			++ parser->ptr;
			skipws(parser);
			struct mathfun_expr *left  = expr;
			struct mathfun_expr *right = mathfun_parse_m_expr(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			expr = mathfun_expr_alloc(ch == '+' ? EX_ADD : EX_SUB);
			if (!expr) {
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr->ex.binary.left  = left;
			expr->ex.binary.right = right;
		}
		else {
			break;
		}
	}

	return expr;
}

struct mathfun_expr *mathfun_parse_m_expr(struct mathfun_parser *parser) {
	struct mathfun_expr *expr = mathfun_parse_u_expr(parser);

	if (!expr) return NULL;

	for (;;) {
		skipws(parser);
		const char ch = *parser->ptr;
		if (ch == '*' || ch == '/' || ch == '%') {
			++ parser->ptr;
			skipws(parser);
			struct mathfun_expr *left  = expr;
			struct mathfun_expr *right = mathfun_parse_u_expr(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			expr = mathfun_expr_alloc(ch == '*' ? EX_MUL : ch == '/' ? EX_DIV : EX_MOD);
			if (!expr) {
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr->ex.binary.left  = left;
			expr->ex.binary.right = right;
		}
		else {
			break;
		}
	}

	return expr;
}

struct mathfun_expr *mathfun_parse_u_expr(struct mathfun_parser *parser) {
	const char ch = *parser->ptr;
	if (ch == '+' || ch == '-') {
		++ parser->ptr;
		skipws(parser);
		struct mathfun_expr *expr = mathfun_parse_u_expr(parser);
		if (!expr) return NULL;

		if (ch == '-') {
			struct mathfun_expr *child = expr;
			expr = mathfun_expr_alloc(EX_NEG);

			if (!expr) {
				mathfun_expr_free(child);
				return NULL;
			}

			expr->ex.unary.expr = child;
		}
		return expr;
	}
	else {
		return mathfun_parse_power(parser);
	}
}

struct mathfun_expr *mathfun_parse_power(struct mathfun_parser *parser) {
	struct mathfun_expr *expr = mathfun_parse_atom(parser);

	if (!expr) return NULL;

	skipws(parser);

	if (parser->ptr[0] == '*' && parser->ptr[1] == '*') {
		parser->ptr += 2;
		skipws(parser);

		struct mathfun_expr *left  = expr;
		struct mathfun_expr *right = mathfun_parse_u_expr(parser);
		
		if (!right) {
			mathfun_expr_free(left);
			return NULL;
		}

		expr = mathfun_expr_alloc(EX_POW);
		if (!expr) {
			mathfun_expr_free(left);
			mathfun_expr_free(right);
			return NULL;
		}

		expr->ex.binary.left  = left;
		expr->ex.binary.right = right;
	}

	return expr;
}

struct mathfun_expr *mathfun_parse_atom(struct mathfun_parser *parser) {
	char ch = *parser->ptr;

	if (ch == '(') {
		++ parser->ptr;
		skipws(parser);
		struct mathfun_expr *expr = mathfun_parse_a_expr(parser);
		if (!expr) return NULL;
		if (*parser->ptr != ')') {
			// missing ')'
			mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS, NULL);
			mathfun_expr_free(expr);
			return NULL;
		}
		++ parser->ptr;
		skipws(parser);
		return expr;
	}
	else if (isdigit(ch) || ch == '.') {
		return mathfun_parse_number(parser);
	}
	else {
		const char *idstart = parser->ptr;
		const char *identifier = mathfun_parse_identifier(parser);
		if (!identifier) return NULL;

		if (strcasecmp(identifier, "nan") == 0) {
			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
			if (!expr) return NULL;
			expr->ex.value = NAN;
			return expr;
		}
		else if (strcasecmp(identifier, "inf") == 0) {
			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
			if (!expr) return NULL;
			expr->ex.value = INFINITY;
			return expr;
		}

		size_t argind = 0;
		for (; argind < parser->argc; ++ argind) {
			if (strcmp(parser->argnames[argind], identifier) == 0) {
				break;
			}
		}

		if (*parser->ptr != '(') {
			if (argind < parser->argc) {
				struct mathfun_expr *expr = mathfun_expr_alloc(EX_ARG);
				if (!expr) return NULL;
				expr->ex.arg = argind;
				return expr;
			}

			const struct mathfun_decl *decl = mathfun_context_get(parser->ctx, identifier);

			if (!decl) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_UNDEFINED_REFERENCE, idstart);
				return NULL;
			}

			if (decl->type != DECL_CONST) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_VARIABLE, idstart);
				return NULL;
			}

			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
			if (!expr) return NULL;

			expr->ex.value = decl->decl.value;
			return expr;
		}
		else if (argind < parser->argc) {
			mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_FUNCTION, idstart);
			return NULL;
		}
		else {
			const struct mathfun_decl *decl = mathfun_context_get(parser->ctx, identifier);

			if (!decl) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_UNDEFINED_REFERENCE, idstart);
				return NULL;
			}

			if (decl->type != DECL_FUNCT) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_FUNCTION, idstart);
				return NULL;
			}

			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CALL);
			size_t size = 0;

			if (!expr) return NULL;

			++ parser->ptr;
			skipws(parser);

			const char *lastarg = parser->ptr;
			for (;;) {
				ch = *parser->ptr;
				if (!ch || ch == ')') break;
				lastarg = parser->ptr;
				struct mathfun_expr *arg = mathfun_parse_a_expr(parser);

				if (!arg) {
					mathfun_expr_free(expr);
					return NULL;
				}

				if (size == expr->ex.funct.argc) {
					size += 16;
					struct mathfun_expr **args = realloc(expr->ex.funct.args, size * sizeof(struct mathfun_expr*));

					if (!args) {
						mathfun_expr_free(expr);
						mathfun_expr_free(arg);
						return NULL;
					}

					expr->ex.funct.args = args;
				}

				expr->ex.funct.args[expr->ex.funct.argc] = arg;
				++ expr->ex.funct.argc;

				if (*parser->ptr != ',') break;

				++ parser->ptr;
				skipws(parser);
			}

			if (*parser->ptr != ')') {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS, NULL);
				mathfun_expr_free(expr);
				return NULL;
			}
			++ parser->ptr;
			skipws(parser);

			if (expr->ex.funct.argc != decl->decl.funct.argc) {
				mathfun_raise_parser_argc_error(parser, lastarg, decl->decl.funct.argc, expr->ex.funct.argc);
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.funct.funct = decl->decl.funct.funct;

			return expr;
		}
	}
}

struct mathfun_expr *mathfun_parse_number(struct mathfun_parser *parser) {
	char *endptr = NULL;
	mathfun_value value = strtod(parser->ptr, &endptr);

	if (parser->ptr == endptr) {
		mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_NUMBER, NULL);
		return NULL;
	}
	parser->ptr = endptr;

	skipws(parser);

	struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
	if (!expr) return NULL;

	expr->ex.value = value;
	return expr;
}

const char *mathfun_find_identifier_end(const char *str) {
	if (!isalpha(*str) && *str != '_') {
		return str;
	}
	++ str;

	while (isalnum(*str) || *str == '_') {
		++ str;
	}

	return str;
}

const char *mathfun_parse_identifier(struct mathfun_parser *parser) {
	const char *from = parser->ptr;
	parser->ptr = mathfun_find_identifier_end(from);

	if (from == parser->ptr) {
		mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_IDENTIFIER, from);
		return NULL;
	}

	strbuf_clear(&parser->buf);

	if (!strbuf_append(&parser->buf, from, parser->ptr - from) ||
		!strbuf_append_nil(&parser->buf)) {
		return NULL;
	}

	skipws(parser);

	return parser->buf.data;
}

struct mathfun_expr *mathfun_context_parse(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code) {
	for (size_t i = 0; i < argc; ++ i) {
		if (!mathfun_valid_name(argnames[i])) {
			mathfun_raise_name_error(MATHFUN_ILLEGAL_NAME, argnames[i]);
			return NULL;
		}
	}

	struct mathfun_parser parser = { ctx, argnames, argc, code, code, {NULL, 0, 0} };
	if (!strbuf_init(&parser.buf)) return NULL;

	skipws(&parser);
	struct mathfun_expr *expr = mathfun_parse_a_expr(&parser);

	strbuf_cleanup(&parser.buf);

	if (expr) {
		skipws(&parser);
		if (*parser.ptr) {
			mathfun_raise_parser_error(&parser, MATHFUN_PARSER_TRAILING_GARBAGE, NULL);
			mathfun_expr_free(expr);
			return NULL;
		}
	}

	return expr;
}
