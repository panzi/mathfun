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
// number       ::= "Infinity" | "NaN" | ["-"]("0"|"1"..."9"digit*)["."digit*][("e"|"E")["+"|"-"]digit+]
//
// or just use strtod for number

struct mathfun_expr *mathfun_parse_a_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_m_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_u_expr(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_power(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_atom(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_identifier(struct mathfun_parser *parser);
struct mathfun_expr *mathfun_parse_number(struct mathfun_parser *parser);

struct strbuf {
	char  *data;
	size_t size;
	size_t used;
};

int strbuf_init(struct strbuf *buf) {
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

int strbuf_ensure(struct strbuf *buf, size_t n) {
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

int strbuf_append(struct strbuf *buf, const char *str, size_t n) {
	int errnum = strbuf_ensure(but, n);
	if (errnum != 0) return errnum;
	memcpy(buf->data + buf->used, str, n);
	buf->used += n;
	return 0;
}

int strbuf_append_nil(struct strbuf *buf) {
	int errnum = strbuf_ensure(but, 1);
	if (errnum != 0) return errnum;
	buf->data[buf->used] = 0;
	++ buf->used;
	return 0;
}

void strbuf_cleanup(struct strbuf *buf) {
	free(buf->data);
	buf->data = NULL;
	buf->size = 0;
	buf->used = 0;
}

struct mathfun_parser {
	const struct mathfun_context *ctx;
	const char **argnames;
	size_t argc;
	const char *code;
	const char *ptr;
	struct strbuf buf;
};

void skipws(struct mathfun_parser *parser) {
	const char ptr = parser->ptr;
	while (isspace(*ptr)) ++ ptr;
	parser->ptr = ptr;
}

struct mathfun_expr *mathfun_parse_a_expr(struct mathfun_parser *parser) {
	struct mathfun_expr *expr = mathfun_parse_m_expr(parser);

	if (!expr) return NULL;

	for (;;) {
		skipws(parser);
		char ch = *parser->ptr;
		if (ch == '+' || ch == '-') {
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
			expr.ex.binary.left  = left;
			expr.ex.binary.right = right;
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
		char ch = *parser->ptr;
		if (ch == '*' || ch == '/' || ch == '%') {
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
			expr.ex.binary.left  = left;
			expr.ex.binary.right = right;
		}
		else {
			break;
		}
	}

	return expr;
}

struct mathfun_expr *mathfun_parse_u_expr(struct mathfun_parser *parser) {
	char ch = *parser->ptr;
	if (ch == '+' || ch == '-') {
		++ parser->ptr;
		skipws(parser);
		struct mathfun_expr *expr = mathfun_parse_u_expr(parser);
		if (!expr) return NULL;

		if (ch == '-') {
			struct mathfun_expr *child = expr;
			expr = mathfun_expr_alloc(EX_NEG);

			if (!expr) {
				mathfun_expr_free(child):
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

		expr.ex.binary.left  = left;
		expr.ex.binary.right = right;
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
		return mathfun_lex_number(parser);
	}
	else {
		struct mathfun_expr *expr = mathfun_parse_identifier(parser);
		if (!expr) return NULL;

		if (strbuf_append_nil(&parser->buf) != 0) {
			mathfun_expr_free(expr);
			return NULL;
		}

		skipws(parser);
		if (*parser->ptr != '(') {
			expr->type = EX_CONST;
			struct mathfun_decl *decl = mathfun_context_get(parser->ctx, parser->buf.data);

			if (!decl || decl->type != DECL_CONST) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.value = decl->decl.value;
		}
		else {
			size_t size = 0;
			size_t argc = 0;
			struct mathfun_expr **args = NULL;
			
			++ parser->ptr;
			skipws(parser);

			for (;;) {
				ch = *parser->ptr;
				if (!ch || ch == ')') break;
				struct mathfun_expr *arg = mathfun_parse_a_expr(parser);

				if (!arg) {
					expr->type = EX_CALL;
					expr->ex.funct.args = args;
					expr->ex.funct.argc = argc;
					mathfun_expr_free(expr);
					return NULL;
				}

				if (size == argc) {
					size += 16;
					struct mathfun_expr **newargs = realloc(args, size);

					if (!newargs) {
						expr->type = EX_CALL;
						expr->ex.funct.args = args;
						expr->ex.funct.argc = argc;
						mathfun_expr_free(expr);
						mathfun_expr_free(arg);
						return NULL;
					}

					args = newargs;
				}

				args[argc] = arg;
				++ argc;

				if (*parser->ptr != ',') break;

				++ parser->ptr;
				skipws(parser);
			}
			
			expr->type = EX_CALL;
			expr->ex.funct.args = args;
			expr->ex.funct.argc = argc;

			if (*parser->ptr != ')') {
				// missing ')'
				mathfun_expr_free(expr);
				return NULL;
			}
			++ parser->ptr;
			skipws(parser);

			struct mathfun_decl *decl = mathfun_context_get(parser->ctx, parser->buf.data);

			if (!decl || decl->type != DECL_FUNCT || argc != decl->decl.funct.argc) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.funct.funct = decl->decl.funct.funct;
		}

		return expr;
	}
}

struct mathfun_expr *mathfun_parse_number(struct mathfun_parser *parser) {
	// TODO
}

struct mathfun_expr *mathfun_parse_identifier(struct mathfun_parser *parser) {
	// TODO
}

struct mathfun_expr *mathfun_context_parse(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code) {
	struct mathfun_expr *expr = NULL;
	struct mathfun_parser parser = { ctx, argnames, argc, code, code, {NULL, 0, 0} };
	int errnum = strbuf_init(&parser.buf);
	if (errnum != 0) return NULL;

	skipws(parser);
	expr = mathfun_parse_a_expr(&parser);

	strbuf_cleanup(&parser.buf);

	if (expr) {
		parser.ptr = skipws(parser.ptr);
		if (*parser.ptr) {
			// trailing garbage
			return errno = EINVAL;
		}
	}

	return expr;
}
