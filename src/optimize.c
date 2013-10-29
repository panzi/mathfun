#include <errno.h>

#include "mathfun_intern.h"

typedef double (*mathfun_binary_op)(double a, double b);
typedef bool (*mathfun_cmp)(double a, double b);

static double mathfun_add(double a, double b) { return a + b; }
static double mathfun_sub(double a, double b) { return a - b; }
static double mathfun_mul(double a, double b) { return a * b; }
static double mathfun_div(double a, double b) { return a / b; }

static bool mathfun_opt_eq(double a, double b) { return a == b; }
static bool mathfun_opt_ne(double a, double b) { return a != b; }
static bool mathfun_opt_gt(double a, double b) { return a >  b; }
static bool mathfun_opt_lt(double a, double b) { return a <  b; }
static bool mathfun_opt_ge(double a, double b) { return a >= b; }
static bool mathfun_opt_le(double a, double b) { return a <= b; }

static mathfun_expr *mathfun_expr_optimize_binary(mathfun_expr *expr,
	mathfun_binary_op op, bool has_neutral, double neutral, bool commutative,
	mathfun_error_p *error) {

	expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
	if (!expr->ex.binary.left) {
		mathfun_expr_free(expr);
		return NULL;
	}

	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
	if (!expr->ex.binary.right) {
		mathfun_expr_free(expr);
		return NULL;
	}

	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {

		errno = 0;
		double value = op(expr->ex.binary.left->ex.value.value.number, expr->ex.binary.right->ex.value.value.number);
		if (errno != 0) {
			mathfun_raise_c_error(error);
			mathfun_expr_free(expr);
			return NULL;
		}

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value.type = MATHFUN_NUMBER;
		expr->ex.value.value.number = value;
	}
	else if (has_neutral) {
		if (expr->ex.binary.right->type == EX_CONST && expr->ex.binary.right->ex.value.value.number == neutral) {
			mathfun_expr *left = expr->ex.binary.left;
			expr->ex.binary.left = NULL;
			mathfun_expr_free(expr);
			expr = left;
		}
		else if (commutative && expr->ex.binary.left->type == EX_CONST &&
			expr->ex.binary.left->ex.value.value.number == neutral) {
			mathfun_expr *right = expr->ex.binary.right;
			expr->ex.binary.right = NULL;
			mathfun_expr_free(expr);
			expr = right;
		}
	}
	return expr;
}

static mathfun_expr *mathfun_expr_optimize_comparison(mathfun_expr *expr,
	mathfun_cmp cmp, mathfun_error_p *error) {

	expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
	if (!expr->ex.binary.left) {
		mathfun_expr_free(expr);
		return NULL;
	}

	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
	if (!expr->ex.binary.right) {
		mathfun_expr_free(expr);
		return NULL;
	}
	
	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {

		bool value = cmp(expr->ex.binary.left->ex.value.value.number, expr->ex.binary.right->ex.value.value.number);

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value.type = MATHFUN_BOOLEAN;
		expr->ex.value.value.boolean = value;
	}

	return expr;
}

static mathfun_expr *mathfun_expr_optimize_boolean_comparison(mathfun_expr *expr, mathfun_error_p *error) {

	expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
	if (!expr->ex.binary.left) {
		mathfun_expr_free(expr);
		return NULL;
	}

	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
	if (!expr->ex.binary.right) {
		mathfun_expr_free(expr);
		return NULL;
	}
	
	mathfun_expr *const_expr;
	mathfun_expr *other_expr;
	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {

		bool value = expr->type == EX_BEQ ?
			expr->ex.binary.left->ex.value.value.boolean == expr->ex.binary.right->ex.value.value.boolean :
			expr->ex.binary.left->ex.value.value.boolean != expr->ex.binary.right->ex.value.value.boolean;

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value.type = MATHFUN_BOOLEAN;
		expr->ex.value.value.boolean = value;
		return expr;
	}
	else if (expr->ex.binary.left->type == EX_CONST) {
		const_expr = expr->ex.binary.left;
		other_expr = expr->ex.binary.right;
	}
	else if (expr->ex.binary.right->type == EX_CONST) {
		const_expr = expr->ex.binary.right;
		other_expr = expr->ex.binary.left;
	}
	else {
		return expr;
	}

	bool value = const_expr->ex.value.value.boolean;

	if ((expr->type == EX_BEQ && value) || (expr->type == EX_BNE && !value)) {
		expr->ex.binary.left  = NULL;
		expr->ex.binary.right = NULL;
		mathfun_expr_free(const_expr);
		mathfun_expr_free(expr);
		return other_expr;
	}
	else if (other_expr->type == EX_NOT) {
		mathfun_expr *child = other_expr->ex.unary.expr;
		other_expr->ex.unary.expr = NULL;
		mathfun_expr_free(expr);
		return child;
	}
	else {
		mathfun_expr_free(const_expr);
		expr->type = EX_NOT;
		expr->ex.unary.expr = other_expr;
		return expr;
	}
}

