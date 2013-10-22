#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "mathfun_intern.h"

#define skipws(parser) { \
	const char *ptr = (parser)->ptr; \
	while (isspace(*ptr)) ++ ptr; \
	(parser)->ptr = ptr; \
	}

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

static mathfun_expr *mathfun_parse_a_expr(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_m_expr(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_u_expr(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_power(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_atom(mathfun_parser *parser);
static size_t        mathfun_parse_identifier(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_number(mathfun_parser *parser);

mathfun_expr *mathfun_parse_a_expr(mathfun_parser *parser) {
	mathfun_expr *expr = mathfun_parse_m_expr(parser);

	if (!expr) return NULL;

	for (;;) {
		skipws(parser);
		const char ch = *parser->ptr;
		if (ch == '+' || ch == '-') {
			++ parser->ptr;
			skipws(parser);
			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_m_expr(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			expr = mathfun_expr_alloc(ch == '+' ? EX_ADD : EX_SUB, parser->error);
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

mathfun_expr *mathfun_parse_m_expr(mathfun_parser *parser) {
	mathfun_expr *expr = mathfun_parse_u_expr(parser);

	if (!expr) return NULL;

	for (;;) {
		skipws(parser);
		const char ch = *parser->ptr;
		if (ch == '*' || ch == '/' || ch == '%') {
			++ parser->ptr;
			skipws(parser);
			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_u_expr(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			expr = mathfun_expr_alloc(ch == '*' ? EX_MUL : ch == '/' ? EX_DIV : EX_MOD, parser->error);
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

mathfun_expr *mathfun_parse_u_expr(mathfun_parser *parser) {
	const char ch = *parser->ptr;
	if (ch == '+' || ch == '-') {
		++ parser->ptr;
		skipws(parser);
		mathfun_expr *expr = mathfun_parse_u_expr(parser);
		if (!expr) return NULL;

		if (ch == '-') {
			mathfun_expr *child = expr;
			expr = mathfun_expr_alloc(EX_NEG, parser->error);

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

mathfun_expr *mathfun_parse_power(mathfun_parser *parser) {
	mathfun_expr *expr = mathfun_parse_atom(parser);

	if (!expr) return NULL;

	skipws(parser);

	if (parser->ptr[0] == '*' && parser->ptr[1] == '*') {
		parser->ptr += 2;
		skipws(parser);

		mathfun_expr *left  = expr;
		mathfun_expr *right = mathfun_parse_u_expr(parser);
		
		if (!right) {
			mathfun_expr_free(left);
			return NULL;
		}

		expr = mathfun_expr_alloc(EX_POW, parser->error);
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

mathfun_expr *mathfun_parse_atom(mathfun_parser *parser) {
	char ch = *parser->ptr;

	if (ch == '(') {
		++ parser->ptr;
		skipws(parser);
		mathfun_expr *expr = mathfun_parse_a_expr(parser);
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
		const size_t idlen = mathfun_parse_identifier(parser);
		if (idlen == 0) return NULL;

		if (idlen == 3 && strncasecmp(idstart, "nan", idlen) == 0) {
			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;
			expr->ex.value = NAN;
			return expr;
		}
		else if (idlen == 3 && strncasecmp(idstart, "inf", idlen) == 0) {
			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;
			expr->ex.value = INFINITY;
			return expr;
		}

		size_t argind = 0;
		for (; argind < parser->argc; ++ argind) {
			const char *argname = parser->argnames[argind];
			if (strncmp(argname, idstart, idlen) == 0 && !argname[idlen]) {
				break;
			}
		}

		if (*parser->ptr != '(') {
			if (argind < parser->argc) {
				mathfun_expr *expr = mathfun_expr_alloc(EX_ARG, parser->error);
				if (!expr) return NULL;
				expr->ex.arg = argind;
				return expr;
			}

			const mathfun_decl *decl = mathfun_context_getn(parser->ctx, idstart, idlen);

			if (!decl) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_UNDEFINED_REFERENCE, idstart);
				return NULL;
			}

			if (decl->type != DECL_CONST) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_VARIABLE, idstart);
				return NULL;
			}

			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;

			expr->ex.value = decl->decl.value;
			return expr;
		}
		else if (argind < parser->argc) {
			mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_FUNCTION, idstart);
			return NULL;
		}
		else {
			const mathfun_decl *decl = mathfun_context_getn(parser->ctx, idstart, idlen);

			if (!decl) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_UNDEFINED_REFERENCE, idstart);
				return NULL;
			}

			if (decl->type != DECL_FUNCT) {
				mathfun_raise_parser_error(parser, MATHFUN_PARSER_NOT_A_FUNCTION, idstart);
				return NULL;
			}

			mathfun_expr *expr = mathfun_expr_alloc(EX_CALL, parser->error);
			size_t size = 0;

			if (!expr) return NULL;

			++ parser->ptr;
			skipws(parser);

			const char *lastarg = parser->ptr;
			for (;;) {
				ch = *parser->ptr;
				if (!ch || ch == ')') break;
				lastarg = parser->ptr;
				mathfun_expr *arg = mathfun_parse_a_expr(parser);

				if (!arg) {
					mathfun_expr_free(expr);
					return NULL;
				}

				if (size == expr->ex.funct.argc) {
					size += 16;
					mathfun_expr **args = realloc(expr->ex.funct.args, size * sizeof(mathfun_expr*));

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

mathfun_expr *mathfun_parse_number(mathfun_parser *parser) {
	char *endptr = NULL;
	mathfun_value value = strtod(parser->ptr, &endptr);

	if (parser->ptr == endptr) {
		mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_NUMBER, NULL);
		return NULL;
	}
	parser->ptr = endptr;

	skipws(parser);

	mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
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

size_t mathfun_parse_identifier(mathfun_parser *parser) {
	const char *from = parser->ptr;
	parser->ptr = mathfun_find_identifier_end(from);

	if (from == parser->ptr) {
		mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_IDENTIFIER, from);
		return 0;
	}

	size_t n = parser->ptr - from;

	skipws(parser);

	return n;
}

mathfun_expr *mathfun_context_parse(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code, mathfun_error_p *error) {
	if (!mathfun_validate_argnames(argnames, argc, error)) return NULL;

	mathfun_parser parser = { ctx, argnames, argc, code, code, error };

	skipws(&parser);
	mathfun_expr *expr = mathfun_parse_a_expr(&parser);

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
