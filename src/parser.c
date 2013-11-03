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
// Note: The parser also does type checks. Arithmetic and comparison operations only work on numbers
// and boolean operations only on boolean values.
//
// test         ::= or_test ["?" or_test ":" test]
// or_test      ::= and_test ("||" and_test)*
// and_test     ::= not_test ("&&" not_test)*
// not_test     ::= "!" not_test | comparison
// comparison   ::= arith_expr (comp_op arith_expr | "in" range)*
// comp_op      ::= "==" | "!=" | "<" | ">" | "<=" | ">="
// range        ::= arith_expr (".."|"...") factor
// arith_expr   ::= term (("+"|"-") term)*
// term         ::= factor (("*"|"/"|"%") factor)*
// factor       ::= ("+"|"-") factor | power
// power        ::= atom ["**" factor]
// atom         ::= identifier ("(" [test ("," test)*] ")")? | number | "true" | "false" | "(" test ")"
// number       ::= "Inf" | "NaN" | ["-"]("0"|"1"..."9"digit*)["."digit*][("e"|"E")["+"|"-"]digit+]
// identifier   ::= (alpha|"_")(alnum|"_")*
//
// strtod is used for number
// isalpha is used for alpha
// isalnum is used for alnum
// isspace is used for whitespace

static mathfun_expr *mathfun_parse_test(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_or_test(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_and_test(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_not_test(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_comparison(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_arith_expr(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_range(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_term(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_factor(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_power(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_atom(mathfun_parser *parser);
static mathfun_expr *mathfun_parse_number(mathfun_parser *parser);
static size_t        mathfun_parse_identifier(mathfun_parser *parser);

mathfun_expr *mathfun_parse_test(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_or_test(parser);

	if (!expr) return NULL;

	if (*parser->ptr == '?') {
		mathfun_expr *cond = expr;

		if (mathfun_expr_type(cond) != MATHFUN_BOOLEAN) {
			// not a boolean expression for condition
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(cond));
			mathfun_expr_free(cond);
			return NULL;
		}

		++ parser->ptr;
		skipws(parser);

		errptr = parser->ptr;
		mathfun_expr *then_expr = mathfun_parse_or_test(parser);

		if (!then_expr) {
			mathfun_expr_free(cond);
			return NULL;
		}

		skipws(parser);

		if (*parser->ptr != ':') {
			mathfun_raise_parser_error(parser, *parser->ptr ? MATHFUN_PARSER_EXPECTED_COLON : MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, NULL);
			mathfun_expr_free(then_expr);
			mathfun_expr_free(cond);
			return NULL;
		}
		
		++ parser->ptr;
		skipws(parser);

		mathfun_expr *else_expr = mathfun_parse_test(parser);

		if (!else_expr) {
			mathfun_expr_free(then_expr);
			mathfun_expr_free(cond);
			return NULL;
		}

		if (mathfun_expr_type(then_expr) != mathfun_expr_type(else_expr)) {
			// type missmatch of the two branches
			mathfun_raise_parser_type_error(parser, errptr, mathfun_expr_type(else_expr), mathfun_expr_type(then_expr));
			mathfun_expr_free(then_expr);
			mathfun_expr_free(else_expr);
			mathfun_expr_free(cond);
			return NULL;
		}

		expr = mathfun_expr_alloc(EX_IIF, parser->error);

		if (!expr) {
			mathfun_expr_free(then_expr);
			mathfun_expr_free(else_expr);
			mathfun_expr_free(cond);
			return NULL;
		}

		expr->ex.iif.cond = cond;
		expr->ex.iif.then_expr = then_expr;
		expr->ex.iif.else_expr = else_expr;
	}

	return expr;
}

mathfun_expr *mathfun_parse_or_test(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_and_test(parser);

	if (!expr) return NULL;

	if (parser->ptr[0] == '|' && parser->ptr[1] == '|') {
		if (mathfun_expr_type(expr) != MATHFUN_BOOLEAN) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}

		do {
			parser->ptr += 2;
			skipws(parser);
			errptr = parser->ptr;

			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_and_test(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			if (mathfun_expr_type(right) != MATHFUN_BOOLEAN) {
				mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(right));
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr = mathfun_expr_alloc(EX_OR, parser->error);
			if (!expr) {
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr->ex.binary.left  = left;
			expr->ex.binary.right = right;

		} while (parser->ptr[0] == '|' && parser->ptr[1] == '|');
	}

	return expr;
}

mathfun_expr *mathfun_parse_and_test(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_not_test(parser);

	if (!expr) return NULL;

	if (parser->ptr[0] == '&' && parser->ptr[1] == '&') {
		if (mathfun_expr_type(expr) != MATHFUN_BOOLEAN) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}

		do {
			parser->ptr += 2;
			skipws(parser);
			errptr = parser->ptr;

			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_not_test(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			if (mathfun_expr_type(right) != MATHFUN_BOOLEAN) {
				mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(right));
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr = mathfun_expr_alloc(EX_AND, parser->error);
			if (!expr) {
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			expr->ex.binary.left  = left;
			expr->ex.binary.right = right;
		} while (parser->ptr[0] == '&' && parser->ptr[1] == '&');
	}

	return expr;
}

mathfun_expr *mathfun_parse_not_test(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = NULL;
	mathfun_expr **exprptr = &expr;

	while (*parser->ptr == '!') {
		++ parser->ptr;
		skipws(parser);
		errptr = parser->ptr;

		mathfun_expr *not_expr = mathfun_expr_alloc(EX_NOT, parser->error);
		if (!not_expr) {
			mathfun_expr_free(expr);
			return NULL;
		}

		*exprptr = not_expr;
		exprptr = &not_expr->ex.unary.expr;

		if (mathfun_expr_type(not_expr) != MATHFUN_BOOLEAN) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(not_expr));
			mathfun_expr_free(expr);
			return NULL;
		}
	}

	errptr = parser->ptr;
	mathfun_expr *comparison_expr = mathfun_parse_comparison(parser);

	if (!comparison_expr) {
		mathfun_expr_free(expr);
		return NULL;
	}

	if (expr && mathfun_expr_type(comparison_expr) != MATHFUN_BOOLEAN) {
		mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, mathfun_expr_type(comparison_expr));
		mathfun_expr_free(comparison_expr);
		mathfun_expr_free(expr);
		return NULL;
	}

	*exprptr = comparison_expr;

	return expr;
}

mathfun_expr *mathfun_parse_range(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *left = mathfun_parse_arith_expr(parser);

	if (!left) return NULL;

	if (mathfun_expr_type(left) != MATHFUN_NUMBER) {
		mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(left));
		mathfun_expr_free(left);
		return NULL;
	}

	if (parser->ptr[-1] == '.') {
		// got something like 5...6 and the dot after the 5 was eaten by the number.
		-- parser->ptr;
	}

	if (parser->ptr[0] == '.' && parser->ptr[1] == '.') {
		enum mathfun_expr_type type;
		if (parser->ptr[2] == '.') {
			type = EX_RNG_EXCL;
			parser->ptr += 3;
		}
		else {
			type = EX_RNG_INCL;
			parser->ptr += 2;
		}
		skipws(parser);

		mathfun_expr *right = mathfun_parse_factor(parser);

		if (!right) {
			mathfun_expr_free(left);
			return NULL;
		}

		if (mathfun_expr_type(right) != MATHFUN_NUMBER) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(right));
			mathfun_expr_free(left);
			mathfun_expr_free(right);
			return NULL;
		}

		mathfun_expr *expr = mathfun_expr_alloc(type, parser->error);

		if (!expr) {
			mathfun_expr_free(left);
			mathfun_expr_free(right);
			return NULL;
		}

		expr->ex.binary.left  = left;
		expr->ex.binary.right = right;

		return expr;
	}

	mathfun_expr_free(left);
	mathfun_raise_parser_error(parser, *parser->ptr ? MATHFUN_PARSER_EXPECTED_DOTS : MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, NULL);
	return NULL;
}

mathfun_expr *mathfun_parse_comparison(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_arith_expr(parser);

	if (!expr) return NULL;

	while (((parser->ptr[0] == '=' || parser->ptr[0] == '!') && parser->ptr[1] == '=') ||
			parser->ptr[0] == '<' || parser->ptr[0] == '>' ||
			(strncasecmp("in", parser->ptr, 2) == 0 && !isalnum(parser->ptr[2]))) {
		enum mathfun_expr_type type;
		mathfun_type left_type = mathfun_expr_type(expr);

		if (parser->ptr[0] == '=' && parser->ptr[1] == '=') {
			parser->ptr += 2;
			type = left_type == MATHFUN_BOOLEAN ? EX_BEQ : EX_EQ;
		}
		else if (parser->ptr[0] == '!' && parser->ptr[1] == '=') {
			parser->ptr += 2;
			type = left_type == MATHFUN_BOOLEAN ? EX_BNE : EX_NE;
		}
		else if (parser->ptr[0] == '<') {
			if (parser->ptr[1] == '=') {
				parser->ptr += 2;
				type = EX_LE;
			}
			else {
				++ parser->ptr;
				type = EX_LT;
			}
		}
		else if (parser->ptr[0] == '>') {
			if (parser->ptr[1] == '=') {
				parser->ptr += 2;
				type = EX_GE;
			}
			else {
				++ parser->ptr;
				type = EX_GT;
			}
		}
		else if (strncasecmp("in", parser->ptr, 2) == 0 && !isalnum(parser->ptr[2])) {
			parser->ptr += 3;
			type = EX_IN;
		}
		else {
			mathfun_raise_error(parser->error, MATHFUN_INTERNAL_ERROR);
			mathfun_expr_free(expr);
			return NULL;
		}
			
		skipws(parser);
		const char *lefterrptr = errptr;
		errptr = parser->ptr;

		mathfun_expr *left  = expr;
		mathfun_expr *right = NULL;
		if (type == EX_IN) {
			if (left_type != MATHFUN_NUMBER) {
				mathfun_raise_parser_type_error(parser, lefterrptr, MATHFUN_NUMBER, left_type);
				mathfun_expr_free(expr);
				return NULL;
			}

			right = mathfun_parse_range(parser);

			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
		}
		else {
			right = mathfun_parse_arith_expr(parser);

			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			mathfun_type right_type = mathfun_expr_type(right);
			if (type == EX_BEQ || type == EX_BNE) {
				// left_type was used to generate EX_BEQ/EX_BNE, so it is MATHFUN_BOOLEAN here
				if (right_type != MATHFUN_BOOLEAN) {
					mathfun_raise_parser_type_error(parser, errptr, MATHFUN_BOOLEAN, right_type);
					mathfun_expr_free(left);
					mathfun_expr_free(right);
					return NULL;
				}
			}
			else if (left_type != MATHFUN_NUMBER) {
				mathfun_raise_parser_type_error(parser, lefterrptr, MATHFUN_NUMBER, left_type);
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
			else if (right_type != MATHFUN_NUMBER) {
				mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, right_type);
				mathfun_expr_free(left);
				mathfun_expr_free(right);
				return NULL;
			}
		}
		expr = mathfun_expr_alloc(type, parser->error);
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

mathfun_expr *mathfun_parse_arith_expr(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_term(parser);

	if (!expr) return NULL;

	char ch = *parser->ptr;
	if (ch == '+' || ch == '-') {
		if (mathfun_expr_type(expr) != MATHFUN_NUMBER) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}

		do {
			++ parser->ptr;
			skipws(parser);
			errptr = parser->ptr;

			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_term(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			if (mathfun_expr_type(right) != MATHFUN_NUMBER) {
				mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(right));
				mathfun_expr_free(left);
				mathfun_expr_free(right);
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

			ch = *parser->ptr;
		} while (ch == '+' || ch == '-');
	}

	return expr;
}

mathfun_expr *mathfun_parse_term(mathfun_parser *parser) {
	const char *errptr = parser->ptr;
	mathfun_expr *expr = mathfun_parse_factor(parser);

	if (!expr) return NULL;

	char ch = *parser->ptr;
	if (ch == '*' || ch == '/' || ch == '%') {
		if (mathfun_expr_type(expr) != MATHFUN_NUMBER) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}

		do {
			++ parser->ptr;
			skipws(parser);
			errptr = parser->ptr;

			mathfun_expr *left  = expr;
			mathfun_expr *right = mathfun_parse_factor(parser);
			if (!right) {
				mathfun_expr_free(left);
				return NULL;
			}
			if (mathfun_expr_type(right) != MATHFUN_NUMBER) {
				mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(right));
				mathfun_expr_free(left);
				mathfun_expr_free(right);
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

			ch = *parser->ptr;
		} while (ch == '*' || ch == '/' || ch == '%');
	}

	return expr;
}

mathfun_expr *mathfun_parse_factor(mathfun_parser *parser) {
	const char ch = *parser->ptr;
	if (ch == '+' || ch == '-') {
		++ parser->ptr;
		skipws(parser);
		const char *errptr = parser->ptr;
		mathfun_expr *expr = mathfun_parse_factor(parser);
		if (!expr) return NULL;

		if (mathfun_expr_type(expr) != MATHFUN_NUMBER) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}

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

	if (parser->ptr[0] == '*' && parser->ptr[1] == '*') {
		parser->ptr += 2;
		skipws(parser);

		const char *errptr  = parser->ptr;
		mathfun_expr *left  = expr;
		mathfun_expr *right = mathfun_parse_factor(parser);
		
		if (!right) {
			mathfun_expr_free(left);
			return NULL;
		}

		if (mathfun_expr_type(right) != MATHFUN_NUMBER) {
			mathfun_raise_parser_type_error(parser, errptr, MATHFUN_NUMBER, mathfun_expr_type(right));
			mathfun_expr_free(left);
			mathfun_expr_free(right);
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
		mathfun_expr *expr = mathfun_parse_test(parser);
		if (!expr) return NULL;
		if (*parser->ptr != ')') {
			// missing ')'
			mathfun_raise_parser_error(parser, *parser->ptr ? MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS : MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, NULL);
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
			expr->ex.value.type = MATHFUN_NUMBER;
			expr->ex.value.value.number = NAN;
			return expr;
		}
		else if (idlen == 3 && strncasecmp(idstart, "inf", idlen) == 0) {
			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;
			expr->ex.value.type = MATHFUN_NUMBER;
			expr->ex.value.value.number = INFINITY;
			return expr;
		}
		else if (idlen == 4 && strncasecmp(idstart, "true", idlen) == 0) {
			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;
			expr->ex.value.type = MATHFUN_BOOLEAN;
			expr->ex.value.value.boolean = true;
			return expr;
		}
		else if (idlen == 5 && strncasecmp(idstart, "false", idlen) == 0) {
			mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
			if (!expr) return NULL;
			expr->ex.value.type = MATHFUN_BOOLEAN;
			expr->ex.value.value.boolean = false;
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

			expr->ex.value.type = MATHFUN_NUMBER;
			expr->ex.value.value.number = decl->decl.value;
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

			if (!expr) {
				return NULL;
			}

			expr->ex.funct.funct = decl->decl.funct.funct;
			expr->ex.funct.sig   = decl->decl.funct.sig;

			if (expr->ex.funct.sig->argc > 0) {
				expr->ex.funct.args = calloc(expr->ex.funct.sig->argc, sizeof(mathfun_expr*));

				if (!expr->ex.funct.args) {
					mathfun_expr_free(expr);
					return NULL;
				}
			}

			++ parser->ptr;
			skipws(parser);

			size_t argc = 0;
			const char *lastarg = parser->ptr;
			for (;;) {
				ch = *parser->ptr;
				if (!ch || ch == ')') break;
				lastarg = parser->ptr;

				mathfun_expr *arg = mathfun_parse_test(parser);

				if (!arg) {
					mathfun_expr_free(expr);
					return NULL;
				}

				if (argc >= expr->ex.funct.sig->argc) {
					mathfun_expr_free(arg);
				}
				else {
					expr->ex.funct.args[argc] = arg;

					if (expr->ex.funct.sig->argtypes[argc] != mathfun_expr_type(arg)) {
						mathfun_raise_parser_type_error(parser, lastarg,
							expr->ex.funct.sig->argtypes[argc], mathfun_expr_type(arg));
						mathfun_expr_free(expr);
						return NULL;
					}
				}

				++ argc;

				if (*parser->ptr != ',') break;

				++ parser->ptr;
				skipws(parser);
			}

			if (*parser->ptr != ')') {
				mathfun_raise_parser_error(parser, *parser->ptr ?
					MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS :
					MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, NULL);
				mathfun_expr_free(expr);
				return NULL;
			}
			++ parser->ptr;
			skipws(parser);

			if (argc != decl->decl.funct.sig->argc) {
				mathfun_raise_parser_argc_error(parser, lastarg, decl->decl.funct.sig->argc, argc);
				mathfun_expr_free(expr);
				return NULL;
			}

			return expr;
		}
	}
}

mathfun_expr *mathfun_parse_number(mathfun_parser *parser) {
	char *endptr = NULL;
	double value = strtod(parser->ptr, &endptr);

	if (parser->ptr == endptr) {
		// can't have MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT here because this function is only called
		// if there is at least one character that can be at the start of a number literal.
		mathfun_raise_parser_error(parser, MATHFUN_PARSER_EXPECTED_NUMBER, NULL);
		return NULL;
	}
	parser->ptr = endptr;

	skipws(parser);

	mathfun_expr *expr = mathfun_expr_alloc(EX_CONST, parser->error);
	if (!expr) return NULL;

	expr->ex.value.type = MATHFUN_NUMBER;
	expr->ex.value.value.number = value;
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
		mathfun_raise_parser_error(parser, *from ? MATHFUN_PARSER_EXPECTED_IDENTIFIER : MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT, from);
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
	mathfun_expr *expr = mathfun_parse_test(&parser);

	if (expr) {
		skipws(&parser);
		if (*parser.ptr) {
			mathfun_raise_parser_error(&parser, MATHFUN_PARSER_TRAILING_GARBAGE, NULL);
			mathfun_expr_free(expr);
			return NULL;
		}

		if (mathfun_expr_type(expr) != MATHFUN_NUMBER) {
			const char *ptr = code;
			while (isspace(*ptr)) ++ ptr;
			mathfun_raise_parser_type_error(&parser, ptr, MATHFUN_NUMBER, mathfun_expr_type(expr));
			mathfun_expr_free(expr);
			return NULL;
		}
	}

	return expr;
}