mathfun_expr *mathfun_expr_optimize(mathfun_expr *expr, mathfun_error_p *error) {
	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
			return expr;

		case EX_CALL:
		{
			bool allconst = true;
			const size_t argc = expr->ex.funct.sig->argc;
			for (size_t i = 0; i < argc; ++ i) {
				mathfun_expr *child = expr->ex.funct.args[i] =
					mathfun_expr_optimize(expr->ex.funct.args[i], error);
				if (!child) {
					mathfun_expr_free(expr);
					return NULL;
				}
				else if (child->type != EX_CONST) {
					allconst = false;
				}
			}
			if (allconst) {
				mathfun_reg *args = calloc(argc, sizeof(mathfun_reg));
				if (!args) return NULL;
				for (size_t i = 0; i < argc; ++ i) {
					args[i] = expr->ex.funct.args[i]->ex.value.value;
				}

				// math errors are communicated via errno
				// XXX: buggy. see NOTES in man math_error
				errno = 0;
				mathfun_reg value = expr->ex.funct.funct(args);

				free(args);
				expr->type = EX_CONST;
				expr->ex.value.type = expr->ex.funct.sig->rettype;
				expr->ex.value.value = value;

				if (errno != 0) {
					mathfun_raise_c_error(error);
					mathfun_expr_free(expr);
					return NULL;
				}
			}
			return expr;
		}

		case EX_NEG:
			expr->ex.unary.expr = mathfun_expr_optimize(expr->ex.unary.expr, error);
			if (!expr->ex.unary.expr) {
				mathfun_expr_free(expr);
				return NULL;
			}
			else if (expr->ex.unary.expr->type == EX_NEG) {
				mathfun_expr *child = expr->ex.unary.expr->ex.unary.expr;
				expr->ex.unary.expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				return child;
			}
			else if (expr->ex.unary.expr->type == EX_CONST) {
				mathfun_expr *child = expr->ex.unary.expr;
				expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				child->ex.value.value.number = -child->ex.value.value.number;
				return child;
			}
			return expr;

		case EX_ADD: return mathfun_expr_optimize_binary(expr, mathfun_add, true,    0, true,  error);
		case EX_SUB: return mathfun_expr_optimize_binary(expr, mathfun_sub, true,    0, false, error);
		case EX_MUL: return mathfun_expr_optimize_binary(expr, mathfun_mul, true,    1, true,  error);
		case EX_DIV: return mathfun_expr_optimize_binary(expr, mathfun_div, true,    1, false, error);
		case EX_MOD: return mathfun_expr_optimize_binary(expr, mathfun_mod, false, NAN, false, error);
		case EX_POW: return mathfun_expr_optimize_binary(expr, pow,         true,    1, false, error);

		case EX_NOT:
			expr->ex.unary.expr = mathfun_expr_optimize(expr->ex.unary.expr, error);
			if (!expr->ex.unary.expr) {
				mathfun_expr_free(expr);
				return NULL;
			}
			else if (expr->ex.unary.expr->type == EX_NOT) {
				mathfun_expr *child = expr->ex.unary.expr->ex.unary.expr;
				expr->ex.unary.expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				return child;
			}
			else if (expr->ex.unary.expr->type == EX_CONST) {
				mathfun_expr *child = expr->ex.unary.expr;
				expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				child->ex.value.value.boolean = !child->ex.value.value.boolean;
				return child;
			}
			return expr;

		case EX_EQ: return mathfun_expr_optimize_comparison(expr, mathfun_opt_eq, error);
		case EX_NE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_ne, error);
		case EX_LT: return mathfun_expr_optimize_comparison(expr, mathfun_opt_lt, error);
		case EX_GT: return mathfun_expr_optimize_comparison(expr, mathfun_opt_gt, error);
		case EX_LE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_le, error);
		case EX_GE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_ge, error);

		case EX_BEQ:
		case EX_BNE: return mathfun_expr_optimize_boolean_comparison(expr, error);

		case EX_AND:
		{
			expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
			if (!expr->ex.binary.left) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
			if (!expr->ex.binary.right) {
				mathfun_expr_free(expr);
				return NULL;
			}

			mathfun_expr *const_expr;
			mathfun_expr *other_expr;
			if (expr->ex.binary.left->type  == EX_CONST &&
				expr->ex.binary.right->type == EX_CONST) {
				bool value = expr->ex.binary.left->ex.value.value.boolean && expr->ex.binary.right->ex.value.value.boolean;

				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = value;
				return expr;
			}
			else if (expr->ex.binary.left->type == EX_CONST) {
				const_expr = expr->ex.binary.left;
				other_expr = expr->ex.binary.right;
			}
			else if (expr->ex.binary.right->type == EX_CONST) {
				other_expr = expr->ex.binary.left;
				const_expr = expr->ex.binary.right;
			}
			else {
				return expr;
			}

			if (const_expr->ex.value.value.boolean) {
				expr->ex.binary.left  = NULL;
				expr->ex.binary.right = NULL;
				mathfun_expr_free(const_expr);
				mathfun_expr_free(expr);
				return other_expr;
			}
			else {
				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = false;
				return expr;
			}
		}
		case EX_OR:
		{
			expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
			if (!expr->ex.binary.left) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
			if (!expr->ex.binary.right) {
				mathfun_expr_free(expr);
				return NULL;
			}

			mathfun_expr *const_expr;
			mathfun_expr *other_expr;
			if (expr->ex.binary.left->type  == EX_CONST &&
				expr->ex.binary.right->type == EX_CONST) {
				bool value = expr->ex.binary.left->ex.value.value.boolean || expr->ex.binary.right->ex.value.value.boolean;

				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = value;
				return expr;
			}
			else if (expr->ex.binary.left->type == EX_CONST) {
				const_expr = expr->ex.binary.left;
				other_expr = expr->ex.binary.right;
			}
			else if (expr->ex.binary.right->type == EX_CONST) {
				other_expr = expr->ex.binary.left;
				const_expr = expr->ex.binary.right;
			}
			else {
				return expr;
			}

			if (const_expr->ex.value.value.boolean) {
				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = true;
				return expr;
			}
			else {
				expr->ex.binary.left  = NULL;
				expr->ex.binary.right = NULL;
				mathfun_expr_free(const_expr);
				mathfun_expr_free(expr);
				return other_expr;
			}
		}
		case EX_IIF:
		{
			expr->ex.iif.cond = mathfun_expr_optimize(expr->ex.iif.cond, error);
			if (!expr->ex.iif.cond) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.iif.then_expr = mathfun_expr_optimize(expr->ex.iif.then_expr, error);
			if (!expr->ex.iif.then_expr) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.iif.else_expr = mathfun_expr_optimize(expr->ex.iif.else_expr, error);
			if (!expr->ex.iif.else_expr) {
				mathfun_expr_free(expr);
				return NULL;
			}

			if (expr->ex.iif.cond->type == EX_CONST) {
				mathfun_expr *child;
				if (expr->ex.iif.cond->ex.value.value.boolean) {
					child = expr->ex.iif.then_expr;
					expr->ex.iif.then_expr = NULL;
				}
				else {
					child = expr->ex.iif.else_expr;
					expr->ex.iif.else_expr = NULL;
				}
				mathfun_expr_free(expr);
				return child;
			}

			return expr;
		}
	}

	return expr;
}
