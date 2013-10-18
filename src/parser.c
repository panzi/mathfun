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

struct strbuf {
	char  *data;
	size_t size;
	size_t used;
};

struct mathfun_parser {
	const struct mathfun_context *ctx;
	const char **argnames;
	size_t argc;
	const char *code;
	const char *ptr;
	struct strbuf buf;
};

struct mathfun_expr *mathfun_parse_a_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_m_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_u_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_power(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_atom(struct mathfun_parser *parser);
const char          *mathfun_parse_identifier(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_number(struct mathfun_parser *parser);

static int strbuf_init(struct strbuf *buf) {
	buf->data = NULL;
	buf->size = 0;
	buf->used = 0;

	size_t size = 256;
	buf->data = malloc(size);

	if (!buf->data) {
		return ENOMEM;
	}

	buf->size = size;
	return 0;
}

static int strbuf_ensure(struct strbuf *buf, size_t n) {
	if (buf->used + n > buf->size) {
		size_t size = buf->used + n;
		size_t rem  = size % 256;
		size += rem == 0 ? 256 : rem;
		char *data = realloc(buf->data, size);

		if (!data) return ENOMEM;

		buf->data = data;
		buf->size = size;
	}
	return 0;
}

static int strbuf_append(struct strbuf *buf, const char *str, size_t n) {
	int errnum = strbuf_ensure(buf, n);
	if (errnum != 0) return errnum;
	memcpy(buf->data + buf->used, str, n);
	buf->used += n;
	return 0;
}

static int strbuf_append_nil(struct strbuf *buf) {
	int errnum = strbuf_ensure(buf, 1);
	if (errnum != 0) return errnum;
	buf->data[buf->used] = 0;
	++ buf->used;
	return 0;
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

			if (!decl || decl->type != DECL_CONST) {
				errno = EINVAL; // no such const/arg
				return NULL;
			}

			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
			if (!expr) return NULL;

			expr->ex.value = decl->decl.value;
			return expr;
		}
		else if (argind < parser->argc) {
			errno = EINVAL; // cannot call argument
			return NULL;
		}
		else {
			const struct mathfun_decl *decl = mathfun_context_get(parser->ctx, identifier);

			if (!decl || decl->type != DECL_FUNCT) {
				errno = EINVAL; // no such function
				return NULL;
			}

			struct mathfun_expr *expr = mathfun_expr_alloc(EX_CALL);
			size_t size = 0;
			
			if (!expr) return NULL;

			++ parser->ptr;
			skipws(parser);

			for (;;) {
				ch = *parser->ptr;
				if (!ch || ch == ')') break;
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
				// missing ')'
				mathfun_expr_free(expr);
				return NULL;
			}
			++ parser->ptr;
			skipws(parser);

			if (expr->ex.funct.argc != decl->decl.funct.argc) {
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
		errno = EINVAL;
		return NULL;
	}
	parser->ptr = endptr;

	skipws(parser);

	struct mathfun_expr *expr = mathfun_expr_alloc(EX_CONST);
	if (!expr) return NULL;

	expr->ex.value = value;
	return expr;
}

const char *mathfun_parse_identifier(struct mathfun_parser *parser) {
	const char *from = parser->ptr;

	if (!isalpha(*parser->ptr) && *parser->ptr != '_') {
		errno = EINVAL;
		return NULL;
	}
	++ parser->ptr;

	while (isalnum(*parser->ptr) || *parser->ptr == '_') {
		++ parser->ptr;
	}

	strbuf_clear(&parser->buf);

	if (strbuf_append(&parser->buf, from, parser->ptr - from) != 0 ||
		strbuf_append_nil(&parser->buf) != 0) {
		return NULL;
	}

	skipws(parser);

	return parser->buf.data;
}

struct mathfun_expr *mathfun_context_parse(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code) {
	struct mathfun_expr *expr = NULL;
	struct mathfun_parser parser = { ctx, argnames, argc, code, code, {NULL, 0, 0} };
	int errnum = strbuf_init(&parser.buf);
	if (errnum != 0) return NULL;

	skipws(&parser);
	expr = mathfun_parse_a_expr(&parser);

	strbuf_cleanup(&parser.buf);

	if (expr) {
		skipws(&parser);
		if (*parser.ptr) {
			// trailing garbage
			mathfun_expr_free(expr);
			errno = EINVAL;
			return NULL;
		}
	}

	return expr;
}
